/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* for stat and fstat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include "IpatchFile.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,
  PROP_FILE_NAME
};

#define IPATCH_FILE_FREE_IOFUNCS(file) \
  (ipatch_item_get_flags(file) & IPATCH_FILE_FLAG_FREE_IOFUNCS)

static void ipatch_file_class_init (IpatchFileClass *klass);
static void ipatch_file_init (IpatchFile *file);
static void ipatch_file_finalize (GObject *gobject);
static void ipatch_file_set_property (GObject *object,
				      guint property_id,
				      const GValue *value,
				      GParamSpec *pspec);
static void ipatch_file_get_property (GObject *object,
				      guint property_id,
				      GValue *value,
				      GParamSpec *pspec);
static gboolean ipatch_file_real_set_name (IpatchFile *file,
					   const char *file_name);
static GType ipatch_file_real_identify (IpatchFile *file, gboolean byext,
                                        GError **err);
static GType *type_all_children (GType type, GArray *pass_array);
static gint sort_type_by_identify_order (gconstpointer a, gconstpointer b);

static gboolean ipatch_file_null_open_method (IpatchFileHandle *handle,
					      const char *mode, GError **err);
static GIOStatus ipatch_file_null_read_method (IpatchFileHandle *handle, gpointer buf,
					       guint size, guint *bytes_read,
					       GError **err);
static GIOStatus ipatch_file_null_write_method (IpatchFileHandle *handle,
						gconstpointer buf,
						guint size, GError **err);
static GIOStatus ipatch_file_null_seek_method (IpatchFileHandle *handle, int offset,
					       GSeekType type, GError **err);

/* default methods GIOChannel based methods */
static IpatchFileIOFuncs default_iofuncs =
{
  ipatch_file_default_open_method,
  ipatch_file_default_close_method,
  ipatch_file_default_read_method,
  ipatch_file_default_write_method,
  ipatch_file_default_seek_method,
  ipatch_file_default_getfd_method,
  ipatch_file_default_get_size_method
};

/* null methods (/dev/null like iofuncs) */
static IpatchFileIOFuncs null_iofuncs =
{
  ipatch_file_null_open_method,
  NULL,				/* close method */
  ipatch_file_null_read_method,
  ipatch_file_null_write_method,
  ipatch_file_null_seek_method,
  NULL,				/* get fd method */
  NULL				/* get_size method */
};

G_DEFINE_TYPE (IpatchFile, ipatch_file, IPATCH_TYPE_ITEM);

static IpatchFileHandle *
ipatch_file_handle_duplicate (IpatchFileHandle *handle)
{
  IpatchFileHandle *newhandle;

  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), NULL);

  newhandle = g_slice_new0 (IpatchFileHandle);     /* ++ alloc handle */
  newhandle->file = g_object_ref (handle->file);        /* ++ ref file */

  return (newhandle);
}

static void
ipatch_file_handle_free (IpatchFileHandle *handle)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (IPATCH_IS_FILE (handle->file));

  g_object_unref (handle->file);        /* -- unref file */
  g_slice_free (IpatchFileHandle, handle);
}

GType
ipatch_file_handle_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchFileHandle",
                                         (GBoxedCopyFunc)ipatch_file_handle_duplicate,
                                         (GBoxedFreeFunc)ipatch_file_handle_free);

  return (type);
}

static void
ipatch_file_class_init (IpatchFileClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_file_finalize;
  obj_class->get_property = ipatch_file_get_property;

  item_class->item_set_property = ipatch_file_set_property;

  klass->identify = NULL;

  g_object_class_install_property (obj_class, PROP_FILE_NAME,
			g_param_spec_string ("file-name", "File Name",
					     "File Name",
					     NULL,
					     G_PARAM_READWRITE));
}

static void
ipatch_file_init (IpatchFile *file)
{
  file->iofuncs = &default_iofuncs;
  ipatch_item_clear_flags (file, IPATCH_FILE_FLAG_FREE_IOFUNCS);

  if (G_BYTE_ORDER != G_LITTLE_ENDIAN)
    ipatch_item_set_flags (file, IPATCH_FILE_FLAG_SWAP);
}

static void
ipatch_file_finalize (GObject *gobject)
{
  IpatchFile *file = IPATCH_FILE (gobject);

  IPATCH_ITEM_WLOCK (file);

  /* No handles will be open, since they hold refs on the file */

  /* free iofuncs structure if needed */
  if (file->iofuncs && ipatch_item_get_flags (file)
      & IPATCH_FILE_FLAG_FREE_IOFUNCS)
  {
    g_slice_free (IpatchFileIOFuncs, file->iofuncs);
    file->iofuncs = NULL;
  }

  g_free (file->file_name);
  file->file_name = NULL;

  if (file->iochan)
    g_io_channel_unref (file->iochan);

  IPATCH_ITEM_WUNLOCK (file);

  if (G_OBJECT_CLASS (ipatch_file_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_file_parent_class)->finalize (gobject);
}

static void
ipatch_file_set_property (GObject *object, guint property_id,
			  const GValue *value, GParamSpec *pspec)
{
  IpatchFile *file = IPATCH_FILE (object);

  switch (property_id)
    {
    case PROP_FILE_NAME:
      ipatch_file_real_set_name (file, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_file_get_property (GObject *object, guint property_id,
			  GValue *value, GParamSpec *pspec)
{
  IpatchFile *file = IPATCH_FILE (object);

  switch (property_id)
    {
    case PROP_FILE_NAME:
      g_value_take_string (value, ipatch_file_get_name (file));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ipatch_file_new:
 *
 * Create a new file object
 *
 * Returns: The new file object
 */
IpatchFile *
ipatch_file_new (void)
{
  return (IPATCH_FILE (g_object_new (IPATCH_TYPE_FILE, NULL)));
}

/**
 * ipatch_file_set_name:
 * @file: File object to assign file name to
 * @file_name: File name or %NULL to unset the file name
 *
 * Sets the file name of a file object.  Assigning the file name of an #IpatchFile
 * object is optional, since a file descriptor could be assigned instead,
 * but some subsystems depend on it.
 */
void
ipatch_file_set_name (IpatchFile *file, const char *file_name)
{
  if (ipatch_file_real_set_name (file, file_name))
    g_object_notify (G_OBJECT (file), "file-name");
}

static gboolean
ipatch_file_real_set_name (IpatchFile *file, const char *file_name)
{
  g_return_val_if_fail (IPATCH_IS_FILE (file), FALSE);

  IPATCH_ITEM_WLOCK (file);
  g_free (file->file_name);
  if (file_name) file->file_name = g_strdup (file_name);
  else file->file_name = NULL;
  IPATCH_ITEM_WUNLOCK (file);

  return (TRUE);
}

/**
 * ipatch_file_get_name:
 * @file: File object to get file name from
 *
 * Gets the assigned file name from a file object.
 *
 * Returns: The file name of the file object or %NULL if not set. String
 * should be freed when finished with it.
 */
char *
ipatch_file_get_name (IpatchFile *file)
{
  char *file_name = NULL;

  g_return_val_if_fail (IPATCH_IS_FILE (file), NULL);

  IPATCH_ITEM_RLOCK (file);
  if (file->file_name) file_name = g_strdup (file->file_name);
  IPATCH_ITEM_RUNLOCK (file);

  return (file_name);
}

/**
 * ipatch_file_open:
 * @file: File object to open from a file name.
 * @file_name: Name of file to open or %NULL to use the file object's assigned
 *   file name (in which case it should not be %NULL).
 * @mode: File open mode ("r" for read or "w" for write)
 * @err: Error information
 *
 * Opens a handle to a file object.  If a I/O channel or file descriptor is
 * already assigned (with ipatch_file_assign_fd() or
 * ipatch_file_assign_io_channel()) then it is used instead of opening a file
 * using @file_name or the already assigned #IpatchFile:file-name property.
 *
 * Returns: New file handle or %NULL (in which case @err may be set).  The
 *   returned file handle is not multi-thread safe, but the file can be opened
 *   multiple times for this purpose.
 */
IpatchFileHandle *
ipatch_file_open (IpatchFile *file, const char *file_name, const char *mode,
		  GError **err)
{
  IpatchFileHandle *handle;
  GIOChannel *iochan = NULL;
  char *old_file_name = NULL;
  char *dup_file_name;
  int retval = FALSE;

  g_return_val_if_fail (IPATCH_IS_FILE (file), NULL);
  g_return_val_if_fail (file->iofuncs != NULL, NULL);

  dup_file_name = g_strdup (file_name);         /* ++ dup file name */

  handle = g_slice_new0 (IpatchFileHandle);     /* ++ alloc handle */
  handle->file = file;

  IPATCH_ITEM_WLOCK (file);

  if (log_if_fail (file->iofuncs->open != NULL))
  {
    IPATCH_ITEM_WUNLOCK (file);
    g_slice_free (IpatchFileHandle, handle);    /* -- free handle */
    g_free (dup_file_name);                     /* -- free dup file name */
    return (NULL);
  }

  if (dup_file_name)			/* set file name if supplied */
  {
    old_file_name = file->file_name;
    file->file_name = dup_file_name;    /* !! takes over allocation of dup file name */
  }

  if (file->iochan)
  {
    iochan = g_io_channel_ref (file->iochan);	/* ++ ref io channel */
    handle->iochan = iochan;
  }

  retval = file->iofuncs->open (handle, mode, err);

  IPATCH_ITEM_WUNLOCK (file);

  g_free (old_file_name);       /* -- free old file name */

  if (!retval)
  {
    g_slice_free (IpatchFileHandle, handle);	/* -- free handle */
    if (iochan) g_io_channel_unref (iochan);	/* -- unref iochan */
    return (NULL);
  }

  g_object_ref (file);          /* ++ ref file for handle */
  handle->buf = g_byte_array_new ();

  return (handle);      /* !! caller takes over handle */
}

/**
 * ipatch_file_assign_fd:
 * @file: File object
 * @fd: File descriptor to assign to file object, or -1 to clear it
 * @close_on_finalize: %TRUE if the descriptor should be closed when @file is
 *   finalized, %FALSE to leave it open
 *
 * Assigns a file descriptor to a file, which gets used for calls to
 * ipatch_file_open().  Note that this means multiple opens will use the same
 * file descriptor and will therefore conflict, so it should only be used in the
 * case where the file object is used by a single exclusive handle.
 */
void
ipatch_file_assign_fd (IpatchFile *file, int fd, gboolean close_on_finalize)
{
  GIOChannel *iochan;

  g_return_if_fail (IPATCH_IS_FILE (file));

  iochan = g_io_channel_unix_new (fd); /* ++ ref new io channel */
  g_io_channel_set_close_on_unref (iochan, close_on_finalize);
  g_io_channel_set_encoding (iochan, NULL, NULL);
  ipatch_file_assign_io_channel (file, iochan);
  g_io_channel_unref (iochan);	/* -- unref creator's ref */
}

/**
 * ipatch_file_assign_io_channel:
 * @file: File object
 * @iochan: IO channel to assign to @file or %NULL to clear it
 *
 * Assigns an I/O channel to a file, which gets used for calls to
 * ipatch_file_open().  Note that this means multiple opens will use the same
 * file descriptor and will therefore conflict, so it should only be used in the
 * case where the file object is used by a single exclusive handle.
 */
void
ipatch_file_assign_io_channel (IpatchFile *file, GIOChannel *iochan)
{
  GIOChannel *old_iochan;
  g_return_if_fail (IPATCH_IS_FILE (file));

  if (iochan) g_io_channel_ref (iochan);	/* ++ ref for file */

  IPATCH_ITEM_WLOCK (file);
  old_iochan = file->iochan;
  file->iochan = iochan;
  IPATCH_ITEM_WUNLOCK (file);

  if (old_iochan) g_io_channel_unref (file->iochan);
}

/**
 * ipatch_file_get_io_channel:
 * @handle: File handle
 *
 * Get the glib IO channel object from a file handle. The caller owns a
 * reference to the returned io channel, and it should be unreferenced with
 * g_io_channel_unref() when finished with it.
 *
 * Returns: GIOChannel assigned to the @handle or %NULL if none (some
 * derived #IpatchFile types might not use io channels). Remember to unref it
 * with g_io_channel_unref() when finished.
 */
GIOChannel *
ipatch_file_get_io_channel (IpatchFileHandle *handle)
{
  GIOChannel *iochan;

  g_return_val_if_fail (handle != NULL, NULL);

  if ((iochan = handle->iochan))
    g_io_channel_ref (iochan);

  return (iochan);
}

/**
 * ipatch_file_get_fd:
 * @handle: File handle
 *
 * Get the unix file descriptor associated with a file handle. Not all file
 * handles have a real OS file descriptor.
 *
 * Returns: File descriptor or -1 if not open or failed to get descriptor.
 */
int
ipatch_file_get_fd (IpatchFileHandle *handle)
{
  int fd = -1;

  g_return_val_if_fail (handle != NULL, -1);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), -1);

  if (handle->file->iofuncs && handle->file->iofuncs->getfd)
    fd = handle->file->iofuncs->getfd (handle);

  return (fd);
}

/**
 * ipatch_file_close:
 * @handle: File handle
 *
 * Close a file handle and free it.
 */
void
ipatch_file_close (IpatchFileHandle *handle)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (IPATCH_IS_FILE (handle->file));

  if (handle->file->iofuncs && handle->file->iofuncs->close)
    handle->file->iofuncs->close (handle);

  g_object_unref (handle->file);
  if (handle->buf) g_byte_array_free (handle->buf, TRUE);
  if (handle->iochan) g_io_channel_unref (handle->iochan);
  g_slice_free (IpatchFileHandle, handle);
}

/**
 * ipatch_file_get_position:
 * @handle: File handle
 *
 * Gets the current position in a file handle. Note that this
 * might not be the actual position in the file if the file handle was
 * attached to an already open file or if ipatch_file_update_position() is
 * used to set virtual positions.
 *
 * Returns: Position in file handle.
 */
guint
ipatch_file_get_position (IpatchFileHandle *handle)
{
  g_return_val_if_fail (handle != NULL, 0);
  return (handle->position);
}

/**
 * ipatch_file_update_position:
 * @handle: File handle
 * @offset: Offset to add to the position counter (can be negative)
 *
 * Adds an offset value to the position counter in a file handle. This can
 * be used if one is operating directly on the underlying file descriptor (i.e.,
 * not using the #IpatchFile functions) or to add virtual space to the counter.
 * Adding virtual space is useful when a system uses the position counter to
 * write data (such as the RIFF parser) to act as a place holder for data that
 * isn't actually written (sample data for example).
 */
void
ipatch_file_update_position (IpatchFileHandle *handle, guint offset)
{
  g_return_if_fail (handle != NULL);
  handle->position += offset;
}

/**
 * ipatch_file_read:
 * @handle: File handle
 * @buf: Buffer to read data into
 * @size: Amount of data to read, in bytes.
 * @err: A location to return an error of type GIOChannelError or %NULL.
 *
 * Reads data from a file handle. An end of file encountered while
 * trying to read the specified @size of data is treated as an error.
 * If this is undesirable use ipatch_file_read_eof() instead.
 *
 * Returns: %TRUE on success (the requested @size of data was read), %FALSE
 * otherwise
 */
gboolean
ipatch_file_read (IpatchFileHandle *handle, gpointer buf, guint size, GError **err)
{
  GIOStatus status;

  /* we let ipatch_file_read_eof do checks for us */
  status = ipatch_file_read_eof (handle, buf, size, NULL, err);

  if (status == G_IO_STATUS_EOF)
  {
    if (err && !*err)
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNEXPECTED_EOF,
                   _("Unexpected end of file"));
    return (FALSE);
  }

  return (status == G_IO_STATUS_NORMAL);
}

/**
 * ipatch_file_read_eof:
 * @handle: File handle
 * @buf: Buffer to read data into
 * @size: Amount of data to read, in bytes.
 * @bytes_read: Pointer to store number of bytes actually read or %NULL.
 * @err: A location to return an error of type GIOChannelError or %NULL.
 *
 * Reads data from a file handle. This function does not treat end of file
 * as an error and will return #G_IO_STATUS_EOF with the number of bytes
 * actually read in @bytes_read. Use ipatch_file_read() for convenience to
 * ensure actual number of requested bytes is read.
 *
 * Returns: The status of the operation
 */
GIOStatus
ipatch_file_read_eof (IpatchFileHandle *handle, gpointer buf, guint size,
		      guint *bytes_read, GError **err)
{
  GIOStatus status;
  guint _bytes_read = 0;

  if (bytes_read) *bytes_read = 0;

  g_return_val_if_fail (handle != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs->read != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (size > 0, FALSE);
  g_return_val_if_fail (!err || !*err, G_IO_STATUS_ERROR);

  status = handle->file->iofuncs->read (handle, buf, size, &_bytes_read, err);

  if (bytes_read) *bytes_read = _bytes_read;

  handle->position += _bytes_read;

  return (status);
}

/* Used internally by IpatchFileBuf.  Like ipatch_file_read() but does not
 * update handle->position, since buffered functions do this themselves. */
gboolean
_ipatch_file_read_no_pos_update (IpatchFileHandle *handle, gpointer buf,
                                 guint size, GError **err)
{
  guint _bytes_read = 0;

  return (handle->file->iofuncs->read (handle, buf, size, &_bytes_read, err)
          == G_IO_STATUS_NORMAL);
}

/**
 * ipatch_file_write:
 * @handle: File handle
 * @buf: Buffer of data to write
 * @size: Amount of data to write, in bytes.
 * @err: A location to return an error of type GIOChannelError or %NULL.
 *
 * Writes data to a file object.
 *
 * Returns: The status of the operation
 */
gboolean
ipatch_file_write (IpatchFileHandle *handle, gconstpointer buf, guint size,
		   GError **err)
{
  GIOStatus status;

  g_return_val_if_fail (handle != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs->write != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (buf != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (size > 0, FALSE);
  g_return_val_if_fail (!err || !*err, G_IO_STATUS_ERROR);

  status = handle->file->iofuncs->write (handle, buf, size, err);

  if (status == G_IO_STATUS_NORMAL)
    handle->position += size;

  return (status == G_IO_STATUS_NORMAL);
}

/* Used internally by IpatchFileBuf.  Like ipatch_file_write() but does not
 * update handle->position, since buffered functions do this themselves. */
gboolean
_ipatch_file_write_no_pos_update (IpatchFileHandle *handle, gconstpointer buf,
                                  guint size, GError **err)
{
  return (handle->file->iofuncs->write (handle, buf, size, err) == G_IO_STATUS_NORMAL);
}

/**
 * ipatch_file_seek:
 * @handle: File handle
 * @offset: Offset in bytes to seek from the position specified by @type
 * @type: Position in file to seek from (see g_io_channel_seek_position
 *   for more details, only #G_SEEK_CUR and #G_SEEK_SET allowed).
 * @err: A location to return error info or %NULL.
 *
 * Seek in a file handle. An end of file condition is treated as an error, use
 * ipatch_file_seek_eof() if this is undesirable.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_file_seek (IpatchFileHandle *handle, int offset, GSeekType type, GError **err)
{
  GIOStatus status;

  /* we let ipatch_file_seek_eof do checks for us */
  status = ipatch_file_seek_eof (handle, offset, type, err);

  if (status == G_IO_STATUS_EOF)
  {
    if (err && !*err)
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNEXPECTED_EOF,
                   _("Unexpected end of file"));
    return (FALSE);
  }

  return (status == G_IO_STATUS_NORMAL);
}

/**
 * ipatch_file_seek_eof:
 * @handle: File handle
 * @offset: Offset in bytes to seek from the position specified by @type
 * @type: Position in file to seek from (see g_io_channel_seek_position
 *   for more details, only G_SEEK_CUR and G_SEEK_SET allowed).
 * @err: A location to return error info or %NULL.
 *
 * Seek in a file object. Does not treat end of file as an error, use
 * ipatch_file_seek() for convenience if this is desirable.
 *
 * Returns: The status of the operation
 */
GIOStatus
ipatch_file_seek_eof (IpatchFileHandle *handle, int offset, GSeekType type,
		      GError **err)
{
  GIOStatus status;

  g_return_val_if_fail (handle != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (IPATCH_IS_FILE (handle->file), G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (handle->file->iofuncs->seek != NULL, G_IO_STATUS_ERROR);
  g_return_val_if_fail (type == G_SEEK_CUR || type == G_SEEK_SET,
			G_IO_STATUS_ERROR);
  g_return_val_if_fail (!err || !*err, G_IO_STATUS_ERROR);

  status = handle->file->iofuncs->seek (handle, offset, type, err);

  if (status == G_IO_STATUS_NORMAL)
  {
    if (type == G_SEEK_SET) handle->position = offset;
    else handle->position += offset;
  }

  return (status);
}

/**
 * ipatch_file_get_size:
 * @file: File object to get size of
 * @err: Location to store error information
 *
 * Get the size of a file object.
 *
 * Returns: File size or -1 on error or if operation unsupported by this file
 * object (in which case @err may be set)
 */
int
ipatch_file_get_size (IpatchFile *file, GError **err)
{
  int size = -1;

  g_return_val_if_fail (IPATCH_IS_FILE (file), -1);
  g_return_val_if_fail (file->iofuncs != NULL, -1);
  g_return_val_if_fail (!err || !*err, -1);

  if (file->iofuncs->get_size)	/* has get_size IO function? */
    size = file->iofuncs->get_size (file, err);

  return (size);
}

/**
 * ipatch_file_identify:
 * @file: File object to identify type of
 * @err: Location to store error information
 *
 * Attempts to identify the type of a file using the "identify" method of
 * registered types derived from #IpatchFile.  The #IpatchFile:file-name property
 * should already be assigned.
 *
 * Returns: The first #IpatchFile derived type that had an "identify" method
 * which returned %TRUE, or 0 if unknown file type or error, in which
 * case error information will be stored in @err provided its not %NULL.
 */
GType
ipatch_file_identify (IpatchFile *file, GError **err)
{
  return (ipatch_file_real_identify (file, FALSE, err));
}

/**
 * ipatch_file_identify_by_ext:
 * @file: File object to identify type of
 *
 * Attempts to identify the type of a file using the "identify" method of
 * registered types derived from #IpatchFile.  The #IpatchFile:file-name property
 * should already be assigned.  Like ipatch_file_identify() but identifies a
 * file by its file name extension only.
 *
 * Returns: The first #IpatchFile derived type that had an "identify" method
 * which returned %TRUE, or 0 if unknown file type or error, in which
 * case error information will be stored in @err provided its not %NULL.
 */
GType
ipatch_file_identify_by_ext (IpatchFile *file)
{
  return (ipatch_file_real_identify (file, TRUE, NULL));
}

static GType
ipatch_file_real_identify (IpatchFile *file, gboolean byext, GError **err)
{
  IpatchFileHandle *handle = NULL;
  IpatchFileClass *file_class;
  GType *children, *p;
  GType found = 0;
  GError *local_err = NULL;

  g_return_val_if_fail (IPATCH_IS_FILE (file), 0);
  g_return_val_if_fail (file->file_name != NULL, 0);

  if (!byext)
  {
    /* Open a handle to the file */
    handle = ipatch_file_open (file, NULL, "r", err);
    if (!handle) return (0);
  }

  children = type_all_children (IPATCH_TYPE_FILE, NULL);

  for (p = children; *p; p++)
  {
    file_class = g_type_class_ref (*p);
    if (!file_class) continue;

    if (file_class->identify)
    {
      if (!file_class->identify (file, handle, &local_err))
      {
        if (local_err)
        {
          g_propagate_error (err, local_err);
          g_type_class_unref (file_class);
          if (handle) ipatch_file_close (handle);
          return (0);
        }
      }
      else found = *p;

      if (handle) ipatch_file_seek (handle, 0, G_SEEK_SET, NULL);
    }

    g_type_class_unref (file_class);
    if (found != 0) break;
  }

  if (handle) ipatch_file_close (handle);

  return (found);
}

static GType *
type_all_children (GType type, GArray *pass_array)
{
  static GType *types = NULL;
  GArray *array = pass_array;
  GType *children;
  int i;

  if (types) return (types);

  if (!array) array = g_array_new (TRUE, FALSE, sizeof (GType));        /* Zero terminated */

  children = g_type_children (type, NULL);
  if (children)
    {
      for (i = 0; children[i]; i++)
	{
	  type_all_children (children[i], array);
	  g_array_append_val (array, children[i]);
	}
      g_free (children);
    }

  if (!pass_array)		/* last iteration? */
    {
      types = (array->len > 0) ? (GType *)(array->data) : NULL;
      g_array_sort (array, sort_type_by_identify_order);
      g_array_free (array, FALSE);
      return (types);
    }
  else return (NULL);
}

/* Sort IpatchFile types by their identify_order class field (largest value first order) */
static gint
sort_type_by_identify_order (gconstpointer a, gconstpointer b)
{
  const GType *typea = a, *typeb = b;
  IpatchFileClass *klassa, *klassb;
  gint retval;

  klassa = g_type_class_ref (*typea);   /* ++ ref class */
  klassb = g_type_class_ref (*typeb);   /* ++ ref class */

  retval = klassb->identify_order - klassa->identify_order;

  g_type_class_unref (klassa);          /* -- unref class */
  g_type_class_unref (klassb);          /* -- unref class */

  return (retval);
}

/**
 * ipatch_file_identify_open:
 * @file_name: File name to identify and open
 * @err: Location to store error of type GIOChannelError
 *
 * A convenience function which calls ipatch_file_identify() to determine the
 * file type of @file_name. If the type is identified a new file object, of the
 * identified type, is created and the file is opened with
 * ipatch_file_open() in read mode.
 *
 * Returns: The new opened handle of the identified type or %NULL if unable to
 *   identify.  Caller should free the handle with ipatch_file_close() when done
 *   using it, at which point the parent #IpatchFile will be destroyed if no other
 *   reference is held (by calling ipatch_file_handle_get_file() for example).
 */
IpatchFileHandle *
ipatch_file_identify_open (const char *file_name, GError **err)
{
  IpatchFileHandle *handle;
  IpatchFile *file;
  GType file_type;

  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  file = ipatch_file_new ();	/* ++ ref new file */
  ipatch_file_set_name (file, file_name);
  file_type = ipatch_file_identify (file, err);
  g_object_unref (file);        /* -- unref file */

  if (file_type == 0) return (NULL);

  file = g_object_new (file_type, NULL);        /* ++ ref file */
  handle = ipatch_file_open (file, file_name, "r", err);       /* ++ alloc handle */
  g_object_unref (file);        /* -- unref file (handle holds a ref) */

  return (handle);      /* !! caller takes over handle */
}

/**
 * ipatch_file_set_little_endian:
 * @file: File object
 *
 * Sets the file object to little endian mode (the default mode).
 * If the system is big endian, byte swapping will be enabled
 * (see IPATCH_FILE_SWAPxx macros). The endian mode affects buffered
 * read and write functions that operate on multi-byte integers.
 */
void
ipatch_file_set_little_endian (IpatchFile *file)
{
  g_return_if_fail (IPATCH_IS_FILE (file));
  
  IPATCH_ITEM_WLOCK (file);

  ipatch_item_clear_flags (file, IPATCH_FILE_FLAG_BIG_ENDIAN);

  if (G_BYTE_ORDER != G_LITTLE_ENDIAN)
    ipatch_item_set_flags (file, IPATCH_FILE_FLAG_SWAP);

  IPATCH_ITEM_WUNLOCK (file);
}

/**
 * ipatch_file_set_big_endian:
 * @file: File object
 *
 * Sets the file object to big endian mode (the default is little endian).
 * If the system is little endian, byte swapping will be enabled
 * (see IPATCH_FILE_SWAPxx macros). The endian mode affects buffered
 * read and write functions that operate on multi-byte integers.
 */
void
ipatch_file_set_big_endian (IpatchFile *file)
{
  g_return_if_fail (IPATCH_IS_FILE (file));
  
  IPATCH_ITEM_WLOCK (file);

  ipatch_item_set_flags (file, IPATCH_FILE_FLAG_BIG_ENDIAN);

  if (G_BYTE_ORDER != G_BIG_ENDIAN)
    ipatch_item_set_flags (file, IPATCH_FILE_FLAG_SWAP);

  IPATCH_ITEM_WUNLOCK (file);
}

/**
 * ipatch_file_set_iofuncs_static:
 * @file: File object
 * @funcs: Static IO functions structure or %NULL to set to defaults
 *
 * Sets the input/output functions of a file object using a statically
 * allocated (guaranteed to exist for lifetime of @file) functions structure.
 * Setting these functions allows one to write custom data sources or hook
 * into other file functions.
 */
void
ipatch_file_set_iofuncs_static (IpatchFile *file, IpatchFileIOFuncs *funcs)
{
  g_return_if_fail (IPATCH_IS_FILE (file));

  IPATCH_ITEM_WLOCK (file);

  if (IPATCH_FILE_FREE_IOFUNCS (file))
    g_slice_free (IpatchFileIOFuncs, file->iofuncs);

  file->iofuncs = funcs ? funcs : &default_iofuncs;
  ipatch_item_clear_flags (file, IPATCH_FILE_FLAG_FREE_IOFUNCS);

  IPATCH_ITEM_WUNLOCK (file);
}

/**
 * ipatch_file_set_iofuncs:
 * @file: File object
 * @funcs: IO functions structure or %NULL to set to defaults
 *
 * Sets the input/output functions of a file object. The @funcs
 * structure is duplicated so as not to use the original, see
 * ipatch_file_set_iofuncs_static() for using a static structure.
 * Setting these functions allows one to write custom data sources or
 * hook into other file functions.
 */
void
ipatch_file_set_iofuncs (IpatchFile *file, const IpatchFileIOFuncs *funcs)
{
  IpatchFileIOFuncs *dupfuncs = NULL;

  g_return_if_fail (IPATCH_IS_FILE (file));

  if (funcs)			/* duplicate functions structure */
  {
    dupfuncs = g_slice_new (IpatchFileIOFuncs);
    *dupfuncs = *funcs;
  }

  IPATCH_ITEM_WLOCK (file);

  if (IPATCH_FILE_FREE_IOFUNCS (file))
    g_slice_free (IpatchFileIOFuncs, file->iofuncs);

  file->iofuncs = dupfuncs ? dupfuncs : &default_iofuncs;

  if (dupfuncs) ipatch_item_set_flags (file, IPATCH_FILE_FLAG_FREE_IOFUNCS);
  else ipatch_item_clear_flags (file, IPATCH_FILE_FLAG_FREE_IOFUNCS);

  IPATCH_ITEM_WUNLOCK (file);
}

/**
 * ipatch_file_get_iofuncs:
 * @file: File object
 * @out_funcs: Location to store current IO functions to
 *
 * Get the current IO functions from a file object. The function pointers
 * are stored in a user supplied structure pointed to by @out_funcs.
 */
void
ipatch_file_get_iofuncs (IpatchFile *file, IpatchFileIOFuncs *out_funcs)
{
  g_return_if_fail (IPATCH_IS_FILE (file));
  g_return_if_fail (out_funcs != NULL);

  IPATCH_ITEM_RLOCK (file);
  *out_funcs = *file->iofuncs;
  IPATCH_ITEM_RUNLOCK (file);
}

/**
 * ipatch_file_set_iofuncs_null:
 * @file: File object
 *
 * Sets the I/O functions of a file object to /dev/null like methods.
 * Reading from the file will return 0s, writing/seeking will do nothing.
 */
void
ipatch_file_set_iofuncs_null (IpatchFile *file)
{
  g_return_if_fail (IPATCH_IS_FILE (file));
  ipatch_file_set_iofuncs_static (file, &null_iofuncs);
}


/**
 * ipatch_file_default_open_method:
 * @handle: File handle
 * @mode: File open mode
 * @err: Error info
 *
 * Default "open" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_file_default_open_method (IpatchFileHandle *handle, const char *mode,
				 GError **err)
{
  if (handle->iochan)	     /* io channel has been explicitly set? */
  {
    g_io_channel_set_encoding (handle->iochan, NULL, NULL);
    return (TRUE);
  }

  g_return_val_if_fail (mode != NULL, FALSE);
  g_return_val_if_fail (handle->file->file_name != NULL, FALSE);

  handle->iochan = g_io_channel_new_file (handle->file->file_name, mode, err);

  if (handle->iochan)
  {
    g_io_channel_set_encoding (handle->iochan, NULL, NULL);
    return (TRUE);
  }
  else return (FALSE);
}

/**
 * ipatch_file_default_close_method:
 * @handle: File handle
 *
 * Default "close" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions.
 */
void
ipatch_file_default_close_method (IpatchFileHandle *handle)
{
  g_return_if_fail (handle->iochan != NULL);

  g_io_channel_shutdown (handle->iochan, TRUE, NULL);
  g_io_channel_unref (handle->iochan);

  handle->iochan = NULL;
}

/**
 * ipatch_file_default_read_method:
 * @handle: File handle
 * @buf: Buffer to store data to
 * @size: Size of data
 * @bytes_read: Number of bytes actually read
 * @err: Error info
 *
 * Default "read" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions.
 *
 * Returns: The status of the operation.
 */
GIOStatus
ipatch_file_default_read_method (IpatchFileHandle *handle, gpointer buf,
                                 guint size, guint *bytes_read, GError **err)
{
  gsize _bytes_read = 0;
  GIOStatus status;

  g_return_val_if_fail (handle->iochan != NULL, G_IO_STATUS_ERROR);

  status = g_io_channel_read_chars (handle->iochan, buf, size,
				    &_bytes_read, err);
  *bytes_read = _bytes_read;

  return (status);
}

/**
 * ipatch_file_default_write_method:
 * @handle: File handle
 * @buf: Buffer to read data from
 * @size: Size of data
 * @err: Error info
 *
 * Default "write" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions.
 *
 * Returns: The status of the operation.
 */
GIOStatus
ipatch_file_default_write_method (IpatchFileHandle *handle, gconstpointer buf,
				  guint size, GError **err)
{
  GIOStatus status;

  g_return_val_if_fail (handle->iochan != NULL, G_IO_STATUS_ERROR);

  status = g_io_channel_write_chars (handle->iochan, buf, size, NULL, err);

  return (status);
}

/**
 * ipatch_file_default_seek_method:
 * @handle: File handle
 * @offset: Offset (depends on seek @type)
 * @type: Seek type
 * @err: Error info
 *
 * Default "seek" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions.
 *
 * Returns: The status of the operation.
 */
GIOStatus
ipatch_file_default_seek_method (IpatchFileHandle *handle, int offset,
                                 GSeekType type, GError **err)
{
  g_return_val_if_fail (handle->iochan != NULL, G_IO_STATUS_ERROR);

  return (g_io_channel_seek_position (handle->iochan, offset, type, err));
}

/**
 * ipatch_file_default_getfd_method:
 * @handle: File handle
 *
 * Default "getfd" method for #IpatchFileIOFuncs. Useful when overriding only
 * some I/O functions. This method gets a unix file descriptor for the given
 * file object, it is an optional method.
 *
 * Returns: Unix file descriptor or -1 if no file descriptor or error.
 */
int
ipatch_file_default_getfd_method (IpatchFileHandle *handle)
{
  return (handle->iochan ? g_io_channel_unix_get_fd (handle->iochan) : -1);
}

/**
 * ipatch_file_default_get_size_method:
 * @file: File object
 * @err: Error info
 *
 * Default get file size method, which is optional.
 *
 * Returns: File size or -1 on error (in which case @err may be set).
 */
int
ipatch_file_default_get_size_method (IpatchFile *file, GError **err)
{
  struct stat info;

  if (file->file_name)
  {
    if (g_stat (file->file_name, &info) != 0)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Error during call to stat(\"%s\"): %s"),
                   file->file_name, g_strerror (errno));
      return (-1);
    }

    return (info.st_size);
  }
  else return (-1);
}

/* ---------------------------------------------------------------
 * NULL file iofunc methods (like /dev/null)
 * --------------------------------------------------------------- */

static gboolean
ipatch_file_null_open_method (IpatchFileHandle *handle, const char *mode,
                              GError **err)
{
  return (TRUE);
}

static GIOStatus
ipatch_file_null_read_method (IpatchFileHandle *handle, gpointer buf, guint size,
			      guint *bytes_read, GError **err)
{
  memset (buf, 0, size);
  *bytes_read = size;
  return (G_IO_STATUS_NORMAL);
}

static GIOStatus
ipatch_file_null_write_method (IpatchFileHandle *handle, gconstpointer buf,
			       guint size, GError **err)
{
  return (G_IO_STATUS_NORMAL);
}

static GIOStatus
ipatch_file_null_seek_method (IpatchFileHandle *handle, int offset, GSeekType type,
			      GError **err)
{
  return (G_IO_STATUS_NORMAL);
}
