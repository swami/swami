/*
 * SwamiguiMenu.c - Swami GUI Menu object
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "SwamiguiMenu.h"
#include "SwamiguiRoot.h"
#include "SwamiguiPref.h"
#include "SwamiguiPythonView.h"
#include "help.h"
#include "patch_funcs.h"
#include "i18n.h"
#include "icons.h"
#include "splash.h"
#include "util.h"

static void swamigui_menu_class_init (SwamiguiMenuClass *klass);
static void swamigui_menu_init (SwamiguiMenu *menubar);

static void
swamigui_menu_recent_chooser_item_activated (GtkRecentChooser *chooser,
					     gpointer user_data);

static void swamigui_menu_realize (GtkWidget *widget);
static void swamigui_menu_update_new_type_item (void);
static GtkWidget *create_patch_type_menu (SwamiguiMenu *menubar);
static int sort_by_type_name (const void *a, const void *b);


/* menu callbacks */
static void swamigui_menu_cb_new_patch (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_load_files (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_save_all (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_quit (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_preferences (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_swamitips (GtkWidget *mitem, gpointer data);
static void swamigui_menu_cb_splash_image (GtkWidget *mitem, gpointer data);

#ifdef PYTHON_SUPPORT
static void swamigui_menu_cb_python (GtkWidget *mitem, gpointer data);
#endif

static void swamigui_menu_cb_restart_fluid (GtkWidget *mitem, gpointer data);


static GtkWidgetClass *parent_class = NULL;

/* the last patch type selected from the NewType menu item */
static GType last_new_type = 0;
static GtkWidget *last_new_mitem = NULL;

static GtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" }, /* name, stock id, label */
  { "EditMenu", NULL, "_Edit" }, /* name, stock id, label */
  { "PluginsMenu", NULL, "_Plugins"  },	/* name, stock id, label */
  { "ToolsMenu", NULL, "_Tools" }, /* name, stock id, label */
  { "HelpMenu", NULL, "_Help" }, /* name, stock id, label */

  /* File Menu */

  /* New is not actually on the menu, just used for key accelerator */
  { "New", GTK_STOCK_NEW, "_New",	/* name, stock id, label */
    "<control>N",	/* label, accelerator */
    NULL, /* tooltip */
    G_CALLBACK (swamigui_menu_cb_new_patch) },
  { "NewType", GTK_STOCK_NEW, "N_ew...",
    "", N_("Create a new patch file of type..") },
  { "Open", GTK_STOCK_OPEN,	/* name, stock id */
    "_Open", "<control>O",	/* label, accelerator */
    NULL,			/* tooltip */
    G_CALLBACK (swamigui_menu_cb_load_files) }, 

  { "OpenRecent", GTK_STOCK_OPEN,	/* name, stock id */
    "Open _Recent", "",	/* label, accelerator */
    NULL,			/* tooltip */
    NULL },			/* callback */

  { "SaveAll", GTK_STOCK_SAVE,	/* name, stock id */
    "_Save All", "",	/* label, accelerator */     
    NULL,			/* tooltip */
    G_CALLBACK (swamigui_menu_cb_save_all) },
  { "Quit", GTK_STOCK_QUIT,	/* name, stock id */
    "_Quit", "<control>Q",	/* label, accelerator */     
    NULL,			/* tooltip */
    G_CALLBACK (swamigui_menu_cb_quit) },

  /* Edit Menu */
  { "Preferences", GTK_STOCK_PREFERENCES,	/* name, stock id */
    "_Preferences", "",	/* label, accelerator */     
    NULL,			/* tooltip */
    G_CALLBACK (swamigui_menu_cb_preferences) },

  /* Plugins Menu */
  { "RestartFluid", GTK_STOCK_REFRESH, N_("_Restart FluidSynth"), "",
    N_("Restart FluidSynth plugin"),
    G_CALLBACK (swamigui_menu_cb_restart_fluid) },

  /* Tools Menu */
#ifdef PYTHON_SUPPORT
  { "Python", SWAMIGUI_STOCK_PYTHON,	/* name, stock id */
    "_Python", "",	/* label, accelerator */     
    N_("Python script editor and console"),	/* tooltip */
    G_CALLBACK (swamigui_menu_cb_python) },
#endif

  /* Help Menu */

  { "SwamiTips", GTK_STOCK_HELP, /* name, stock id */
    "Swami _Tips", "",	/* label, accelerator */     
    N_("Get helpful tips on using Swami"),	/* tooltip */  
    G_CALLBACK (swamigui_menu_cb_swamitips) },
  { "SplashImage", GTK_STOCK_INFO,  /* name, stock id */
    "_Splash Image", "",  /* label, accelerator */
    N_("Show splash image"),	/* tooltip */
    G_CALLBACK (swamigui_menu_cb_splash_image) },
  { "About", GTK_STOCK_ABOUT,	/* name, stock id */
    "_About", "",	/* label, accelerator */     
    N_("About Swami"),			/* tooltip */  
    G_CALLBACK (swamigui_help_about) },
};
static guint n_entries = G_N_ELEMENTS (entries);


static const gchar *ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='NewType'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='OpenRecent'/>"
"      <menuitem action='SaveAll'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='PluginsMenu'>"
"      <menuitem action='RestartFluid'/>"
"    </menu>"
/* FIXME - Python disabled until crashing is fixed and binding is updated */
#if 0
"    <menu action='ToolsMenu'>"
#ifdef PYTHON_SUPPORT
"      <menuitem action='Python'/>"
#endif
"    </menu>"
#endif
"    <menu action='HelpMenu'>"
"      <menuitem action='SwamiTips'/>"
"      <menuitem action='SplashImage'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

GType
swamigui_menu_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiguiMenuClass), NULL, NULL,
	(GClassInitFunc) swamigui_menu_class_init, NULL, NULL,
	sizeof (SwamiguiMenu), 0,
	(GInstanceInitFunc) swamigui_menu_init,
      };

    obj_type = g_type_register_static (GTK_TYPE_VBOX, "SwamiguiMenu",
				       &obj_info, 0);
  }

  return (obj_type);
}

static void
swamigui_menu_class_init (SwamiguiMenuClass *klass)
{
  GtkWidgetClass *widg_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widg_class->realize = swamigui_menu_realize;
}

static void
swamigui_menu_init (SwamiguiMenu *guimenu)
{
  GtkActionGroup *actions;
  GtkWidget *new_type_menu;
  GtkWidget *mitem;
  GError *error = NULL;
  GtkWidget *recent_menu;
  GtkRecentManager *manager;
  GtkRecentFilter *filter;

  actions = gtk_action_group_new ("Actions");
  gtk_action_group_add_actions (actions, entries, n_entries, guimenu);

  guimenu->ui = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (guimenu->ui, actions, 0);

  if (!gtk_ui_manager_add_ui_from_string (guimenu->ui, ui_info, -1, &error))
    {
      g_critical ("Building SwamiGuiMenu failed: %s", error->message);
      g_error_free (error);
      return;
    }

  gtk_box_pack_start (GTK_BOX (guimenu),
		      gtk_ui_manager_get_widget (guimenu->ui, "/MenuBar"),
		      FALSE, FALSE, 0);

  /* if last_new_type not set assign it from SwamiguiRoot "default-patch-type"
     property */
  if (!last_new_type)
  {
    g_object_get (swamigui_root, "default-patch-type", &last_new_type, NULL);

    /* also not set?  Just set it to SoundFont type */
    if (last_new_type == G_TYPE_NONE) last_new_type = IPATCH_TYPE_SF2;
  }

  /* set correct label of "New <Last>" menu item */
  last_new_mitem = gtk_ui_manager_get_widget (guimenu->ui, "/MenuBar/FileMenu/New");

  swamigui_menu_update_new_type_item ();

  /* create patch type menu and add to File->"New .." menu item */
  new_type_menu = create_patch_type_menu (guimenu);
  mitem = gtk_ui_manager_get_widget (guimenu->ui, "/MenuBar/FileMenu/NewType");
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), new_type_menu);

  /* Recent chooser menu */
  manager = gtk_recent_manager_get_default ();
  recent_menu = gtk_recent_chooser_menu_new_for_manager (manager);

  /* set the limit to unlimited (FIXME - issues?) */
  gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (recent_menu), -1);

  /* filter recent items to only include those stored by Swami */
  filter = gtk_recent_filter_new ();
  gtk_recent_filter_add_application (filter, "swami");
  gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (recent_menu), filter);

  /* Set the sort type to most recent first */
  gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (recent_menu),
                                    GTK_RECENT_SORT_MRU);

  mitem = gtk_ui_manager_get_widget (guimenu->ui, "/MenuBar/FileMenu/OpenRecent");
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mitem), recent_menu);

  g_signal_connect (recent_menu, "item-activated",
		    G_CALLBACK (swamigui_menu_recent_chooser_item_activated), NULL);
}

/* callback for when the user selects a recent file in the recent files menu */
static void
swamigui_menu_recent_chooser_item_activated (GtkRecentChooser *chooser,
					     gpointer user_data)
{
  char *file_uri, *fname;
  GError *err = NULL;
  GtkWidget *msgdialog;

  file_uri = gtk_recent_chooser_get_current_uri (chooser);
  if (!file_uri) return;

  fname = g_filename_from_uri (file_uri, NULL, NULL);
  g_free (file_uri);

  if (!fname)
    {
      g_critical (_("Failed to parse recent file URI '%s'"), file_uri);
      return;
    }

  if (!swami_root_patch_load (SWAMI_ROOT (swamigui_root), fname, NULL, &err))
  {
    msgdialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Failed to load '%s': %s"),
					fname, ipatch_gerror_message (err));
    g_clear_error (&err);

    if (gtk_dialog_run (GTK_DIALOG (msgdialog)) != GTK_RESPONSE_NONE)
      gtk_widget_destroy (msgdialog);
  }

  g_free (fname);
}

static void
swamigui_menu_realize (GtkWidget *widget)
{
  SwamiguiMenu *guimenu = SWAMIGUI_MENU (widget);
  GtkWidget *toplevel;

  parent_class->realize (widget);

  toplevel = gtk_widget_get_toplevel (widget);

  if (toplevel)
    gtk_window_add_accel_group (GTK_WINDOW (toplevel),
				gtk_ui_manager_get_accel_group (guimenu->ui));
}

static void
swamigui_menu_update_new_type_item (void)
{
  char *name, *s;
  char *free_icon, *icon_name;
  GtkWidget *icon;
  gint category;

  ipatch_type_get (last_new_type, "name", &name, NULL);
  s = g_strdup_printf (_("_New %s"), name);
  g_free (name);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (gtk_bin_get_child
					       (GTK_BIN (last_new_mitem))), s);
  g_free (s);

  /* get icon stock name */
  ipatch_type_get (last_new_type, "icon", &free_icon,
		   "category", &category,
		   NULL);
  if (!free_icon) icon_name = swamigui_icon_get_category_icon (category);
  else icon_name = free_icon;

  icon = gtk_image_new_from_stock (icon_name, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (last_new_mitem), icon);

  if (free_icon) g_free (free_icon);
}

static GtkWidget *
create_patch_type_menu (SwamiguiMenu *guimenu)
{
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *icon;
  GType *types, *ptype;
  char *name, *blurb;
  char *free_icon = NULL, *icon_name;
  guint n_types;
  gint category;

  menu = gtk_menu_new ();

  types = swami_util_get_child_types (IPATCH_TYPE_BASE, &n_types);
  qsort (types, sizeof (GType), n_types, sort_by_type_name);

  for (ptype = types; *ptype; ptype++)
    {
      ipatch_type_get (*ptype,
		       "name", &name,
		       "blurb", &blurb,
		       NULL);
      if (name)
	{
	  item = gtk_image_menu_item_new_with_label (name);

	  /* get icon stock name */
	  ipatch_type_get (*ptype, "icon", &free_icon,
				  "category", &category,
				  NULL);
	  if (!free_icon) icon_name = swamigui_icon_get_category_icon (category);
	  else icon_name = free_icon;

	  icon = gtk_image_new_from_stock (icon_name, GTK_ICON_SIZE_MENU);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
	  if (free_icon) g_free (free_icon);

	  g_object_set_data (G_OBJECT (item), "patch-type",
			     GUINT_TO_POINTER (*ptype));
	  gtk_widget_show_all (item);

	  gtk_container_add (GTK_CONTAINER (menu), item);
	  g_signal_connect (item, "activate",
			    G_CALLBACK (swamigui_menu_cb_new_patch), guimenu);
	}
    }

  g_free (types);

  return (menu);
}

static int
sort_by_type_name (const void *a, const void *b)
{
  GType *atype = (GType *)a, *btype = (GType *)b;
  char *aname, *bname;

  ipatch_type_get (*atype, "name", &aname, NULL);
  ipatch_type_get (*btype, "name", &bname, NULL);

  if (!aname && !bname) return (0);
  else if (!aname) return (1);
  else if (!bname) return (-1);
  else return (strcmp (aname, bname));
}

/**
 * swamigui_menu_new:
 *
 * Create a Swami main menu object.
 *
 * Returns: New Swami menu object.
 */
GtkWidget *
swamigui_menu_new (void)
{
  return (GTK_WIDGET (g_object_new (SWAMIGUI_TYPE_MENU, NULL)));
}

/* main menu callback to create a new patch objects */
static void
swamigui_menu_cb_new_patch (GtkWidget *mitem, gpointer data)
{
  GType patch_type;

  patch_type = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mitem),
						    "patch-type"));
  if (patch_type)
    {
      last_new_type = patch_type;
      swamigui_menu_update_new_type_item ();
    }
  else patch_type = last_new_type;

  swamigui_new_item (NULL, patch_type);
}

static void
swamigui_menu_cb_load_files (GtkWidget *mitem, gpointer data)
{
  SwamiguiRoot *root = swamigui_get_root (data);
  if (root) swamigui_load_files (root);
}

/* main menu callback to save files */
static void
swamigui_menu_cb_save_all (GtkWidget *mitem, gpointer data)
{
  IpatchList *patches;

  /* save them all */
  patches = ipatch_container_get_children (IPATCH_CONTAINER (swami_root->patch_root),
					   IPATCH_TYPE_BASE);
  if (patches)
    {
      if (patches->items) swamigui_save_files (patches, FALSE);
      g_object_unref (patches);
    }
}

static void
swamigui_menu_cb_quit (GtkWidget *mitem, gpointer data)
{
  SwamiguiRoot *root = swamigui_get_root (data);
  if (root) swamigui_root_quit (root);
}

static void
swamigui_menu_cb_preferences (GtkWidget *mitem, gpointer data)
{
  GtkWidget *pref;

  pref = swamigui_util_lookup_unique_dialog ("preferences", 0);

  if (!pref)
  {
    pref = swamigui_pref_new ();
    gtk_widget_show (pref);
    swamigui_util_register_unique_dialog (pref, "preferences", 0);
  }
}

static void
swamigui_menu_cb_swamitips (GtkWidget *mitem, gpointer data)
{
  SwamiguiRoot *root = swamigui_get_root (data);
  if (root) swamigui_help_swamitips_create (root);
}

static void
swamigui_menu_cb_splash_image (GtkWidget *mitem, gpointer data)
{
  swamigui_splash_display (0);
}

#ifdef PYTHON_SUPPORT
static void
swamigui_menu_cb_python (GtkWidget *mitem, gpointer data)
{
  GtkWidget *window;
  GtkWidget *pythonview;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  pythonview = swamigui_python_view_new ();
  gtk_container_add (GTK_CONTAINER (window), pythonview);

  gtk_widget_show_all (window);
}
#endif

static void
swamigui_menu_cb_restart_fluid (GtkWidget *mitem, gpointer data)
{
  /* FIXME - Should be handled by FluidSynth plugin */
  swami_wavetbl_close (swamigui_root->wavetbl);
  swami_wavetbl_open (swamigui_root->wavetbl, NULL);
}
