/*
 * patch_funcs.c - General instrument patch functions
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
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
#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiMultiSave.h"
#include "SwamiguiPaste.h"
#include "SwamiguiProp.h"
#include "SwamiguiRoot.h"
#include "SwamiguiTree.h"
#include "util.h"
#include "i18n.h"

/* maximum notebook tab length (in characters).  Only used for item properties
   dialog currently. */
#define MAX_NOTEBOOK_TAB_LENGTH 20

/* Columns used in sample export file format combo box list store */
enum
{
  FILE_FORMAT_COL_TEXT,         /* Descriptive format label displayed in combo */
  FILE_FORMAT_COL_NAME,         /* Name identifier of format */
  FILE_FORMAT_COL_VALUE,        /* Enum value of format */
  FILE_FORMAT_COL_COUNT
};

/* Local Prototypes */

static void swamigui_cb_load_files_response (GtkWidget *dialog, gint response,
					     gpointer user_data);
static gboolean swamigui_load_sample_helper (const char *fname, IpatchItem *parent_hint,
                                             IpatchPaste *paste, IpatchList *biglist,
                                             gboolean *paste_possible);
static void swamigui_cb_export_samples_response (GtkWidget *dialog,
						 gint response,
						 gpointer user_data);
/* global variables */

static char *path_patch_load = NULL; /* last loaded patch path */
//static char *path_patch_save = NULL; /* last saved patch path */
static char *path_sample_load = NULL; /* last sample load path */
static char *path_sample_export = NULL;	/* last sample export path */
static char *last_sample_format = NULL;	/* last sample export format */

/* clipboard for item selections */
static IpatchList *item_clipboard = NULL;


/**
 * swamigui_load_files:
 * @parent_hint: Parent of new samples, a child thereof or SwamiRoot object
 * @load_samples: TRUE to load audio files only, FALSE for patch and audio files
 *
 * Open files routine. Displays a file selection dialog to open patch
 * and sample files with.
 */
void
swamigui_load_files (GObject *parent_hint, gboolean load_samples)
{
  GtkWidget *dialog;
  GtkWindow *main_window;
  char *path;

  /* ++ ref main window */
  g_object_get (swamigui_root, "main-window", &main_window, NULL);

  dialog = gtk_file_chooser_dialog_new (_("Load files"), main_window,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_ADD, GTK_RESPONSE_APPLY,
					NULL);

  g_object_unref (main_window);	  /* -- unref main window */

  /* enable multiple selection mode */
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  /* set default response */
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  g_object_set_data (G_OBJECT (dialog), "_load_samples", GUINT_TO_POINTER (load_samples));

  // Is file path set? If not use default from preferences
  if (load_samples)
  {
    if (!path_sample_load)
    {
      SwamiRoot *root = swami_get_root (G_OBJECT (parent_hint));
      g_object_get (G_OBJECT (root), "sample-path", &path_sample_load, NULL);   // ++ alloc sample path
    }

    path = path_sample_load;
  }
  else
  {
    if (!path_patch_load)
    {
      SwamiRoot *root = swami_get_root (G_OBJECT (parent_hint));
      g_object_get (root, "patch-path", &path_patch_load, NULL);        // ++ alloc patch path
    }

    path = path_patch_load;
  }

  if (path && strlen (path))
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), path);

  if (parent_hint) g_object_ref (parent_hint); /* ++ ref for dialog */

  g_signal_connect (G_OBJECT (dialog), "response",
		    G_CALLBACK (swamigui_cb_load_files_response),
		    parent_hint);

  gtk_widget_show (dialog);
}

/* loads the list of user selected files */
static void
swamigui_cb_load_files_response (GtkWidget *dialog, gint response,
				 gpointer user_data)
{
  GObject *parent_hint = G_OBJECT (user_data);
  SwamiRoot *root = swami_get_root (G_OBJECT (parent_hint));
  GSList *file_names, *p;
  gboolean patch_loaded = FALSE, samples_loaded = FALSE;
  char *fname;
  GError *err = NULL;
  GtkRecentManager *manager;
  char *file_uri;
  GType type;
  gboolean load_samples, paste_possible;
  IpatchPaste *paste;
  IpatchList *biglist;
  GtkRecentData recent_data;
  char *groups[2] = { SWAMIGUI_ROOT_INSTRUMENT_FILES_GROUP, NULL };
  IpatchItem *patch;

  if (response != GTK_RESPONSE_ACCEPT && response != GTK_RESPONSE_APPLY)
  {
    if (parent_hint) g_object_unref (parent_hint);
    gtk_widget_destroy (dialog);
    return;
  }

  load_samples = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (dialog), "_load_samples"));

  paste = ipatch_paste_new ();	/* ++ ref paste object */
  biglist = ipatch_list_new ();	/* ++ ref list */

  /* "Add" or "OK" button clicked */

  file_names = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));

  /* loop over file names */
  for (p = file_names; p; p = g_slist_next (p))
  {
    fname = (char *)(p->data);

    // Identify file type
    type = ipatch_file_identify_name (fname, &err);

    if (type == G_TYPE_NONE)
    {
      g_critical (_("Failed to identify file '%s': %s"), fname,
                  ipatch_gerror_message (err));
      g_clear_error (&err);
      continue;
    }

    if (!load_samples && ipatch_find_converter (type, IPATCH_TYPE_BASE) != 0)
    {
      patch_loaded = TRUE;      // Set patch path regardless if successful

      if (!swami_root_patch_load (root, fname, &patch, &err))   // ++ ref patch object
      { /* error occurred - log it */
        g_critical (_("Failed to load file '%s': %s"), fname, ipatch_gerror_message (err));
        g_clear_error (&err);
        continue;
      }

      if ((file_uri = g_filename_to_uri (fname, NULL, NULL)))   // ++ alloc
      {
        manager = gtk_recent_manager_get_default ();

        recent_data.display_name = NULL;
        recent_data.description = NULL;

        // ++ alloc mime type
        recent_data.mime_type = ipatch_base_type_get_mime_type (G_OBJECT_TYPE (patch));
        if (!recent_data.mime_type) recent_data.mime_type = g_strdup ("application/octet-stream");

        recent_data.app_name = g_strdup (g_get_application_name ());            // ++ alloc
        recent_data.app_exec = g_strjoin (" ", g_get_prgname (), "%f", NULL);   // ++ alloc
        recent_data.groups = groups;
        recent_data.is_private = FALSE;

        // Add full info to set group of instrument files, to filter out sample files from the recent menu, etc.
        if (!gtk_recent_manager_add_full (manager, file_uri, &recent_data))
          g_warning ("Error while adding file name to recent manager.");

        g_free (recent_data.mime_type); // -- free mime type
        g_free (recent_data.app_name);  // -- free application name
        g_free (recent_data.app_exec);  // -- free app exec command
        g_free (file_uri);              // -- free file uri
      }

      g_object_unref (patch);           // -- free
    }
    else if (g_type_is_a (type, IPATCH_TYPE_SND_FILE))
    {
      paste_possible = FALSE;

      if (!IPATCH_IS_ITEM (parent_hint)
          || !swamigui_load_sample_helper (fname, IPATCH_ITEM (parent_hint),
                                           paste, biglist, &paste_possible))
      {
        if (!paste_possible && !samples_loaded)
        {
          GtkWidget *msg = gtk_message_dialog_new (GTK_WINDOW (SWAMIGUI_ROOT (root)->main_window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   _("Please select location in tree view to load samples into."));
          gtk_dialog_run (GTK_DIALOG (msg));
          gtk_widget_destroy (msg);
        }
      }

      samples_loaded = TRUE;       // Set sample path regardless if successful
    }
    else
    {
      if (load_samples)
        g_critical (_("File '%s' is not a supported sample file"), fname);
      else g_critical (_("File '%s' is not a supported file type"), fname);
    }
  }

  if (samples_loaded)
  { /* finish the paste operation */
    if (ipatch_paste_finish (paste, &err))
    { /* select all samples which were added */
      biglist->items = g_list_reverse (biglist->items);	/* put in right order */
      g_object_set (swamigui_root, "selection", biglist, NULL);
    }
    else
    {
      g_critical (_("Failed to finish load of samples (paste operation): %s"),
                  ipatch_gerror_message (err));
      g_clear_error (&err);
    }

    /* !! free old load path and take over allocation of new path */
    g_free (path_sample_load);
    path_sample_load = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
  }

  g_object_unref (biglist);	/* -- unref list */
  g_object_unref (paste);	/* -- unref paste object */

  if (patch_loaded)
  { /* !! free old load path and take over allocation of new path */
    g_free (path_patch_load);
    path_patch_load = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
  }

  /* free the file name list and strings */
  for (p = file_names; p; p = g_slist_delete_link (p, p))
    g_free (p->data);

  /* destroy dialog if "OK" button was clicked */
  if (response == GTK_RESPONSE_ACCEPT)
  {
    if (parent_hint) g_object_unref (parent_hint);
    gtk_widget_destroy (dialog);
  }
}

static gboolean
swamigui_load_sample_helper (const char *fname, IpatchItem *parent_hint,
                             IpatchPaste *paste, IpatchList *biglist, gboolean *paste_possible)
{
  IpatchFile *file;
  IpatchList *list;
  GList *lp;
  int size, max_size;
  GError *err = NULL;

  *paste_possible = TRUE;

  file = ipatch_file_identify_new (fname, &err);        // ++ ref file

  if (!file)
  {
    g_critical (_("Failed to identify and open file '%s': %s"), fname,
                ipatch_gerror_message (err));
    g_clear_error (&err);
    return (FALSE);
  }

  g_object_get (swami_root, "sample-max-size", &max_size, NULL);

  if (max_size != 0)    // 0 means unlimited import size
  {
    size = ipatch_file_get_size (file, &err);

    if (size == -1)
    {
      g_warning (_("Failed to get sample file '%s' size: %s"), fname, ipatch_gerror_message (err));
      g_clear_error (&err);
    }
    else if (size > max_size * 1024 * 1024)
    {
      g_critical (_("Sample file '%s' of %d bytes exceeds max sample setting of %dMB"), fname, size, max_size);
      return (FALSE);
    }
  }

  /* determine if IpatchSampleFile can be pasted to destination.. */
  if (!ipatch_is_paste_possible (IPATCH_ITEM (parent_hint),
                                 IPATCH_ITEM (file)))
  {
    g_object_unref (file);      // -- unref file
    *paste_possible = FALSE;
    return (FALSE);
  }

  /* paste sample file to destination */
  if (!ipatch_paste_objects (paste, IPATCH_ITEM (parent_hint),
                             IPATCH_ITEM (file), &err))
  { /* object paste failed */
    g_critical (_("Failed to load object of type '%s' to '%s': %s"),
                g_type_name (IPATCH_TYPE_SND_FILE),
                g_type_name (G_OBJECT_TYPE (parent_hint)),
                ipatch_gerror_message (err));
    g_clear_error (&err);
    g_object_unref (file);      // -- unref file
    return (FALSE);
  }

  g_object_unref (file);        // -- unref file

  list = ipatch_paste_get_add_list (paste);     /* ++ ref added object list */

  if (list)
  { /* biglist takes over items of list */
    for (lp = list->items; lp; lp = g_list_delete_link (lp, lp))
      biglist->items = g_list_prepend (biglist->items, lp->data);

    list->items = NULL;
    g_object_unref (list);	/* -- unref list */
  }

  return (TRUE);
}

/**
 * swamigui_close_files:
 * @item_list: List of items to close (usually only #IpatchBase derived
 *   objects make sense).
 *
 * User interface to close files.
 */
void
swamigui_close_files (IpatchList *item_list)
{
  GtkWidget *dialog;
  IpatchIter iter;
  IpatchItem *item;
  gboolean patch_found = FALSE;
  gboolean changed;
  GError *err = NULL;

  /* see if there are any patch items to close and if they have been changed */
  ipatch_list_init_iter (item_list, &iter);
  item = ipatch_item_first (&iter);
  while (item)
    {
      if (IPATCH_IS_BASE (item)) /* only IpatchBase derived patches */
	{
	  patch_found = TRUE;
	  g_object_get (item, "changed", &changed, NULL);
	  if (changed) break;
	}
      item = ipatch_item_next (&iter);
    }

  if (!patch_found) return;	/* no patches to close, return */

  /* if no items changed, then go ahead and close files */
  if (!item)
  {
    if (!ipatch_close_base_list (item_list, &err))
    {
      g_warning (_("Failed to close file(s): %s"), ipatch_gerror_message (err));
      g_clear_error (&err);
    }

    return;
  }

  /* item(s) have been changed, pop user interactive dialog */
  dialog = swamigui_multi_save_new (_("Close files"),
				    _("Save changed files before closing?"),
                                    SWAMIGUI_MULTI_SAVE_CLOSE_MODE);
  swamigui_multi_save_set_selection (SWAMIGUI_MULTI_SAVE (dialog), item_list);
  gtk_widget_show (dialog);
}

/**
 * swamigui_save_files:
 * @item_list: List of items to save.
 * @saveas: TRUE forces popup of dialog even if all files have previously
 *   been saved (forces "Save As").
 *
 * Save files user interface.  If @saveas is %FALSE and all selected files
 * have already been saved before, then they are saved.  If only one file has
 * not yet been saved then the normal save as file dialog is shown.  If
 * multiple files have not been saved or @saveas is %TRUE then the multi-file
 * save dialog is used.
 */
void
swamigui_save_files (IpatchList *item_list, gboolean saveas)
{
  GtkWidget *dialog;
  IpatchItem *item, *base;
  char *filename;
  gboolean popup = FALSE, match = FALSE;
  gboolean saved, changed;
  GError *err = NULL;
  int savecount = 0, failcount = 0;
  GList *p;

  /* see if any items have been changed */
  for (p = item_list->items; p; p = p->next)
  {
    item = (IpatchItem *)(p->data);
    base = ipatch_item_get_base (item);         /* ++ ref base object */

    if (base)           /* only save IpatchBase items or children thereof */
    {
      match = TRUE;		/* found a patch base object */
      g_object_get (base,
		    "saved", &saved,
		    "changed", &changed,
		    NULL);

      if (!saved) popup = TRUE; /* never been saved, force dialog */
      g_object_unref (base);    /* -- unref base */
    }
  }

  if (!match) return;		/* return, if there are no items to save */

  popup |= saveas;		/* force dialog popup, "Save As"? */

  /* no dialog required? (all items previously saved and !saveas) */
  if (!popup)
  {
    for (p = item_list->items; p; p = p->next)
    {
      item = (IpatchItem *)(p->data);
      base = ipatch_item_get_base (item);         /* ++ ref base object */
      if (!base) continue;

      g_object_get (base, "file-name", &filename, NULL);        /* ++ alloc filename */

      if (!swami_root_patch_save (IPATCH_ITEM (base), filename, &err))
      {
	g_critical (_("Failed to save file '%s': %s"), filename,
		    ipatch_gerror_message (err));
	g_clear_error (&err);
	failcount++;
      }
      else savecount++;

      g_free (filename);        /* -- free filename */
      g_object_unref (base);    /* -- unref base object */
    }

    if (!failcount) swamigui_statusbar_printf (swamigui_root->statusbar,
                                               _("Saved %d file(s)"), savecount);
    else swamigui_statusbar_printf (swamigui_root->statusbar,
                                    _("Saved %d file(s), %d FAILED"), savecount,
                                    failcount);
    return;
  }

  /* save-as was requested or a file has not yet been saved */
  dialog = swamigui_multi_save_new (_("Save files"),
				    _("Select files to save"), 0);
  swamigui_multi_save_set_selection (SWAMIGUI_MULTI_SAVE (dialog), item_list);
  gtk_widget_show (dialog);
}

/**
 * swamigui_delete_items:
 * @item_list: List of items to delete
 *
 * Delete patch items
 */
void
swamigui_delete_items (IpatchList *item_list)
{
  IpatchItem *parent = NULL;
  gboolean same_parent = TRUE;
  IpatchItem *item;
  IpatchIter iter;
  IpatchList *list;

  ipatch_list_init_iter (item_list, &iter);
  
  for (item = ipatch_item_first (&iter); item; item = ipatch_item_next (&iter))
  {
    if (IPATCH_IS_ITEM (item) && !IPATCH_IS_BASE (item))
    {
      if (same_parent)
      {
        if (parent)
        {
          if (parent != ipatch_item_peek_parent (item))
            same_parent = FALSE;
        }
        else parent = ipatch_item_get_parent (item);    /* ++ ref parent */
      }

      ipatch_item_remove (item);
    }
  }

  /* If all items had same parent and it wasn't the patch object, make it the
   * new selection */
  if (same_parent && parent && !IPATCH_IS_BASE (parent))
  {
    list = ipatch_list_new ();  /* ++ ref list */
    list->items = g_list_append (list->items, g_object_ref (parent));
    swamigui_tree_set_selection (SWAMIGUI_TREE (swamigui_root->tree), list);
    g_object_unref (list);      /* -- unref list */
  }

  if (parent) g_object_unref (parent);  /* -- unref parent */
}

/**
 * swamigui_wtbl_load_patch:
 * @item: Patch to load into wavetable.
 *
 * Load a patch item
 */
void
swamigui_wtbl_load_patch (IpatchItem *item)
{
  SwamiRoot *root;
  GObject *wavetbl;
  GError *err = NULL;

  /* IpatchBase derived objects only */
  if (!IPATCH_IS_BASE (item)) return;

  root = swami_get_root (G_OBJECT (item));
  if (!root) return;

  wavetbl = swami_object_get_by_type (G_OBJECT (root), "SwamiWavetbl");
  if (wavetbl)
    {
      if (!swami_wavetbl_load_patch (SWAMI_WAVETBL (wavetbl), item, &err))
	{
	  g_critical (_("Patch load failed: %s"), ipatch_gerror_message (err));
	  g_clear_error (&err);
	}
    }
}

/**
 * swamigui_new_item:
 * @parent_hint: The parent of the new item or a hint item. An example of
 *   a hint item is a SWAMIGUI_TREE_PRESET_MELODIC item which would allow the
 *   real IpatchSF2Preset parent to be found, and would also indicate that
 *   the new zone should be in the melodic branch. Can (and should be) NULL for
 *   toplevel patch objects (IpatchSF2, etc).
 * @type: GType of an #IpatchItem derived type to create.
 *
 * Create a new patch item.
 */
void
swamigui_new_item (IpatchItem *parent_hint, GType type)
{
  IpatchItem *new_item, *real_parent = NULL;
  IpatchVirtualContainerConformFunc conform_func;
  IpatchList *list;

  g_return_if_fail (!parent_hint || IPATCH_IS_ITEM (parent_hint));

  if (!parent_hint) parent_hint = IPATCH_ITEM (swami_root->patch_root);

  new_item = g_object_new (type, NULL);	/* ++ ref new item */

  /* parent hint is a virtual container? */
  if (IPATCH_IS_VIRTUAL_CONTAINER (parent_hint))
  { /* get virtual container conform function if any */
    ipatch_type_get (G_OBJECT_TYPE (parent_hint), "virtual-child-conform-func",
		     &conform_func, NULL);

    real_parent = ipatch_item_get_parent (parent_hint); /* ++ ref parent */
    g_return_if_fail (real_parent != NULL);

    parent_hint = real_parent;

    /* force the new item to conform to virtual container parent_hint */
    if (conform_func) conform_func ((GObject *)new_item);
  }

  /* add and make unique (if appropriate) */
  ipatch_container_add_unique (IPATCH_CONTAINER (parent_hint), new_item);

  /* -- unref real parent object if parent_hint was a virtual container */
  if (real_parent) g_object_unref (real_parent);

  /* update selection to be new item */
  list = ipatch_list_new ();	/* ++ ref new list */
  list->items = g_list_append (list->items, new_item);  /* !! list takes over ref */

  g_object_set (swamigui_root, "selection", list, NULL);
  g_object_unref (list);	/* -- unref new list */
}

/**
 * swamigui_goto_link_item:
 * @item: Patch item
 * @tree: Swami tree object
 *
 * Goto an item's linked item in a #SwamiguiTree object.
 * Moves the view and selects the item in a #SwamiguiTree that is linked
 * by @item.
 */
void
swamigui_goto_link_item (IpatchItem *item, SwamiguiTree *tree)
{
  GObject *link = NULL;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (SWAMIGUI_IS_TREE (tree));

  g_object_get (item, "link-item", &link, NULL); /* ++ ref link */

  if (link)
    {
      swamigui_tree_spotlight_item (tree, link);
      g_object_unref (link);	/* -- unref from g_object_get */
    }
}

/**
 * swamigui_export_samples:
 * @samples: List of objects (non #IpatchSample interface items are ignored)
 *
 * Export one or more samples (object with #IpatchSample interface) to a file
 * or directory.
 */
void
swamigui_export_samples (IpatchList *samples)
{
  gboolean found_sample = FALSE;
  GtkWindow *main_window;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *combo;
  GEnumClass *format_enum;
  GtkListStore *format_store;
  GtkCellRenderer *renderer;
  gboolean multi;
  GtkTreeIter iter;
  IpatchSample *sample = NULL;
  int sel, def_index = 0;
  guint i;
  GList *p;

  g_return_if_fail (IPATCH_IS_LIST (samples));

  for (p = samples->items; p; p = p->next)
    {
      if (IPATCH_IS_SAMPLE (p->data))
	{
	  if (found_sample) break;
          sample = p->data;
	  found_sample = TRUE;
	}
    }

  if (!found_sample) return;

  multi = (p != NULL);  /* Multi sample selection? */

  /* ++ ref main window */
  g_object_get (swamigui_root, "main-window", &main_window, NULL);

  /* if only 1 item found, create a file save dialog, otherwise create a
     folder selection dialog */
  dialog = gtk_file_chooser_dialog_new (_("Export samples"), main_window,
					multi ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
					: GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
  g_object_unref (main_window);		/* -- unref main window */

  /* set default response */
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  /* if sample load path isn't set, use default from config */
  if (!path_sample_export)
    g_object_get (swamigui_root, "sample-path", &path_sample_export, NULL);

  if (path_sample_export && strlen (path_sample_export))
  {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                         path_sample_export);

    if (!multi)         /* Single sample export? */
    {
      char *name, *filename;

      /* Set the file name to be that of the sample's title */
      g_object_get (sample, "title", &name, NULL);      /* ++ alloc name */
      filename = g_strconcat (name, ".wav", NULL);     /* ++ alloc filename */
      g_free (name);    /* -- free name */

      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);
      g_free (filename);        /* -- free file name */
    }
  }

  g_object_set_data_full (G_OBJECT (dialog), "samples", g_object_ref (samples),
			  (GDestroyNotify)g_object_unref);
  g_object_set_data (G_OBJECT (dialog), "multi", GINT_TO_POINTER (multi));

  g_signal_connect (G_OBJECT (dialog), "response",
		    G_CALLBACK (swamigui_cb_export_samples_response), NULL);

  /* Create file format selector combo */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("File format"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  /* ++ ref new store for file format combo box */
  format_store = gtk_list_store_new (FILE_FORMAT_COL_COUNT, G_TYPE_STRING,
                                     G_TYPE_STRING, G_TYPE_INT);
  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (format_store));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                  "text", FILE_FORMAT_COL_TEXT, NULL);

  if (!last_sample_format)
  {
    g_object_get (swamigui_root, "sample-format", &last_sample_format, NULL);
    if (!last_sample_format) last_sample_format = g_strdup ("wav");
  }

  sel = -1;	/* selected index */

  /* Populate file formats */
  format_enum = g_type_class_ref (g_type_from_name ("IpatchSndFileFormat"));    /* ++ ref class */

  if (format_enum)
  {
    for (i = 0; i < format_enum->n_values; i++)
    {
      gtk_list_store_append (format_store, &iter);
      gtk_list_store_set (format_store, &iter,
                          FILE_FORMAT_COL_TEXT, format_enum->values[i].value_nick,
                          FILE_FORMAT_COL_NAME, format_enum->values[i].value_name,
                          FILE_FORMAT_COL_VALUE, format_enum->values[i].value,
                          -1);

      if (last_sample_format
	  && strcmp (last_sample_format, format_enum->values[i].value_name) == 0)
	sel = i;

      if (format_enum->values[i].value == IPATCH_SND_FILE_DEFAULT_FORMAT)
        def_index = i;
    }

    g_type_class_unref (format_enum);   /* -- unref enum class */
  }

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), sel != -1 ? sel : def_index);

  g_object_unref (format_store);        /* -- unref the list store */

  gtk_widget_show (combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (dialog), "combo", combo);

  gtk_widget_show (dialog);
}

static void
swamigui_cb_export_samples_response (GtkWidget *dialog, gint response,
				     gpointer user_data)
{
  IpatchList *samples;
  IpatchSample *sample;
  GtkWidget *combo;
  gboolean multi;
  char *filepath;	/* file name or directory name (single/multi) */
  char *filename;
  GError *err = NULL;
  GtkTreeModel *format_model;
  GtkTreeIter iter;
  char *format_name;
  int format_value = IPATCH_SND_FILE_DEFAULT_FORMAT;
  GList *p;

  if (response != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  samples = IPATCH_LIST (g_object_get_data (G_OBJECT (dialog), "samples"));
  multi = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "multi"));
  combo = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "combo"));

  filepath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
  {
    format_model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

    gtk_tree_model_get (format_model, &iter,
                        FILE_FORMAT_COL_NAME, &format_name,     /* ++ alloc */
                        FILE_FORMAT_COL_VALUE, &format_value,
                        -1);
  }

  /* update last sample format */
  if (last_sample_format) g_free (last_sample_format);
  last_sample_format = format_name;  /* !! last_sample_format takes over alloc */

  for (p = samples->items; p; p = p->next)
    {
      if (!IPATCH_IS_SAMPLE (p->data)) continue;

      sample = IPATCH_SAMPLE (p->data);

      /* compose file name */
      if (multi)
	{
	  char *name, *temp;

	  g_object_get (sample, "title", &name, NULL);
	  temp = g_strconcat (name, ".", format_name, NULL);
	  g_free (name);

	  filename = g_build_filename (filepath, temp, NULL);
	  g_free (temp);
	}
      else filename = g_strdup (filepath);

      if (!ipatch_sample_save_to_file (sample, filename, format_value, -1, &err))
	{
	  g_critical (_("Failed to save sample '%s': %s"), filename,
		      ipatch_gerror_message (err));
	  continue;
	}

      g_free (filename);
    }

  g_free (filepath);

  gtk_widget_destroy (dialog);
}

/**
 * swamigui_copy_items:
 * @items: List of items to copy or %NULL to clear
 *
 * Set the item clipboard to a given list of items.
 */
void
swamigui_copy_items (IpatchList *items)
{
  g_return_if_fail (!items || IPATCH_IS_LIST (items));

  if (item_clipboard)
    {
      g_object_unref (item_clipboard);
      item_clipboard = NULL;
    }

  if (items)
    item_clipboard = ipatch_list_duplicate (items);
}

/* structure used to remember user decisions */
typedef struct
{
  SwamiguiPasteDecision all;   /* choice for all items or 0 for none */
  GList *types;		   /* per type choices (RememberTypeChoice) */
} RememberChoices;

/* per item type choice structure */
typedef struct
{
  GType type;			/* GType of this choice */
  SwamiguiPasteDecision choice;	/* choice */
} RememberTypeChoice;

/**
 * swamigui_paste_items:
 * @dstitem: Destination item for paste
 * @items: List of source items to paste to destination item or %NULL
 *   to use item clipboard
 *
 * Paste items user interface routine
 */
void
swamigui_paste_items (IpatchItem *dstitem, GList *items)
{
  IpatchPaste *paste;		/* paste instance */
  IpatchItem *src;
  GError *err = NULL;
  GList *p;

  /* use clipboard if no items given */
  if (!items && item_clipboard)
    items = item_clipboard->items;

  paste = ipatch_paste_new ();		/* ++ ref new paste instance */

  for (p = items; p; p = p->next)	/* loop on source items */
    {
      src = IPATCH_ITEM (p->data);

      if (ipatch_is_paste_possible (dstitem, src))	/* paste possible? */
	{	/* add paste operation to instance */
	  if (!ipatch_paste_objects (paste, dstitem, src, &err))
	    {
	      g_critical (_("Failed to paste item of type %s to %s: %s"),
			  G_OBJECT_TYPE_NAME (src),
			  G_OBJECT_TYPE_NAME (dstitem),
			  ipatch_gerror_message (err));
	      g_clear_error (&err);
	    }
	}
    }

  /* complete the paste operations */
  if (!ipatch_paste_finish (paste, &err))
    {
      g_critical (_("Failed to execute paste operation: %s"),
		  ipatch_gerror_message (err));
      g_clear_error (&err);
    }

  g_object_unref (paste);	/* -- unref paste instance */
}

/**
 * swamigui_get_clipboard_items:
 *
 * Get the current item clipboard list used for copy/paste operations.
 *
 * Returns: Current clipboard item list or NULL.  Caller should not modify the list
 *   but owns a reference and should unref the list object when finished using it.
 */
IpatchList *
swamigui_get_clipboard_items (void)
{
  if (!item_clipboard) return (NULL);
  return (g_object_ref (item_clipboard));
}

