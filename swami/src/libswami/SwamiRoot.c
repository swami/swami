/*
 * SwamiRoot.c - Root Swami application object
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>

#include "SwamiRoot.h"
#include "SwamiLog.h"
#include "SwamiObject.h"
#include "i18n.h"

/* Maximum swap file waste in megabytes (not yet used) */
#define DEFAULT_SWAP_MAX_WASTE	64

/* Maximum sample size to import in megabytes
 * (To prevent "O crap, I didn't mean to load that one!") */
#define DEFAULT_SAMPLE_MAX_SIZE	32


/* Swami root object properties */
enum
{
  PROP_0,
  PROP_PATCH_SEARCH_PATH,
  PROP_PATCH_PATH,
  PROP_SAMPLE_PATH,
  PROP_SAMPLE_FORMAT,
  PROP_SWAP_MAX_WASTE,
  PROP_SAMPLE_MAX_SIZE,
  PROP_PATCH_ROOT
};

/* Swami root object signals */
enum
{
  SWAMI_PROP_NOTIFY,
  OBJECT_ADD,			/* add object */
  LAST_SIGNAL
};

/* --- private function prototypes --- */

static void swami_root_class_init (SwamiRootClass *klass);
static void swami_root_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec);
static void swami_root_get_property (GObject *object, guint property_id,
				     GValue *value, GParamSpec *pspec);
static void swami_root_init (SwamiRoot *root);

guint root_signals[LAST_SIGNAL] = { 0 };


GType
swami_root_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
    {
      static const GTypeInfo item_info =
	{
	  sizeof (SwamiRootClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) swami_root_class_init, NULL, NULL,
	  sizeof (SwamiRoot), 0,
	  (GInstanceInitFunc) swami_root_init,
	};

      item_type = g_type_register_static (SWAMI_TYPE_LOCK, "SwamiRoot",
					  &item_info, 0);
    }

  return (item_type);
}

static void
swami_root_class_init (SwamiRootClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  root_signals[SWAMI_PROP_NOTIFY] =
    g_signal_new ("swami-prop-notify", G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED
		  | G_SIGNAL_NO_HOOKS,
		  0, NULL, NULL,
		  g_cclosure_marshal_VOID__PARAM,
		  G_TYPE_NONE, 1, G_TYPE_PARAM);

  root_signals[OBJECT_ADD] =
    g_signal_new ("object-add", G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (SwamiRootClass, object_add), NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, G_TYPE_OBJECT);

  obj_class->set_property = swami_root_set_property;
  obj_class->get_property = swami_root_get_property;

  g_object_class_install_property (obj_class, PROP_PATCH_SEARCH_PATH,
			g_param_spec_string ("patch-search-path",
					     N_("Patch search path"),
					     N_("Patch search path"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PATCH_PATH,
			g_param_spec_string ("patch-path",
					     N_("Patch path"),
					     N_("Default patch path"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_PATH,
			g_param_spec_string ("sample-path",
					     N_("Sample path"),
					     N_("Default sample path"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_FORMAT,
			g_param_spec_string ("sample-format",
					     N_("Sample format"),
					     N_("Default sample format"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SWAP_MAX_WASTE,
		g_param_spec_int ("swap-max-waste",
				  N_("Swap max waste"),
				  N_("Max waste of sample swap in megabytes"),
				  1, G_MAXINT, DEFAULT_SWAP_MAX_WASTE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAMPLE_MAX_SIZE,
		g_param_spec_int ("sample-max-size",
				  N_("Sample max size"),
				  N_("Max sample size in megabytes"),
				  1, G_MAXINT, DEFAULT_SAMPLE_MAX_SIZE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PATCH_ROOT,
		g_param_spec_object ("patch-root", N_("Patch root"),
				     N_("Root container of instrument patch tree"),
				     SWAMI_TYPE_CONTAINER, G_PARAM_READABLE | IPATCH_PARAM_NO_SAVE));
}

static void
swami_root_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
  SwamiRoot *root = SWAMI_ROOT (object);

  switch (property_id)
    {
    case PROP_PATCH_SEARCH_PATH:
      if (root->patch_search_path) g_free (root->patch_search_path);
      root->patch_search_path = g_value_dup_string (value);
      break;
    case PROP_PATCH_PATH:
      if (root->patch_path) g_free (root->patch_path);
      root->patch_path = g_value_dup_string (value);
      break;
    case PROP_SAMPLE_PATH:
      if (root->sample_path) g_free (root->sample_path);
      root->sample_path = g_value_dup_string (value);
      break;
    case PROP_SAMPLE_FORMAT:
      if (root->sample_format) g_free (root->sample_format);
      root->sample_format = g_value_dup_string (value);
      break;
    case PROP_SWAP_MAX_WASTE:
      root->swap_max_waste = g_value_get_int (value);
      break;
    case PROP_SAMPLE_MAX_SIZE:
      root->sample_max_size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swami_root_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
  SwamiRoot *root = SWAMI_ROOT (object);

  switch (property_id)
    {
    case PROP_PATCH_SEARCH_PATH:
      g_value_set_string (value, root->patch_search_path);
      break;
    case PROP_PATCH_PATH:
      g_value_set_string (value, root->patch_path);
      break;
    case PROP_SAMPLE_PATH:
      g_value_set_string (value, root->sample_path);
      break;
    case PROP_SAMPLE_FORMAT:
      g_value_set_string (value, root->sample_format);
      break;
    case PROP_SWAP_MAX_WASTE:
      g_value_set_int (value, root->swap_max_waste);
      break;
    case PROP_SAMPLE_MAX_SIZE:
      g_value_set_int (value, root->sample_max_size);
      break;
    case PROP_PATCH_ROOT:
      g_value_set_object (value, root->patch_root);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swami_root_init (SwamiRoot *root)
{
  root->swap_max_waste = DEFAULT_SWAP_MAX_WASTE;
  root->sample_max_size = DEFAULT_SAMPLE_MAX_SIZE;
  root->patch_root = swami_container_new ();
  root->patch_root->root = root;

  /* set the IpatchItem hooks active flag to make all children items execute
     hook callback functions (once added to root) */
  ipatch_item_set_flags (IPATCH_ITEM (root->patch_root),
			 IPATCH_ITEM_HOOKS_ACTIVE);
  root->proptree = swami_prop_tree_new (); /* ++ ref property tree */
  swami_prop_tree_set_root (root->proptree, G_OBJECT (root));
}

/**
 * swami_root_new:
 *
 * Create a new Swami root object which is a toplevel container for
 * patches, objects, configuration data and state history.
 *
 * Returns: New Swami root object
 */
SwamiRoot *
swami_root_new (void)
{
  return SWAMI_ROOT (g_object_new (SWAMI_TYPE_ROOT, NULL));
}

/**
 * swami_get_root:
 * @object: An object registered to a #SwamiRoot, an #IpatchItem contained
 *   in a #SwamiRoot or the root itself
 *
 * Gets the #SwamiRoot object associated with a @object.
 *
 * Returns: The #SwamiRoot object or %NULL if @object not registered to a
 *   root. Returned root object is not referenced, we assume it won't be
 *   destroyed.
 */
SwamiRoot *
swami_get_root (gpointer object)
{
  SwamiContainer *container;
  SwamiRoot *root = NULL;
  SwamiObjectPropBag *propbag;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  if (SWAMI_IS_ROOT (object)) root = (SwamiRoot *)object;
  else if (IPATCH_IS_ITEM (object))
    {
      container = (SwamiContainer *)ipatch_item_peek_ancestor_by_type
	(object, SWAMI_TYPE_CONTAINER);
      root = container->root;
    }
  else
    {
      propbag = g_object_get_qdata (G_OBJECT (object),
				    swami_object_propbag_quark);
      if (propbag) root = propbag->root;
    }

  return (root);
}

/**
 * swami_root_get_objects:
 * @root: Swami root object
 *
 * Get an iterator filled with toplevel objects that are the first children
 * of a #SwamiRoot object property tree.
 *
 * Returns: New object list with a reference count of 1 which the caller owns,
 *   remember to unref it when finished.
 */
IpatchList *
swami_root_get_objects (SwamiRoot *root)
{
  GNode *node;
  IpatchList *list;

  g_return_val_if_fail (SWAMI_IS_ROOT (root), NULL);

  list = ipatch_list_new ();	/* ++ ref new list */

  SWAMI_LOCK_READ (root->proptree);
  node = root->proptree->tree->children;
  while (node)
    {
      g_object_ref (node->data);
      list->items = g_list_prepend (list->items, node->data);
      node = node->next;
    }
  SWAMI_UNLOCK_READ (root->proptree);

  return (list);
}

/**
 * swami_root_add_object:
 * @root: Swami root object
 * @object: Object to add
 *
 * Add an object to a Swami root property tree. A reference is held on
 * the object for the @root object.
 */
void
swami_root_add_object (SwamiRoot *root, GObject *object)
{
  g_return_if_fail (SWAMI_IS_ROOT (root));
  g_return_if_fail (G_IS_OBJECT (object));

  swami_object_set (object, "root", root, NULL);
  swami_prop_tree_prepend (root->proptree, G_OBJECT (root), object);

  g_signal_emit (root, root_signals[OBJECT_ADD], 0, object);
}

/**
 * swami_root_new_object:
 * @root: Swami root object
 * @type_name: Name of GObject derived GType of object to create and add to a
 *   @root object.
 *
 * Like swami_root_add_object() but creates a new object rather than using
 * an existing one.
 *
 * Returns: The new GObject created or NULL on error. The caller owns a
 * reference on the new object and should unref it when done. The @root
 * also owns a reference, until swami_root_remove_object() is called on it.
 */
GObject *
swami_root_new_object (SwamiRoot *root, const char *type_name)
{
  GObject *obj;
  GType type;

  g_return_val_if_fail (SWAMI_IS_ROOT (root), NULL);
  g_return_val_if_fail (g_type_from_name (type_name) != 0, NULL);

  type = g_type_from_name (type_name);
  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), NULL);

  if (!(obj = g_object_new (type, NULL))) return (NULL); /* ++ ref new item */
  swami_root_add_object (root, obj);

  return (obj);			/* !! caller takes over creator's ref */
}

/**
 * swami_root_prepend_object:
 * @root: Swami root object
 * @parent: New parent of @object
 * @object: Object to prepend
 *
 * Prepends an object to an object property tree in a @root object as
 * a child of @parent. Like swami_root_add_object() but allows parent
 * to specified (rather than using the @root as the parent).
 */
void
swami_root_prepend_object (SwamiRoot *root, GObject *parent, GObject *object)
{
  g_return_if_fail (SWAMI_IS_ROOT (root));
  g_return_if_fail (G_IS_OBJECT (parent));
  g_return_if_fail (G_IS_OBJECT (object));

  swami_object_set (object, "root", root, NULL);
  swami_prop_tree_prepend (root->proptree, parent, object);

  g_signal_emit (root, root_signals[OBJECT_ADD], 0, object);
}

/**
 * swami_root_insert_object_before:
 * @root: Swami root object
 * @parent: New parent of @object
 * @sibling: Sibling to insert object before or %NULL to append
 * @object: Object to insert
 *
 * Inserts an object into an object property tree in a @root object as
 * a child of @parent and before @sibling.
 */
void
swami_root_insert_object_before (SwamiRoot *root, GObject *parent,
				 GObject *sibling, GObject *object)
{
  g_return_if_fail (SWAMI_IS_ROOT (root));
  g_return_if_fail (G_IS_OBJECT (parent));
  g_return_if_fail (!sibling || G_IS_OBJECT (sibling));
  g_return_if_fail (G_IS_OBJECT (object));

  swami_object_set (object, "root", root, NULL);
  swami_prop_tree_insert_before (root->proptree, parent, sibling, object);

  g_signal_emit (root, root_signals[OBJECT_ADD], 0, object);
}

/**
 * swami_root_patch_load:
 * @root: Swami root object to load into
 * @filename: Name and path of file to load
 * @item: Location to store pointer to object that has been loaded into Swami
 *   root object (or %NULL). Remember to unref the object when done with it
 *   (not necessary of course if %NULL was passed).
 * @err: Location to store error info or %NULL
 *
 * Load an instrument patch file and append to Swami object tree. The caller
 * owns a reference to the returned patch object and should unref it when
 * done with the object.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_root_patch_load (SwamiRoot *root, const char *filename, IpatchItem **item,
		       GError **err)
{
  IpatchFileHandle *handle;
  GObject *obj;

  g_return_val_if_fail (SWAMI_IS_ROOT (root), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  handle = ipatch_file_identify_open (filename, err);   /* ++ open file handle */
  if (!handle) return (FALSE);

  if (!(obj = ipatch_convert_object_to_type (G_OBJECT (handle->file),
					     IPATCH_TYPE_BASE, err)))
  {
    ipatch_file_close (handle);  /* -- close file handle */
    return (FALSE);
  }

  ipatch_file_close (handle);  /* -- close file handle */

  ipatch_container_append (IPATCH_CONTAINER (root->patch_root),
			   IPATCH_ITEM (obj));

  /* !! if @item field was passed, then caller takes over ref */
  if (item) *item = IPATCH_ITEM (obj);
  else g_object_unref (obj);

  return (TRUE);
}

/**
 * swami_root_patch_save:
 * @item: Patch item to save.
 * @filename: New file name to save to or NULL to use current one.
 * @err: Location to store error info or NULL
 *
 * Save a patch item to a file.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_root_patch_save (IpatchItem *item, const char *filename, GError **err)
{
  IpatchConverterInfo *info;
  IpatchFile *file;
  char *dir, *tmpfname, *prop_filename = NULL;
  int tmpfd;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* if no filename specified try to use current one */
  if (!filename)
    {
      g_object_get (item, "file-name", &prop_filename, NULL);
      if (!prop_filename)
	{
	  g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_INVALID,
		       _("File name not supplied and none currently assigned"));
	  return (FALSE);
	}
      filename = prop_filename;
    }

  info = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (item), IPATCH_TYPE_FILE);
  if (!info)
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_UNSUPPORTED,
		   _("Saving object of type '%s' to file '%s' not supported"),
                   g_type_name (G_OBJECT_TYPE (item)), filename);
      g_free (prop_filename);
      return (FALSE);
    }

  /* get the destination directory and create a temporary file template */
  dir = g_path_get_dirname (filename);
  tmpfname = g_build_filename (dir, "swami_tmpXXXXXX", NULL);
  g_free (dir);

  /* open temporary file in same directory as destination */
  if ((tmpfd = g_mkstemp (tmpfname)) == -1)
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_IO,
		   _("Unable to open temp file '%s' for writing: %s"),
		   tmpfname, g_strerror (errno));
      g_free (tmpfname);
      g_free (prop_filename);
      return (FALSE);
    }

  file = IPATCH_FILE (g_object_new (info->dest_type, NULL)); /* ++ ref new file */
  ipatch_file_assign_fd (file, tmpfd, TRUE);    /* Assign file descriptor and set close on finalize */

  /* attempt to save patch file */
  if (!ipatch_convert_objects (G_OBJECT (item), G_OBJECT (file), err))
    {
      g_object_unref (file);	/* -- unref creators reference */
      unlink (tmpfname);
      g_free (tmpfname);
      g_free (prop_filename);
      return (FALSE);
    }

  g_object_unref (file);	/* -- unref creators reference */

#ifdef G_OS_WIN32
  /* win32 rename won't overwrite files, so just blindly unlink destination */
  unlink (filename);
#endif

  if (rename (tmpfname, filename) == -1)
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_IO,
		   _("Failed to rename temp file: %s"), g_strerror (errno));
      unlink (tmpfname);
      g_free (tmpfname);
      g_free (prop_filename);
      return (FALSE);
    }

  g_free (tmpfname);

  if (prop_filename) g_free (prop_filename); /* same file name? */
  else g_object_set (item, "file-name", filename, NULL); /* new file name */

  return (TRUE);
}
