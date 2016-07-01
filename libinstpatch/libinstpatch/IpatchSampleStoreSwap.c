/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: IpatchSampleStoreSwap
 * @short_description: Sample storage object for audio in memory or temporary
 *   swap file
 * @see_also: 
 * @stability: Stable
 *
 * Swap sample stores are used for data which does not have a safe external
 * source, for example if a sample was originally loaded from an external
 * audio file or an instrument file that was closed.
 *
 * Swap sample stores are stored in RAM up to the total size set by
 * ipatch_sample_store_swap_set_max_memory().  Additional sample stores
 * are written to the swap file, whose file name is set by
 * ipatch_sample_store_set_file_name() with a fallback to a temporary file
 * name if not set.
 *
 * Currently there is a global lock on read or write accesses of sample stores
 * in the swap file.  This is contrary to most other sample store types.
 *
 * When a sample store in the swap file is no longer used, it is added to a
 * recover list, which new sample stores may utilize.  This cuts down on unused
 * space in the swap file (ipatch_sample_store_swap_get_unused_size()), which
 * can be compacted with ipatch_sample_store_swap_compact().
 */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include "IpatchSampleStoreSwap.h"
#include "ipatch_priv.h"
#include "compat.h"
#include "i18n.h"
#include "config.h"

#define MAX_RAM_SWAP            (32*1024*1024)          // Default maximum RAM memory swap

/* Indicates if a swap sample store has been allocated space */
#define SAMPLE_ALLOCATED  (1 << IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT)

/* Keeps track of areas in the swap file which are no longer used and can be
 * re-used by new samples */
typedef struct
{
  guint size;
  guint location;
} SwapRecover;


static gint ipatch_sample_store_swap_recover_size_sort_func (gconstpointer a, gconstpointer b);
static void ipatch_sample_store_swap_sample_iface_init (IpatchSampleIface *iface);
static gboolean ipatch_sample_store_swap_sample_iface_open (IpatchSampleHandle *handle,
                                                            GError **err);
static void ipatch_sample_store_swap_open_file (void);
static gboolean
ipatch_sample_store_swap_sample_iface_read (IpatchSampleHandle *handle,
                                            guint offset, guint frames,
                                            gpointer buf, GError **err);
static gboolean
ipatch_sample_store_swap_sample_iface_write (IpatchSampleHandle *handle,
                                             guint offset, guint frames,
                                             gconstpointer buf, GError **err);
static void ipatch_sample_store_swap_finalize (GObject *gobject);


G_LOCK_DEFINE_STATIC (swap);
static int swap_fd = -1;
static char *swap_file_name = NULL;
static guint swap_position = 0;                         // Current position in swap file, for new sample data
static volatile gint swap_unused_size = 0;              // Amount of wasted space (unused samples)
static volatile gint swap_ram_used = 0;                 // Amount of RAM memory used for swap
static volatile gint swap_ram_max = MAX_RAM_SWAP;       // Maximum amount of RAM swap storage
static GSList *swap_list = NULL;                        // List of #IpatchSampleStoreSwap objects stored on disk

// Both recover lists share the same SwapRecover structure
static GSList *swap_recover_list = NULL;        // List of SwapRecover structures sorted by size field (from larger to smaller)
static GSList *swap_recover_loc_list = NULL;    // List of SwapRecover structures sorted by location field (from low to high)

G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreSwap, ipatch_sample_store_swap,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_swap_sample_iface_init))

static SwapRecover *
ipatch_sample_store_swap_recover_new (void)
{
  return (g_slice_new0 (SwapRecover));
}

static void
ipatch_sample_store_swap_recover_free (SwapRecover *recover)
{
  g_slice_free (SwapRecover, recover);
}

/* Add a recover segment to the recover pool, merging segments if possible.
 * NOTE: swap must be locked by caller.  Allocation of recover structure is taken over. */
static void
ipatch_sample_store_swap_recover_add (SwapRecover *recover)
{
  GSList *p, *next, *prev;
  SwapRecover *compare, *compare2;
  guint end;

  end = recover->location + recover->size;

  // See if this segment can be joined with other ones
  for (p = swap_recover_loc_list, prev = NULL; p; prev = p, p = p->next)
  {
    compare = (SwapRecover *)(p->data);

    // Can this segment be merged?
    if (compare->location == end || compare->location + compare->size == recover->location)
    {
      compare->size += recover->size;

      if (compare->location != end)     // If recover segment comes after compare, see if the next segment can be merged too
      {
        next = p->next;

        if (next)
        {
          compare2 = (SwapRecover *)(next->data);

          if (compare->location + compare->size == compare2->location)
          {
            compare->size += compare2->size;
            p->next = next->next;
            g_slist_free_1 (next);                      // Free the 2nd segment and list node
            ipatch_sample_store_swap_recover_free (compare2);

            swap_recover_list = g_slist_remove (swap_recover_list, compare2);           // Remove from recover size list also
          }
        }
      }
      else compare->location = recover->location;       // Recover segment comes before, no more mergable segments

      // Remove merged segment from recover size list and re-insert sorted by its new size
      swap_recover_list = g_slist_remove (swap_recover_list, compare);
      swap_recover_list = g_slist_insert_sorted (swap_recover_list, compare,
                                                 ipatch_sample_store_swap_recover_size_sort_func);

      ipatch_sample_store_swap_recover_free (recover);  // -- Recover segment was merged, free it
      recover = NULL;
      break;
    }

    if (compare->location > recover->location) break;
  }

  if (recover)  // Recover segment didn't get merged? - insert it into both lists
  {
    p = g_slist_append (NULL, recover);
    p->next = prev ? prev->next : swap_recover_loc_list;

    if (prev) prev->next = p;
    else swap_recover_loc_list = p;

    swap_recover_list = g_slist_insert_sorted (swap_recover_list, recover,      // !! takes over allocation of recover structure
                                               ipatch_sample_store_swap_recover_size_sort_func);
  }
}

// Function to sort recover segments in largest to smallest order
static gint
ipatch_sample_store_swap_recover_size_sort_func (gconstpointer a, gconstpointer b)
{
  const SwapRecover *recover_a = a, *recover_b = b;
  return (recover_b->size - recover_a->size);
}

static void
ipatch_sample_store_swap_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_swap_sample_iface_open;
  iface->read = ipatch_sample_store_swap_sample_iface_read;
  iface->write = ipatch_sample_store_swap_sample_iface_write;
}

static void
ipatch_sample_store_swap_class_init (IpatchSampleStoreSwapClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_sample_store_swap_finalize;
}

static gboolean
ipatch_sample_store_swap_sample_iface_open (IpatchSampleHandle *handle,
                                            GError **err)
{
  IpatchSampleStoreSwap *store = IPATCH_SAMPLE_STORE_SWAP (handle->sample);
  gboolean already_allocated_or_write_mode;
  guint sample_size;
  gint new_ram_used;
  guint flags;
  SwapRecover *recover;
  GSList *p, *prev, *prevprev;

  ipatch_sample_get_size (IPATCH_SAMPLE (store), &sample_size);
  flags = ipatch_item_get_flags (IPATCH_ITEM (store));

  g_return_val_if_fail (sample_size > 0, FALSE);

  already_allocated_or_write_mode = (flags & SAMPLE_ALLOCATED) || !handle->read_mode;
  g_return_val_if_fail (already_allocated_or_write_mode, FALSE);

  // !! No lock needed on sample store since sample data is set once only, prior to multi-thread usage

  // Has sample been allocated?
  if (!(flags & SAMPLE_ALLOCATED))
  {
    new_ram_used = g_atomic_int_exchange_and_add (&swap_ram_used, sample_size);
    new_ram_used += sample_size;        // Value returned is the amount before the add

    // Check if allocating sample in RAM would exceed max allowed
    if (new_ram_used > g_atomic_int_get (&swap_ram_max))
    { // RAM swap is maxed out - correct swap_ram_used
      g_atomic_int_add (&swap_ram_used, -sample_size);

      if (swap_fd == -1)   /* Swap file not yet created? */
        ipatch_sample_store_swap_open_file ();

      if (swap_fd != -1)
      {
        G_LOCK (swap);          // ++ lock swap_list

        // See if there are any recover segments that can be used (find closest size, they are sorted largest to smallest)
        for (p = swap_recover_list, prev = NULL, prevprev = NULL; p;
             prevprev = prev, prev = p, p = p->next)
        {
          recover = (SwapRecover *)(p->data);
          if (recover->size < sample_size) break;
        }

        if (prev)
        {
          recover = (SwapRecover *)(prev->data);
          store->location = recover->location;

          recover->size -= sample_size;
          recover->location += sample_size;
          g_atomic_int_add (&swap_unused_size, -sample_size);

          // Remove the node from the size recover list
          if (prevprev) prevprev->next = prev->next;
          else swap_recover_list = prev->next;

          g_slist_free_1 (prev);        // Delete list node

          swap_recover_loc_list = g_slist_remove (swap_recover_loc_list, recover);      // Remove from location list

          if (recover->size > 0)
            ipatch_sample_store_swap_recover_add (recover);             // Re-add segment to recover lists
          else ipatch_sample_store_swap_recover_free (recover);         // Free empty recover structure
        }
        else    // No adequate recover segment found, reserve new area
        {
          store->location = swap_position;
          swap_position += sample_size;
        }
  
        swap_list = g_slist_prepend (swap_list, store);         // Append disk swap store to swap_list
        G_UNLOCK (swap);        // -- unlock swap_list
      }
      else store->ram_location = g_malloc (sample_size);  // Failed to open swap file - allocate sample store in memory
    }
    else store->ram_location = g_malloc (sample_size);  // Allocate sample store in memory

    ipatch_item_set_flags (IPATCH_ITEM (store), SAMPLE_ALLOCATED);
  }

  handle->data1 = GUINT_TO_POINTER (ipatch_sample_format_size (ipatch_sample_store_get_format (store)));

  return (TRUE);
}

/* Opens swap file (either assigned file name or temporary file) */
static void
ipatch_sample_store_swap_open_file (void)
{
  char *template = NULL, *s;
  GError *local_err = NULL;

  G_LOCK (swap);        // ++ lock swap

  if (swap_file_name)   // Use existing name if it was assigned
  {
    swap_fd = g_open (swap_file_name, O_RDWR | O_CREAT, 0600);

    if (swap_fd != -1)
    {
      G_UNLOCK (swap);  // -- unlock swap
      return;
    }

    g_warning (_("Failed to open sample swap file '%s': %s"), swap_file_name,
               g_strerror (errno));

    // Failed to open swap file, free assigned file name and fall back to temporary file
    g_free (swap_file_name);
    swap_file_name = NULL;
  }

  // Use application name, if set, as prefix of swap file
  if (ipatch_application_name)
  {
    template = g_strdup (ipatch_application_name);    // ++ allocate template application name string

    s = strchr (template, ' ');               // Search for space version separator in application name
    if (s) *s = '\0';

    if (strlen (template) > 0)
    {
      s = g_strconcat (template, "-swap_XXXXXX", NULL);         // ++ allocate full template string
      g_free (template);        // -- free old template string
      template = s;
    }
    else
    {
      g_free (template);      // -- free template string
      template = NULL;
    }
  }

  swap_fd = g_file_open_tmp (template ? template : "libInstPatch-swap_XXXXXX",
                             &swap_file_name, &local_err);
  g_free (template);          // -- free template string (if set)

  if (swap_fd == -1)
  {
    g_critical (_("Failed to create temp sample store swap file: %s"),
                ipatch_gerror_message (local_err));
    g_clear_error (&local_err);
  }

  G_UNLOCK (swap);      // -- unlock swap
}

static gboolean
ipatch_sample_store_swap_sample_iface_read (IpatchSampleHandle *handle,
                                            guint offset, guint frames,
                                            gpointer buf, GError **err)
{
  IpatchSampleStoreSwap *store = (IpatchSampleStoreSwap *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);
  ssize_t retval;

  if (store->ram_location)
  {
    memcpy (buf, ((guint8 *)store->ram_location) + offset * frame_size, frames * frame_size);
    return (TRUE);
  }

  G_LOCK (swap);        // ++ lock swap

  if (lseek (swap_fd, store->location + offset * frame_size, SEEK_SET) == -1)
  {
    G_UNLOCK (swap);    // -- unlock swap
    g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                 _("Error seeking in sample store swap file: %s"), g_strerror (errno));
    return (FALSE);
  }

  retval = read (swap_fd, buf, frames * frame_size);

  if (retval == -1)
    g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                 _("Error reading from sample store swap file: %s"), g_strerror (errno));
  else if (retval < frames * frame_size)
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                 _("Short read from sample store swap file, expected %d but got %" G_GSSIZE_FORMAT),
                 frames * frame_size, retval);

  G_UNLOCK (swap);    // -- unlock swap

  return (retval == frames * frame_size);
}

static gboolean
ipatch_sample_store_swap_sample_iface_write (IpatchSampleHandle *handle,
                                             guint offset, guint frames,
                                             gconstpointer buf, GError **err)
{
  IpatchSampleStoreSwap *store = (IpatchSampleStoreSwap *)(handle->sample);
  guint8 frame_size = GPOINTER_TO_UINT (handle->data1);
  ssize_t retval;

  if (store->ram_location)
  {
    memcpy (((guint8 *)store->ram_location) + offset * frame_size, buf, frames * frame_size);
    return (TRUE);
  }

  G_LOCK (swap);        // ++ lock swap

  if (lseek (swap_fd, store->location + offset * frame_size, SEEK_SET) == -1)
  {
    G_UNLOCK (swap);    // -- unlock swap
    g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                 _("Error seeking in sample store swap file: %s"), g_strerror (errno));
    return (FALSE);
  }

  retval = write (swap_fd, buf, frames * frame_size);

  if (retval == -1)
    g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                 _("Error writing to sample store swap file: %s"), g_strerror (errno));
  else if (retval < frames * frame_size)
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                 _("Short write to sample store swap file, expected %d but got %" G_GSSIZE_FORMAT),
                 frames * frame_size, retval);

  G_UNLOCK (swap);    // -- unlock swap

  return (retval == frames * frame_size);
}

static void
ipatch_sample_store_swap_init (IpatchSampleStoreSwap *store)
{
}

/* finalization for a swap sample store, keeps track of unused data size */
static void
ipatch_sample_store_swap_finalize (GObject *gobject)
{
  IpatchSampleStoreSwap *store = IPATCH_SAMPLE_STORE_SWAP (gobject);
  SwapRecover *recover;
  guint size;

  size = ipatch_sample_store_get_size_bytes ((IpatchSampleStore *)store);

  if (store->ram_location)                      // Allocated in RAM?
  {
    g_atomic_int_add (&swap_ram_used, -size);   // Subtract size from RAM usage
    g_free (store->ram_location);               // -- free allocated RAM
  }
  else
  {
    recover = ipatch_sample_store_swap_recover_new ();  // ++ alloc recover structure
    recover->size = size;
    recover->location = store->location;

    G_LOCK (swap);      // ++ lock swap_list
    swap_list = g_slist_remove (swap_list, store);      // Remove disk swap store from swap_list
    ipatch_sample_store_swap_recover_add (recover);     // Add the recover segment
    G_UNLOCK (swap);    // -- unlock swap_list

    g_atomic_int_add (&swap_unused_size, size);         // Store allocated in disk swap file, add to unused size
  }

  if (G_OBJECT_CLASS (ipatch_sample_store_swap_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_swap_parent_class)->finalize (gobject);
}

/**
 * ipatch_set_sample_store_swap_file_name:
 * @filename: File name to use for sample swap disk file
 *
 * Set name of sample swap storage file on disk.  Can only be assigned once and
 * should be done prior to any #IpatchSampleStoreSwap objects being created.
 *
 * Since: 1.1.0
 */
void
ipatch_set_sample_store_swap_file_name (const char *filename)
{
  g_return_if_fail (filename != NULL);
  g_return_if_fail (swap_file_name == NULL);

  swap_file_name = g_strdup (filename);
}

/**
 * ipatch_get_sample_store_swap_file_name:
 *
 * Get name of sample swap storage file on disk.
 *
 * Returns: Newly allocated sample store swap file name or %NULL
 *
 * Since: 1.1.0
 */
char *
ipatch_get_sample_store_swap_file_name (void)
{
  return (g_strdup (swap_file_name));   // !! allocate for caller
}

/**
 * ipatch_sample_store_swap_new:
 *
 * Creates a new disk swap sample store.
 *
 * Returns: (type IpatchSampleStoreSwap): New disk swap sample store, cast
 *   as an #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_swap_new (void)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_SWAP, NULL)));
}

/**
 * ipatch_sample_store_swap_close:
 *
 * Close the swap sample store temporary file and delete it.  Should only be called prior to
 * exiting application when no more sample store accesses will occur.
 *
 * Since: 1.1.0
 */
void
ipatch_sample_store_swap_close (void)
{
  G_LOCK (swap);        // ++ lock swap

  if (swap_fd != -1)
  {
    close (swap_fd);
    swap_fd = -1;

    // Just blindly delete the swap file
    if (swap_file_name)
      g_unlink (swap_file_name);
  }

  G_UNLOCK (swap);      // -- unlock swap
}

/**
 * ipatch_get_sample_store_swap_unused_size:
 *
 * Get amount of unused space in the swap file.
 *
 * Returns: Amount of unused data in bytes
 *
 * Since: 1.1.0
 */
int
ipatch_get_sample_store_swap_unused_size (void)
{
  return (g_atomic_int_get (&swap_unused_size));
}

/**
 * ipatch_set_sample_store_swap_max_memory:
 * @size: Maximum amount of RAM to use for swap sample stores (-1 for unlimited)
 *
 * Set maximum RAM memory size to use for samples in swap.  Using RAM increases
 * performance, at the expense of memory use.  Once max RAM usage is exceeded
 * samples will be allocated in sample swap file on disk.
 *
 * Since: 1.1.0
 */
void
ipatch_set_sample_store_swap_max_memory (int size)
{
  g_atomic_int_set (&swap_ram_max, size);
}

/**
 * ipatch_get_sample_store_swap_max_memory:
 *
 * Get maximum RAM memory size to use for samples in swap.
 *
 * Returns: Max sample store swap RAM memory size.
 *
 * Since: 1.1.0
 */
int
ipatch_get_sample_store_swap_max_memory (void)
{
  return (g_atomic_int_get (&swap_ram_max));
}

/**
 * ipatch_sample_store_swap_compact:
 * @err: Location to store error information or %NULL to ignore
 *
 * Compact the sample store swap file by re-writing it to a new file
 * and creating new sample stores to replace the old ones.  This should be
 * done when the unused size (ipatch_get_sample_store_swap_unused_size())
 * exceeds a certain amount.  This occurs when sample stores in the swap file
 * are no longer used, leaving gaps of unused data.  If there is no unused data
 * then nothing is done.
 * NOTE: Swap file will be locked at multi thread sensitive phases of this operation
 * which may cause simultaneous sample operations on swap samples to be delayed.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_compact_sample_store_swap (GError **err)
{
  IpatchSampleStoreSwap *store;
  guint8 *buf;
  char *newname;
  int newfd;
  GArray *position_array;
  GSList *p;
  guint size, ofs, this_size;
  int retval;
  int i;

  g_return_val_if_fail (!err || !*err, FALSE);

  // No unused data? - Return success
  if (ipatch_get_sample_store_swap_unused_size () == 0)
    return (TRUE);

  // Create new swap file to copy existing disk samples to
  newname = g_strconcat (swap_file_name, "_new", NULL); // ++ alloc new file name (same as existing one + _new)
  newfd = g_open (newname, O_RDWR | O_CREAT, 0600);

  if (newfd == -1)
  {
    g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                 _("Failed to open new swap file '%s': %s"), newname, g_strerror (errno));
    g_free (newname);   // -- free newname
    return (FALSE);
  }

  buf = g_malloc (IPATCH_SAMPLE_COPY_BUFFER_SIZE);      // ++ alloc copy buffer

  position_array = g_array_new (FALSE, FALSE, sizeof (guint));  // ++ alloc array

  G_LOCK (swap);        // ++ lock swap

  swap_position = 0;

  for (p = swap_list; p; p = p->next)
  {
    store = (IpatchSampleStoreSwap *)(p->data);
    ipatch_sample_get_size (IPATCH_SAMPLE (store), &size);
    ofs = 0;

    g_array_append_val (position_array, swap_position);

    this_size = IPATCH_SAMPLE_COPY_BUFFER_SIZE;

    while (ofs < size)
    {
      if (size - ofs < IPATCH_SAMPLE_COPY_BUFFER_SIZE)
        this_size = size - ofs;

      swap_position += this_size;

      if (lseek (swap_fd, store->location + ofs, SEEK_SET) == -1)
      {
        g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Error seeking in sample store swap file: %s"), g_strerror (errno));
        goto error;
      }

      ofs += this_size;

      retval = read (swap_fd, buf, this_size);

      if (retval == -1)
      {
        g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Error reading from sample store swap file: %s"), g_strerror (errno));
        goto error;
      }
      else if (retval < this_size)
      {
        g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                     _("Short read from sample store swap file, expected %d but got %d"),
                     this_size, retval);
        goto error;
      }

      retval = write (newfd, buf, this_size);

      if (retval == -1)
      {
        g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Error writing to new sample store swap file: %s"), g_strerror (errno));
        goto error;
      }
      else if (retval < this_size)
      {
        g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                     _("Short write to new sample store swap file, expected %d but got %d"),
                     this_size, retval);
        goto error;
      }
    }
  }

  // Free the recover lists
  g_slist_free_full (swap_recover_list, (GDestroyNotify)ipatch_sample_store_swap_recover_free);
  g_slist_free (swap_recover_loc_list);
  swap_recover_list = NULL;
  swap_recover_loc_list = NULL;

  g_atomic_int_set (&swap_unused_size, 0);      // Set unused size back to 0

  close (swap_fd);
  g_unlink (swap_file_name);    // unlink old file
  swap_fd = newfd;

  // Rename new swap file to swap file name
  if (g_rename (newname, swap_file_name) == -1)
  { // If rename fails for some reason, just leave the new file where it is
    g_warning (_("Failed to rename new swap file from '%s' to '%s'"),
               newname, swap_file_name);
    g_free (swap_file_name);    // -- free swap file name
    swap_file_name = newname;   // !! takes over allocation
    newname = NULL;
  }

  // Fixup locations
  for (i = 0, p = swap_list; i < position_array->len; i++, p = p->next)
  {
    store = (IpatchSampleStoreSwap *)(p->data);
    store->location = g_array_index (position_array, guint, i);
  }

  G_UNLOCK (swap);      // -- unlock swap

  g_free (newname);     // -- free newname
  g_free (buf);         // -- free buffer
  g_array_free (position_array, TRUE);  // -- free array

  return (TRUE);

error:
  G_UNLOCK (swap);      // -- unlock swap
  close (newfd);        // -- close new swap file
  g_unlink (newname);   // -- unlink new swap file
  g_free (newname);     // -- free newname
  g_free (buf);         // -- free buffer
  g_array_free (position_array, TRUE);  // -- free array

  return (FALSE);
}

#ifdef IPATCH_DEBUG
/**
 * ipatch_sample_store_swap_dump: (skip)
 *
 * Dump information about sample swap to stdout for debugging.
 */
void
ipatch_sample_store_swap_dump (void)
{
  IpatchSampleStoreSwap *swap_store;
  SwapRecover *recover;
  GSList *p;

  G_LOCK (swap);                // ++ lock swap

  printf ("Swap file: %s\n", swap_file_name);
  printf ("Pos=%u Unused=%d RamUse=%d RamMax=%d\n", swap_position, swap_unused_size, swap_ram_used, swap_ram_max);
  printf ("\nSwap Samples:\n");

  for (p = swap_list; p; p = p->next)
  {
    int sample_format, sample_rate, loop_type, root_note, fine_tune;
    guint sample_size, loop_start, loop_end;
    char *title;

    swap_store = IPATCH_SAMPLE_STORE_SWAP (p->data);

    g_object_get (swap_store,
                  "title", &title,
                  "sample-size", &sample_size,          // ++ allocate title
                  "sample-format", &sample_format,
                  "sample-rate", &sample_rate,
                  "loop-type", &loop_type,
                  "loop-start", &loop_start,
                  "loop-end", &loop_end,
                  "root-note", &root_note,
                  "fine-tune", &fine_tune,
                  NULL);

    printf ("  Store %p: loc=%u title='%s' size=%u fmt=0x%X rate=%d ltype=%d lstart=%u lend=%u root=%d fine=%d\n",
            swap_store, swap_store->location, title, sample_size, sample_format,
            sample_rate, loop_type, loop_start, loop_end, root_note, fine_tune);

    g_free (title);                                     // -- free title
  }

  printf ("\nRecover Segments:\n");

  for (p = swap_recover_loc_list; p; p = p->next)
  {
    recover = (SwapRecover *)(p->data);
    printf ("%08X: size=%u\n", recover->location, recover->size);
  }

  G_UNLOCK (swap);              // -- unlock swap
}

#endif

