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
 * SECTION: IpatchBase
 * @short_description: Base instrument file object type
 * @see_also: 
 * @stability: Stable
 *
 * Defines an abstract object type which is used as the basis of instrument
 * files, such as #IpatchSF2, #IpatchDLS, etc.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include "IpatchBase.h"
#include "IpatchConverter.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "util.h"
#include "ipatch_priv.h"

enum {
  PROP_0,
  PROP_CHANGED,
  PROP_SAVED,
  PROP_FILENAME,
  PROP_FILE
};


static void ipatch_base_finalize (GObject *gobject);
static void ipatch_base_set_property (GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec);
static void ipatch_base_get_property (GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec);
static void ipatch_base_real_set_file (IpatchBase *base, IpatchFile *file);
static void ipatch_base_real_set_file_name (IpatchBase *base,
					    const char *file_name);
static gboolean ipatch_base_real_save (IpatchBase *base, const char *filename,
                                       gboolean save_a_copy, GError **err);

/* private var used by IpatchItem, for fast "changed" property notifies */
GParamSpec *ipatch_base_pspec_changed;

/* cached parameter specs to speed up prop notifies */
static GParamSpec *file_pspec;
static GParamSpec *file_name_pspec;

G_DEFINE_ABSTRACT_TYPE (IpatchBase, ipatch_base, IPATCH_TYPE_CONTAINER);


/**
 * ipatch_base_type_get_mime_type:
 * @base_type: #IpatchBase derived type to get mime type of
 *
 * Get the mime type of the file type associated with the given base patch file type.
 *
 * Returns: Mime type or NULL if none assigned for this base type.
 *   Free the string when finished with it.
 *
 * Since: 1.1.0
 */
char *
ipatch_base_type_get_mime_type (GType base_type)
{
  const IpatchConverterInfo *info;
  char *mime_type;

  info = ipatch_lookup_converter_info (0, base_type, IPATCH_TYPE_FILE);
  if (!info) return (NULL);
  ipatch_type_get (info->dest_type, "mime-type", &mime_type, NULL);

  return (mime_type);
}

static void
ipatch_base_class_init (IpatchBaseClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  item_class->item_set_property = ipatch_base_set_property;
  obj_class->get_property = ipatch_base_get_property;
  obj_class->finalize = ipatch_base_finalize;

  ipatch_base_pspec_changed =
    g_param_spec_boolean ("changed", "Changed", "Changed Flag",
			  TRUE, G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE_CHANGE
			  | IPATCH_PARAM_NO_SAVE);
  g_object_class_install_property (obj_class, PROP_CHANGED,
				   ipatch_base_pspec_changed);

  g_object_class_install_property (obj_class, PROP_SAVED,
		    g_param_spec_boolean ("saved", "Saved", "Been Saved Flag",
					  FALSE, G_PARAM_READWRITE
					  | IPATCH_PARAM_NO_SAVE_CHANGE
					  | IPATCH_PARAM_NO_SAVE));
  file_name_pspec = g_param_spec_string ("file-name", "File Name",
					 "File Name", "untitled",
					 G_PARAM_READWRITE
                                         | IPATCH_PARAM_NO_SAVE_CHANGE);
  g_object_class_install_property (obj_class, PROP_FILENAME, file_name_pspec);

  file_pspec = g_param_spec_object ("file", "File", "File Object",
				    IPATCH_TYPE_FILE,
				    G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE
				    | IPATCH_PARAM_HIDE
                                    | IPATCH_PARAM_NO_SAVE_CHANGE);
  g_object_class_install_property (obj_class, PROP_FILE, file_pspec);
}

static void
ipatch_base_init (IpatchBase *base)
{
}

/* function called when a patch is being destroyed */
static void
ipatch_base_finalize (GObject *gobject)
{
  IpatchBase *base = IPATCH_BASE (gobject);

  IPATCH_ITEM_WLOCK (base);

  if (base->file) ipatch_file_unref_from_object (base->file, gobject);  // -- unref file from object
  base->file = NULL;

  IPATCH_ITEM_WUNLOCK (base);

  if (G_OBJECT_CLASS (ipatch_base_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_base_parent_class)->finalize (gobject);
}

static void
ipatch_base_set_property (GObject *object, guint property_id,
			  const GValue *value, GParamSpec *pspec)
{
  IpatchBase *base = IPATCH_BASE (object);

  switch (property_id)
    {
    case PROP_CHANGED:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (base), IPATCH_BASE_CHANGED);
      else ipatch_item_clear_flags (IPATCH_ITEM (base), IPATCH_BASE_CHANGED);
      break;
    case PROP_SAVED:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (base), IPATCH_BASE_SAVED);
      else ipatch_item_clear_flags (IPATCH_ITEM (base), IPATCH_BASE_SAVED);
      break;
    case PROP_FILENAME:
      ipatch_base_real_set_file_name (base, g_value_get_string (value));
      break;
    case PROP_FILE:
      ipatch_base_real_set_file (base, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_base_get_property (GObject *object, guint property_id,
			  GValue *value, GParamSpec *pspec)
{
  IpatchBase *base;

  g_return_if_fail (IPATCH_IS_BASE (object));
  base = IPATCH_BASE (object);

  switch (property_id)
    {
    case PROP_CHANGED:
      g_value_set_boolean (value,
			   ipatch_item_get_flags (IPATCH_ITEM (base))
			   & IPATCH_BASE_CHANGED);
      break;
    case PROP_SAVED:
      g_value_set_boolean (value,
			   ipatch_item_get_flags (IPATCH_ITEM (base))
			   & IPATCH_BASE_SAVED);
      break;
    case PROP_FILENAME:
      g_value_take_string (value, ipatch_base_get_file_name (base));
      break;
    case PROP_FILE:
      g_value_take_object (value, ipatch_base_get_file (base));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ipatch_base_set_file:
 * @base: Patch base object to set file object of
 * @file: File object
 *
 * Sets the file object associated with a patch.
 */
void
ipatch_base_set_file (IpatchBase *base, IpatchFile *file)
{
  GValue value = { 0 }, oldval = { 0 };

  g_return_if_fail (IPATCH_IS_BASE (base));
  g_return_if_fail (IPATCH_IS_FILE (file));

  g_value_init (&value, IPATCH_TYPE_FILE);
  g_value_set_object (&value, file);

  ipatch_item_get_property_fast ((IpatchItem *)base, file_pspec, &oldval);
  ipatch_base_real_set_file (base, file);
  ipatch_item_prop_notify ((IpatchItem *)base, file_pspec, &value, &oldval);

  g_value_unset (&value);
  g_value_unset (&oldval);
}

static void
ipatch_base_real_set_file (IpatchBase *base, IpatchFile *file)
{
  GValue value = { 0 }, oldval = { 0 };
  IpatchFile *oldfile;

  ipatch_file_ref_from_object (file, (GObject *)base);                          // ++ ref new file from object

  IPATCH_ITEM_WLOCK (base);
  oldfile = base->file;
  base->file = file;
  IPATCH_ITEM_WUNLOCK (base);

  g_value_init (&oldval, G_TYPE_STRING);

  if (oldfile)
  {
    g_value_take_string (&oldval, ipatch_file_get_name (oldfile));
    ipatch_file_unref_from_object (oldfile, (GObject *)base);        // -- remove reference to old file
  }

  g_value_init (&value, G_TYPE_STRING);
  g_value_take_string (&value, ipatch_file_get_name (file));

  // Notify file-name property change as well
  ipatch_item_prop_notify ((IpatchItem *)base, file_name_pspec, &value, &oldval);

  g_value_unset (&value);
  g_value_unset (&oldval);
}

/**
 * ipatch_base_get_file:
 * @base: Patch base object to get file object from
 *
 * Get the file object associated with a patch base object. Caller owns a
 * reference to the returned file object and it should be unrefed when
 * done with it.
 *
 * Returns: (transfer full): File object or %NULL if not set.
 *   Remember to unref it when done with it.
 */
IpatchFile *
ipatch_base_get_file (IpatchBase *base)
{
  IpatchFile *file;

  g_return_val_if_fail (IPATCH_IS_BASE (base), NULL);

  IPATCH_ITEM_RLOCK (base);
  file = base->file;
  if (file) g_object_ref (file);
  IPATCH_ITEM_RUNLOCK (base);

  return (file);
}

/**
 * ipatch_base_set_file_name:
 * @base: Patch base object to set file name of
 * @file_name: Path and name to set filename to
 *
 * Sets the file name of the file object in a patch base object. File object
 * should have been set before calling this function (otherwise request is
 * silently ignored). A convenience function as one could get the file object
 * and set it directly.
 */
void
ipatch_base_set_file_name (IpatchBase *base, const char *file_name)
{
  GValue value = { 0 }, oldval = { 0 };

  g_return_if_fail (IPATCH_IS_BASE (base));

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, file_name);

  ipatch_item_get_property_fast ((IpatchItem *)base, file_name_pspec, &oldval);
  ipatch_base_real_set_file_name (base, file_name);
  ipatch_item_prop_notify ((IpatchItem *)base, file_name_pspec, &value, &oldval);

  g_value_unset (&value);
  g_value_unset (&oldval);
}

/* the real set file name routine, user routine does a notify */
static void
ipatch_base_real_set_file_name (IpatchBase *base, const char *file_name)
{
  IPATCH_ITEM_RLOCK (base);
  if (!base->file)		/* silently fail */
    {
      IPATCH_ITEM_RUNLOCK (base);
      return;
    }
  ipatch_file_set_name (base->file, file_name);
  IPATCH_ITEM_RUNLOCK (base);
}

/**
 * ipatch_base_get_file_name:
 * @base: Base patch item to get file name from.
 *
 * Get the file name of the file object in a base patch item. A convenience
 * function.
 *
 * Returns: New allocated file name or %NULL if not set or file object
 *   not set. String should be freed when finished with it.
 */
char *
ipatch_base_get_file_name (IpatchBase *base)
{
  char *file_name = NULL;

  g_return_val_if_fail (IPATCH_IS_BASE (base), NULL);

  IPATCH_ITEM_RLOCK (base);
  if (base->file)
    file_name = ipatch_file_get_name (base->file);
  IPATCH_ITEM_RUNLOCK (base);

  return (file_name);
}

/**
 * ipatch_base_find_unused_midi_locale:
 * @base: Patch base object
 * @bank: (inout): MIDI bank number
 * @program: (inout): MIDI program number
 * @exclude: (nullable): Child of @base with MIDI locale to exclude or %NULL
 * @percussion: Set to %TRUE to find a free percussion MIDI locale
 *
 * Finds an unused MIDI locale (bank:program number pair) in a patch
 * base object. The way in which MIDI bank and program numbers are
 * used is implementation dependent. Percussion instruments often
 * affect the bank parameter (for example SoundFont uses bank 128 for
 * percussion presets).  On input the @bank and @program parameters
 * set the initial locale to start from (set to 0:0 to find the first
 * free value). If the @percussion parameter is set it may affect
 * @bank, if its not set, bank will not be modified (e.g., if bank is
 * a percussion value it will be used). The exclude parameter can be
 * set to a child item of @base to exclude from the list of "used"
 * locales (useful when making an item unique that is already parented
 * to @base).  On return @bank and @program will be set to an unused
 * MIDI locale based on the input criteria.
 */
void
ipatch_base_find_unused_midi_locale (IpatchBase *base, int *bank,
				     int *program, const IpatchItem *exclude,
				     gboolean percussion)
{
  IpatchBaseClass *klass;

  g_return_if_fail (IPATCH_IS_BASE (base));
  g_return_if_fail (bank != NULL);
  g_return_if_fail (program != NULL);

  *bank = 0;
  *program = 0;

  klass = IPATCH_BASE_GET_CLASS (base);
  if (klass && klass->find_unused_locale)
    klass->find_unused_locale (base, bank, program, exclude, percussion);
}

/**
 * ipatch_base_find_item_by_midi_locale:
 * @base: Patch base object
 * @bank: MIDI bank number of item to search for
 * @program: MIDI program number of item to search for
 *
 * Find a child object in a base patch object which matches the given MIDI
 * locale (@bank and @program numbers).
 *
 * Returns: (transfer full): The item or %NULL if not found.  The caller owns a reference to the
 *   returned object, and is responsible for unref'ing when finished.
 */
IpatchItem *
ipatch_base_find_item_by_midi_locale (IpatchBase *base, int bank, int program)
{
  IpatchBaseClass *klass;

  g_return_val_if_fail (IPATCH_IS_BASE (base), NULL);

  klass = IPATCH_BASE_GET_CLASS (base);
  if (klass && klass->find_item_by_locale)
    return (klass->find_item_by_locale (base, bank, program));
  else return (NULL);
}

/* GFunc used by g_list_foreach() to remove created sample stores */
static void
remove_created_stores (gpointer data, gpointer user_data)
{
  IpatchSampleStore *store = data;
  IpatchSampleData *sampledata;

  sampledata = (IpatchSampleData *)ipatch_item_get_parent ((IpatchItem *)store);        // ++ ref parent sampledata
  if (sampledata) ipatch_sample_data_remove (sampledata, store);
  g_object_unref (sampledata);          // -- unref sampledata
}

/**
 * ipatch_base_save:
 * @base: Base item to save
 * @err: Location to store error info or %NULL
 *
 * Save a patch item to the assigned filename or file object.  This function handles
 * saving over an existing file and migrates sample stores as needed.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_base_save (IpatchBase *base, GError **err)
{
  return ipatch_base_real_save (base, NULL, FALSE, err);
}

/**
 * ipatch_base_save_to_filename:
 * @base: Base item to save
 * @filename: (nullable): New file name to save to or %NULL to use current one
 * @err: Location to store error info or %NULL
 *
 * Save a patch item to a file.  This function handles saving over an existing
 * file and migrates sample stores as needed.  It is an error to try to save
 * over an open file that is not owned by @base.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_base_save_to_filename (IpatchBase *base, const char *filename, GError **err)
{
  return ipatch_base_real_save (base, filename, FALSE, err);
}

/**
 * ipatch_base_save_a_copy:
 * @base: Base item to save
 * @filename: File name to save a copy to
 * @err: Location to store error info or %NULL
 *
 * Save a patch item to a file name.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_base_save_a_copy (IpatchBase *base, const char *filename, GError **err)
{
  return ipatch_base_real_save (base, filename, TRUE, err);
}

/*
 * ipatch_base_real_save:
 * @base: Base item to save
 * @filename: New file name to save to or %NULL to use current one
 * @save_a_copy: If TRUE then new file object will not be assigned
 * @err: Location to store error info or %NULL
 *
 * Save a patch item to a file.  This function handles saving over an existing
 * file and migrates sample stores as needed.  It is an error to try to save
 * over an open file that is not owned by @base though.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
static gboolean
ipatch_base_real_save (IpatchBase *base, const char *filename, gboolean save_a_copy, GError **err)
{
  const IpatchConverterInfo *info;
  IpatchFile *lookup_file, *newfile = NULL, *oldfile = NULL;
  char *tmp_fname = NULL, *abs_fname = NULL, *base_fname = NULL;
  IpatchConverter *converter;
  gboolean tempsave = FALSE;    // Set to TRUE if writing to a temp file first, before replacing a file
  GError *local_err = NULL;
  IpatchList *created_stores = NULL;
  int tmpfd;

  g_return_val_if_fail (IPATCH_IS_BASE (base), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  g_object_get (base, "file", &oldfile, NULL);          // ++ ref old file (if any)

  /* Check if file name specified would overwrite another open file */
  if (filename)
  {
    abs_fname = ipatch_util_abs_filename (filename);    // ++ allocate absolute filename
    lookup_file = ipatch_file_pool_lookup (abs_fname);  // ++ ref file matching filename
    if (lookup_file) g_object_unref (lookup_file);      // -- unref file (we only need the pointer value)

    if (lookup_file && lookup_file != oldfile)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_BUSY,
                   _("Refusing to save over other open file '%s'"), abs_fname);
      goto error;
    }
  }

  if (oldfile) g_object_get (base, "file-name", &base_fname, NULL);     // ++ allocate base file name

  // Write to temporary file if saving over or new file name exists
  tempsave = !abs_fname || (base_fname && strcmp (abs_fname, base_fname) == 0)
    || g_file_test (abs_fname, G_FILE_TEST_EXISTS);

  /* if no filename specified try to use current one */
  if (!abs_fname)
  {
    if (!base_fname)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                   _("File name not supplied and none assigned"));
      goto error;
    }

    abs_fname = base_fname;     // !! abs_fname takes over base_fname
    base_fname = NULL;
  }
  else g_free (base_fname);     // -- free base file name

  /* Find a converter from base object to file */
  info = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (base), IPATCH_TYPE_FILE);

  if (!info)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNSUPPORTED,
                 _("Saving object of type '%s' to file '%s' not supported"),
                 g_type_name (G_OBJECT_TYPE (base)), abs_fname);
    goto error;
  }

  if (tempsave) // Saving to a temporary file?
  {
    tmp_fname = g_strconcat (abs_fname, "_tmpXXXXXX", NULL);         // ++ alloc temporary file name

    // open temporary file in same directory as destination
    if ((tmpfd = g_mkstemp (tmp_fname)) == -1)
    {
      g_set_error (err, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unable to open temp file '%s' for writing: %s"),
                   tmp_fname, g_strerror (errno));
      goto error;
    }

    newfile = IPATCH_FILE (g_object_new (info->dest_type, "file-name", tmp_fname, NULL));       /* ++ ref new file */
    ipatch_file_assign_fd (newfile, tmpfd, TRUE);         /* Assign file descriptor and set close on finalize */
  }
  else  // Not replacing a file, just save it directly without using a temporary file
    newfile = IPATCH_FILE (g_object_new (info->dest_type, "file-name", abs_fname, NULL));       /* ++ ref new file */

  // ++ Create new converter and set create-stores property if not "save a copy" mode
  converter = IPATCH_CONVERTER (g_object_new (info->conv_type, "create-stores", !save_a_copy, NULL));

  ipatch_converter_add_input (converter, G_OBJECT (base));
  ipatch_converter_add_output (converter, G_OBJECT (newfile));

  /* attempt to save patch file */
  if (!ipatch_converter_convert (converter, err))
  {
    g_object_unref (converter);                 // -- unref converter
    goto error;
  }

  // If "create-stores" was set, then we get the list of stores in case of error, so new stores can be removed
  if (!save_a_copy)
  {
    IpatchList *out_list = ipatch_converter_get_outputs (converter);    // ++ ref output object list
    created_stores = (IpatchList *)g_list_nth_data (out_list->items, 1);
    if (created_stores) g_object_ref (created_stores);          // ++ ref created stores list
    g_object_unref (out_list);          // -- unref output object list
  }

  g_object_unref (converter);                   // -- unref converter

  if (tempsave)
    ipatch_file_assign_fd (newfile, -1, FALSE); // Unset file descriptor of file (now using file name), closes file descriptor

  // Migrate samples
  if (!save_a_copy)
  {
    if (!ipatch_migrate_file_sample_data (oldfile, newfile, abs_fname, IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED
                                          | IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE | (tempsave ? IPATCH_SAMPLE_DATA_MIGRATE_REPLACE : 0), err))
      goto error;

    ipatch_base_set_file (IPATCH_BASE (base), newfile); // Assign new file to base object
  }
  else if (tempsave && !ipatch_file_rename (newfile, abs_fname, err))   // If "save a copy" mode and saved to a temporary file, rename it here
    goto error;

  if (created_stores) g_object_unref (created_stores);  // -- unref created stores
  g_object_unref (newfile);	                        // -- unref creators reference
  g_free (tmp_fname);                                   // -- free temp file name
  g_free (abs_fname);                                   // -- free file name
  if (oldfile) g_object_unref (oldfile);                // -- unref old file

  return (TRUE);

error:

  if (created_stores)   // Remove new created stores
  {
    g_list_foreach (created_stores->items, remove_created_stores, NULL);
    g_object_unref (created_stores);  // -- unref created stores
  }

  if (newfile)
  {
    if (!ipatch_file_unlink (newfile, &local_err))   // -- Delete new file
    {
      g_warning (_("Failed to remove file after save failure: %s"),
                 ipatch_gerror_message (local_err));
      g_clear_error (&local_err);
    }

    g_object_unref (newfile);   // -- unref creators reference
  }

  g_free (tmp_fname);           // -- free temp file name
  g_free (abs_fname);           // -- free file name
  if (oldfile) g_object_unref (oldfile);        // -- unref old file

  return (FALSE);
}

/**
 * ipatch_base_close:
 * @base: Base item to close
 * @err: Location to store error info or %NULL
 *
 * Close a base instrument object (using ipatch_item_remove()), migrating sample data as needed.
 *
 * Returns: TRUE on success, FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_base_close (IpatchBase *base, GError **err)
{
  IpatchFile *file;

  g_return_val_if_fail (IPATCH_IS_BASE (base), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  g_object_get (base, "file", &file, NULL);     // ++ ref file (if any)

  ipatch_item_remove (IPATCH_ITEM (base));

  if (file && !ipatch_migrate_file_sample_data (file, NULL, NULL, 0, err))
  {
    g_object_unref (file);                      // -- unref file
    return (FALSE);
  }

  g_object_unref (file);                        // -- unref file

  return (TRUE);
}

/**
 * ipatch_close_base_list:
 * @list: List of base objects to close (non base objects are skipped)
 * @err: Location to store error info or %NULL
 *
 * Close a list of base instrument objects (using ipatch_item_remove_recursive()),
 * migrating sample data as needed.  Using this function instead of ipatch_base_close()
 * can save on unnecessary sample data migrations, if multiple base objects reference
 * the same sample data.
 *
 * Returns: TRUE on success, FALSE otherwise (in which case @err may be set)
 *
 * Since: 1.1.0
 */
gboolean
ipatch_close_base_list (IpatchList *list, GError **err)
{
  GList *p, *file_list = NULL;
  IpatchFile *file;
  gboolean retval = TRUE;
  GError *local_err = NULL;
  char *filename;

  g_return_val_if_fail (IPATCH_IS_LIST (list), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  for (p = list->items; p; p = p->next)
  {
    if (!IPATCH_IS_BASE (p->data)) continue;            // Just skip if its not a base object

    g_object_get (p->data, "file", &file, NULL);        // ++ ref file (if any)
    ipatch_item_remove_recursive (IPATCH_ITEM (p->data), TRUE);         // Recursively remove to release IpatchSampleData resources

    if (file) file_list = g_list_prepend (file_list, file);
  }

  file_list = g_list_reverse (file_list);               // Reverse list to migrate samples in same order

  for (p = file_list; p; p = g_list_delete_link (p, p))
  {
    file = p->data;

    if (!ipatch_migrate_file_sample_data (file, NULL, NULL, 0, &local_err))
    {
      if (!retval || !err)
      { // Log additional errors
        g_object_get (file, "file-name", &filename, NULL);      // ++ alloc filename
        g_critical (_("Error migrating samples from closed file '%s': %s"), filename,
                    ipatch_gerror_message (local_err));
        g_free (filename);                                      // -- free filename
        g_clear_error (&local_err);
      }
      else g_propagate_error (err, local_err);          // Propagate first error

      retval = FALSE;
    }

    g_object_unref (file);                              // -- unref file
  }

  return (retval);
}

