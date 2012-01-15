/*
 * SwamiguiRoot.c - Swami main GUI object
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <errno.h>

#include <libswami/libswami.h>
#include <libswami/version.h>
#include <libinstpatch/IpatchParamProp.h>
#include <libinstpatch/IpatchXml.h>

#include "swamigui.h"
#include "i18n.h"
#include "../libswami/swami_priv.h"

enum
{
  PROP_0,
  PROP_MAIN_WINDOW,
  PROP_UPDATE_INTERVAL,
  PROP_QUIT_CONFIRM,
  PROP_SPLASH_ENABLE,
  PROP_SPLASH_DELAY,
  PROP_TIPS_ENABLE,
  PROP_TIPS_POSITION,
  PROP_PIANO_LOWER_KEYS,
  PROP_PIANO_UPPER_KEYS,
  PROP_DEFAULT_PATCH_TYPE,
  PROP_TREE_STORE_LIST,
  PROP_SELECTION_ORIGIN,
  PROP_SELECTION,
  PROP_SELECTION_SINGLE,
  PROP_SOLO_ITEM_ENABLE
};

enum
{
  QUIT,
  LAST_SIGNAL
};

/* Default splash delay in milliseconds */
#define SWAMIGUI_ROOT_DEFAULT_SPLASH_DELAY 5000

#define SWAMIGUI_ROOT_DEFAULT_LOWER_KEYS  "z,s,x,d,c,v,g,b,h,n,j,m,comma,l,period,semicolon,slash"
#define SWAMIGUI_ROOT_DEFAULT_UPPER_KEYS  "q,2,w,3,e,r,5,t,6,y,7,u,i,9,o,0,p,bracketleft,equal,bracketright"

/* external private global prototypes */
void _swamigui_ifaces_init (void); /* ifaces/ifaces.c */
void _swamigui_stock_icons_init (void); /* icons.c */
void _swamigui_control_init (void); /* SwamiguiControl.c */
void _swamigui_control_widgets_init (void); /* SwamiguiControl_widgets.c */
void _swamigui_item_menu_init (void);
void _swamigui_item_menu_actions_init (void);	/* SwamiguiItemMenu_actions.c */

#ifdef PYTHON_SUPPORT
void _swamigui_python_init (int argc, char **argv); /* in swami_python.c */
#endif

/* Local Prototypes */

static void swamigui_root_class_init (SwamiguiRootClass *klass);
static void swamigui_root_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec);
static void swamigui_root_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec);
static void swamigui_root_quit_method (SwamiguiRoot *root);

static void swamigui_root_init (SwamiguiRoot *obj);
static void swamigui_root_finalize (GObject *object);
static gboolean swamigui_queue_test_func (SwamiControlQueue *queue,
					  SwamiControl *control,
					  SwamiControlEvent *event);
static gboolean swamigui_update_gui_timeout (gpointer data);

static void ctrl_prop_set_func (SwamiControl *control,
				SwamiControlEvent *event, const GValue *value);
static void ctrl_add_set_func (SwamiControl *control, SwamiControlEvent *event,
			       const GValue *value);
static void ctrl_remove_set_func (SwamiControl *control,
				  SwamiControlEvent *event,
				  const GValue *value);

static void swamigui_cb_quit_response (GtkDialog *dialog, int response,
				      gpointer user_data);
static void swamigui_real_quit (SwamiguiRoot *root);
static void swamigui_root_create_main_window (SwamiguiRoot *root);
static void swamigui_root_cb_solo_item (SwamiguiRoot *root, GParamSpec *pspec,
                                        gpointer user_data);
static void swamigui_root_update_solo_item (SwamiguiRoot *root, GObject *solo_item);
static void swamigui_root_connect_midi_keyboard_controls (SwamiguiRoot *root,
                                                          GObject *midikey);
static gint swamigui_root_cb_main_window_delete (GtkWidget *widget,
						 GdkEvent *event, gpointer data);
static guint *swamigui_root_parse_piano_keys (const char *str);
static char *swamigui_root_encode_piano_keys (const guint *keyvals);
static GtkWidget *sf2_prop_handler (GtkWidget *widg, GObject *obj);
static void current_date_btn_clicked (GtkButton *button, gpointer user_data);

static guint uiroot_signals[LAST_SIGNAL] = { 0 };

SwamiguiRoot *swamigui_root = NULL;
SwamiRoot *swami_root = NULL;

/* global enable bools, kind of hackish, but what is better? */
gboolean swamigui_disable_python =  TRUE;
gboolean swamigui_disable_plugins = FALSE;

static GObjectClass *parent_class = NULL;

/* used to determine if current thread is the GUI thread */
static GStaticPrivate is_gui_thread = G_STATIC_PRIVATE_INIT;


/**
 * swamigui_init:
 * @argc: Number of arguments from main()
 * @argv: Command line arguments from main()
 *
 * Function to initialize Swami User Interface. Should be called before any
 * other Swamigui related functions. This function calls swami_init() as well.
 */
void
swamigui_init (int *argc, char **argv[])
{
  static gboolean initialized = FALSE;

  if (initialized) return;
  initialized = TRUE;

  /* initialize glib thread support (if it hasn't been already!) */
  if (!g_thread_supported ()) g_thread_init (NULL);

  gtk_set_locale ();
  gtk_init_check (argc, argv);	/* initialize GTK */

  /* set the application/program name, so things work right with recent file chooser
   * even when binary name is different (such as lt-swami) */
  g_set_application_name ("swami");

  swami_init ();

  /* install icon parameter property (for assigning icons to effect controls) */
  ipatch_param_install_property
    (g_param_spec_string ("icon", "Icon", "Icon",
			  NULL, G_PARAM_READWRITE));

  /* install icon property for types (for assigning icons to object types) */
  ipatch_type_install_property
    (g_param_spec_string ("icon", "Icon", "Icon",
			  NULL, G_PARAM_READWRITE));

  /* set type specific icons */
  ipatch_type_set (IPATCH_TYPE_DLS2, "icon", SWAMIGUI_STOCK_DLS, NULL);
  ipatch_type_set (IPATCH_TYPE_GIG, "icon", SWAMIGUI_STOCK_GIG, NULL);
  ipatch_type_set (IPATCH_TYPE_SF2, "icon", SWAMIGUI_STOCK_SOUNDFONT, NULL);

  swamigui_util_init ();
  _swamigui_stock_icons_init ();
  _swamigui_control_init ();
  _swamigui_control_widgets_init ();
  _swamigui_item_menu_init ();


  /* initialize Swamigui types */
  swamigui_bar_get_type ();
  swamigui_bar_ptr_get_type ();
  swamigui_bar_ptr_type_get_type ();
  swamigui_control_adj_get_type ();
  swamigui_control_flags_get_type ();
  swamigui_control_midi_key_get_type ();
  swamigui_control_object_flags_get_type ();
  swamigui_control_rank_get_type ();
  swamigui_item_menu_flags_get_type ();
  swamigui_item_menu_get_type ();
  swamigui_knob_get_type ();
  swamigui_menu_get_type ();
  swamigui_mod_edit_get_type ();
  swamigui_panel_get_type ();
  swamigui_panel_selector_get_type ();
  swamigui_panel_sf2_gen_get_type ();
  swamigui_paste_decision_get_type ();
  swamigui_paste_get_type ();
  swamigui_paste_status_get_type ();
  swamigui_piano_get_type ();
  swamigui_pref_get_type ();
  swamigui_prop_get_type ();
  swamigui_quit_confirm_get_type ();
  swamigui_root_get_type ();
  swamigui_sample_canvas_get_type ();
  swamigui_sample_editor_get_type ();
  swamigui_sample_editor_marker_flags_get_type ();
  swamigui_sample_editor_marker_id_get_type ();
  swamigui_sample_editor_status_get_type ();
  swamigui_spectrum_canvas_get_type ();
  swamigui_spin_scale_get_type ();
  swamigui_splits_get_type ();
  swamigui_splits_mode_get_type ();
  swamigui_splits_status_get_type ();
  swamigui_statusbar_get_type ();
  swamigui_statusbar_pos_get_type ();
  swamigui_tree_get_type ();
  swamigui_tree_store_get_type ();
  swamigui_tree_store_patch_get_type ();
  swamigui_util_unit_rgba_color_get_type ();

  _swamigui_item_menu_actions_init ();

  /* Register panel types and their order */
  swamigui_register_panel_selector_type (SWAMIGUI_TYPE_PROP, 80);
  swamigui_register_panel_selector_type (SWAMIGUI_TYPE_SAMPLE_EDITOR, 90);
  swamigui_register_panel_selector_type (SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC, 100);
  swamigui_register_panel_selector_type (SWAMIGUI_TYPE_PANEL_SF2_GEN_ENV, 105);
  swamigui_register_panel_selector_type (SWAMIGUI_TYPE_MOD_EDIT, 110);

  /* Register prop widget types */
  swamigui_register_prop_handler (IPATCH_TYPE_SF2, sf2_prop_handler);
  swamigui_register_prop_glade_widg (IPATCH_TYPE_SF2_PRESET, "PropSF2Preset");
  swamigui_register_prop_glade_widg (IPATCH_TYPE_SF2_INST, "PropSF2Inst");
  swamigui_register_prop_glade_widg (IPATCH_TYPE_SF2_IZONE, "PropSF2IZone");
  swamigui_register_prop_glade_widg (IPATCH_TYPE_SF2_SAMPLE, "PropSF2Sample");

#ifdef PYTHON_SUPPORT
  if (!swamigui_disable_python)
  {
    _swamigui_python_init (*argc, *argv);
    swamigui_python_view_get_type ();
  }
#endif

  if (!swamigui_disable_plugins)
    swami_plugin_load_all (); /* load plugins */
}

/* Function used by libglade to initialize libswamigui widgets */
void
glade_module_register_widgets (void)
{
  swamigui_init (0, NULL);
}

GType
swamigui_root_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
    {
      static const GTypeInfo item_info =
	{
	  sizeof (SwamiguiRootClass), NULL, NULL,
	  (GClassInitFunc) swamigui_root_class_init, NULL, NULL,
	  sizeof (SwamiguiRoot), 0,
	  (GInstanceInitFunc) swamigui_root_init,
	};

      item_type = g_type_register_static (SWAMI_TYPE_ROOT, "SwamiguiRoot",
					  &item_info, 0);
    }

  return (item_type);
}

static void
swamigui_root_class_init (SwamiguiRootClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_root_set_property;
  obj_class->get_property = swamigui_root_get_property;
  obj_class->finalize = swamigui_root_finalize;

  klass->quit = swamigui_root_quit_method;

  uiroot_signals[QUIT] =
    g_signal_new ("quit", G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (SwamiguiRootClass, quit), NULL, NULL,
		  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_object_class_install_property (obj_class, PROP_MAIN_WINDOW,
				   g_param_spec_object ("main-window",
							_("Main window"),
							_("Main window"),
							GTK_TYPE_WIDGET,
							G_PARAM_READABLE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_UPDATE_INTERVAL,
		g_param_spec_int ("update-interval",
				  _("Update interval"),
				  _("GUI update interval in milliseconds"),
				  10, 1000, 100, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_QUIT_CONFIRM,
			g_param_spec_enum ("quit-confirm",
					   _("Quit confirm"),
					   _("Quit confirmation method"),
					   SWAMIGUI_TYPE_QUIT_CONFIRM,
					   SWAMIGUI_QUIT_CONFIRM_UNSAVED,
					   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SPLASH_ENABLE,
			g_param_spec_boolean ("splash-enable",
					      _("Splash image enable"),
					      _("Show splash on startup"),
					      TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SPLASH_DELAY,
			g_param_spec_uint ("splash-delay",
					   _("Splash delay"),
					   _("Splash delay in milliseconds (0 to wait for button click)"),
					   0, G_MAXUINT, SWAMIGUI_ROOT_DEFAULT_SPLASH_DELAY,
			                   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TIPS_ENABLE,
			g_param_spec_boolean ("tips-enable",
					      _("Tips enable"),
					      _("Show tips on startup"),
					      TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TIPS_POSITION,
			g_param_spec_int ("tips-position",
					  "Tips position",
					  "Tips position",
					  0, 255, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PIANO_LOWER_KEYS,
		g_param_spec_string ("piano-lower-keys",
				     "Piano lower keys",
				     "Comma delimited list of GDK key names",
				     SWAMIGUI_ROOT_DEFAULT_LOWER_KEYS,
				     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PIANO_UPPER_KEYS,
		g_param_spec_string ("piano-upper-keys",
				     "Piano upper keys",
				     "Comma delimited list of GDK key names",
				     SWAMIGUI_ROOT_DEFAULT_UPPER_KEYS,
				     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_DEFAULT_PATCH_TYPE,
				   g_param_spec_gtype ("default-patch-type",
						       "Default patch type",
						       "Default patch type",
						       IPATCH_TYPE_BASE,
						       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_TREE_STORE_LIST,
		g_param_spec_object ("tree-store-list", "Tree store list",
				     "List of tree stores",
				     IPATCH_TYPE_LIST,
				     G_PARAM_READABLE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_SELECTION_ORIGIN,
		g_param_spec_object ("selection-origin", "Selection origin",
				     "Origin of selection",
				     G_TYPE_OBJECT,
				     G_PARAM_READABLE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_SELECTION,
		g_param_spec_object ("selection", "Item selection",
				     "Last item selection",
				     IPATCH_TYPE_LIST,
				     G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_SELECTION_SINGLE,
		g_param_spec_object ("selection-single", "Single item selection",
				     "Last single selected item",
				     G_TYPE_OBJECT,
				     G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, PROP_SOLO_ITEM_ENABLE,
		g_param_spec_boolean ("solo-item-enable", "Solo item enable",
				      "Enable solo audition of active instrument",
				      FALSE, G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
}

static void
swamigui_root_set_property (GObject *object, guint property_id,
			    const GValue *value, GParamSpec *pspec)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (object);
  IpatchList *list;
  GObject *obj = NULL;
  int ival;

  switch (property_id)
    {
    case PROP_UPDATE_INTERVAL:
      ival = g_value_get_int (value);

      if (ival != root->update_interval)
      {
	root->update_interval = ival;

        /* remove old timeout and install new one */
        if (root->update_timeout_id) g_source_remove (root->update_timeout_id);
        root->update_timeout_id = g_timeout_add (root->update_interval,
				  (GSourceFunc)swamigui_update_gui_timeout, root);
      }
      break;
    case PROP_QUIT_CONFIRM:
      root->quit_confirm = g_value_get_enum (value);
      break;
    case PROP_SPLASH_ENABLE:
      root->splash_enable = g_value_get_boolean (value);
      break;
    case PROP_SPLASH_DELAY:
      root->splash_delay = g_value_get_uint (value);
      break;
    case PROP_TIPS_ENABLE:
      root->tips_enable = g_value_get_boolean (value);
      break;
    case PROP_TIPS_POSITION:
      root->tips_position = g_value_get_int (value);
      break;
    case PROP_PIANO_LOWER_KEYS:
      g_free (root->piano_lower_keys);
      root->piano_lower_keys =
	swamigui_root_parse_piano_keys (g_value_get_string (value));
      break;
    case PROP_PIANO_UPPER_KEYS:
      g_free (root->piano_upper_keys);
      root->piano_upper_keys =
	swamigui_root_parse_piano_keys (g_value_get_string (value));
      break;
    case PROP_DEFAULT_PATCH_TYPE:
      root->default_patch_type = g_value_get_gtype (value);
      break;
    case PROP_TREE_STORE_LIST:
      if (root->tree_stores) g_object_unref (root->tree_stores);
      root->tree_stores = IPATCH_LIST (g_value_dup_object (value));
      break;
    case PROP_SELECTION:
      if (root->selection) g_object_unref (root->selection);
      obj = g_value_dup_object (value);
      if (obj) root->selection = IPATCH_LIST (obj);
      else root->selection = NULL;

      g_object_notify (G_OBJECT (root), "selection-single");
      break;
    case PROP_SELECTION_SINGLE:
      if (root->selection) g_object_unref (root->selection);
      obj = g_value_dup_object (value);

      if (obj)
	{
	  list = ipatch_list_new ();
	  list->items = g_list_append (list->items, obj);
	  root->selection = list;
	}
      else root->selection = NULL;

      g_object_notify (G_OBJECT (root), "selection");
      break;
    case PROP_SOLO_ITEM_ENABLE:
      if (root->solo_item_enabled != g_value_get_boolean (value))
      {
        root->solo_item_enabled = !root->solo_item_enabled;

        if (root->solo_item_enabled && root->selection && root->selection->items
	    && !root->selection->items->next)
	  obj = root->selection->items->data;

        swamigui_root_update_solo_item (root, obj);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_root_get_property (GObject *object, guint property_id,
			    GValue *value, GParamSpec *pspec)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (object);
  GObject *obj = NULL;

  switch (property_id)
    {
    case PROP_MAIN_WINDOW:
      g_value_set_object (value, G_OBJECT (root->main_window));
      break;
    case PROP_UPDATE_INTERVAL:
      g_value_set_int (value, root->update_interval);
      break;
    case PROP_QUIT_CONFIRM:
      g_value_set_enum (value, root->quit_confirm);
      break;
    case PROP_SPLASH_ENABLE:
      g_value_set_boolean (value, root->splash_enable);
      break;
    case PROP_SPLASH_DELAY:
      g_value_set_uint (value, root->splash_delay);
      break;
    case PROP_TIPS_ENABLE:
      g_value_set_boolean (value, root->tips_enable);
      break;
    case PROP_TIPS_POSITION:
      g_value_set_int (value, root->tips_position);
      break;
    case PROP_PIANO_LOWER_KEYS:
      g_value_set_string_take_ownership (value, swamigui_root_encode_piano_keys
					 (root->piano_lower_keys));
      break;
    case PROP_PIANO_UPPER_KEYS:
      g_value_set_string_take_ownership (value, swamigui_root_encode_piano_keys
					 (root->piano_upper_keys));
      break;
    case PROP_DEFAULT_PATCH_TYPE:
      g_value_set_gtype (value, root->default_patch_type);
      break;
    case PROP_TREE_STORE_LIST:
      g_value_set_object (value, root->tree_stores);
      break;
    case PROP_SELECTION_ORIGIN:
      if (root->selection)
	  obj = swami_object_get_origin (G_OBJECT (root->selection));

      g_value_take_object (value, obj);
      break;
    case PROP_SELECTION:
      g_value_set_object (value, root->selection);
      break;
    case PROP_SELECTION_SINGLE:
      if (root->selection && root->selection->items
	  && !root->selection->items->next)
	obj = root->selection->items->data;

      g_value_set_object (value, obj);
      break;
    case PROP_SOLO_ITEM_ENABLE:
      g_value_set_boolean (value, root->solo_item_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_root_quit_method (SwamiguiRoot *root)
{
  swamigui_root_save_prefs (root);
  gtk_main_quit ();
}

/* Additional init operations are performed in swamigui_root_activate() */
static void
swamigui_root_init (SwamiguiRoot *root)
{
  IpatchList *list;
  GType type;

  /* having global vars for the Swami instance makes things easier */
  swamigui_root = root;
  swami_root = SWAMI_ROOT (root);

#ifdef PYTHON_SUPPORT
  if (!swamigui_disable_python)
    swamigui_python_set_root ();
#endif

  root->update_interval = 40;
  root->quit_confirm = SWAMIGUI_QUIT_CONFIRM_UNSAVED;
  root->splash_enable = TRUE;
  root->splash_delay = SWAMIGUI_ROOT_DEFAULT_SPLASH_DELAY;
  root->tips_enable = TRUE;
  root->default_patch_type = IPATCH_TYPE_SF2;

  root->piano_lower_keys
    = swamigui_root_parse_piano_keys (SWAMIGUI_ROOT_DEFAULT_LOWER_KEYS);
  root->piano_upper_keys
    = swamigui_root_parse_piano_keys (SWAMIGUI_ROOT_DEFAULT_UPPER_KEYS);

  swami_object_set (G_OBJECT (root),
		    "name", "Swami",
		    "flags", SWAMI_OBJECT_SAVE | SWAMI_OBJECT_USER,
		    NULL);

  /* create tree store */
  root->patch_store = SWAMIGUI_TREE_STORE
    (swamigui_tree_store_patch_new ()); /* ++ ref */
  swami_object_set (G_OBJECT (root->patch_store), "name", "Patches", NULL);
  swami_root_add_object (SWAMI_ROOT (root), G_OBJECT (root->patch_store));

  /* create GUI control queue */
  root->ctrl_queue = swami_control_queue_new (); /* ++ ref control queue */

  /* setup the GUI queue test function and is_gui_thread private */
  g_static_private_set (&is_gui_thread, GUINT_TO_POINTER (TRUE), NULL);
  swami_control_queue_set_test_func (root->ctrl_queue,
				     swamigui_queue_test_func);

  /* create queued patch item property changed listener */
  root->ctrl_prop = swami_control_func_new (); /* ++ ref new control */
  swami_control_func_assign_funcs (root->ctrl_prop, NULL /* get_func */,
				   ctrl_prop_set_func, NULL /* destroy_func */,
				   root);

  /* this control will never send (receives events only), disables event loop check */
  SWAMI_CONTROL (root->ctrl_prop)->flags &= ~SWAMI_CONTROL_SENDS;

  /* create queued patch item add control listener */
  root->ctrl_add = swami_control_func_new (); /* ++ ref new control */
  swami_control_func_assign_funcs (root->ctrl_add, NULL /* get_func */,
				   ctrl_add_set_func, NULL /* destroy_func */,
				   root);

  /* this control will never send (receives events only), disables event loop check */
  SWAMI_CONTROL (root->ctrl_add)->flags &= ~SWAMI_CONTROL_SENDS;

  /* create queued patch item remove control listener */
  root->ctrl_remove = swami_control_func_new (); /* ++ ref new control */
  swami_control_func_assign_funcs (root->ctrl_remove, NULL /* get_func */,
				   ctrl_remove_set_func,
				   NULL /* destroy_func */, root);

  /* this control will never send (receives events only), disables event loop check */
  SWAMI_CONTROL (root->ctrl_remove)->flags &= ~SWAMI_CONTROL_SENDS;

  /* set the queue of the controls */
  swami_control_set_queue (SWAMI_CONTROL (root->ctrl_prop), root->ctrl_queue);
  swami_control_set_queue (SWAMI_CONTROL (root->ctrl_add), root->ctrl_queue);
  swami_control_set_queue (SWAMI_CONTROL (root->ctrl_remove),
			   root->ctrl_queue);

  /* connect controls to ipatch event senders */
  swami_control_connect (swami_patch_prop_title_control,
			 SWAMI_CONTROL (root->ctrl_prop), 0);
  swami_control_connect (swami_patch_add_control,
			 SWAMI_CONTROL (root->ctrl_add), 0);
  swami_control_connect (swami_patch_remove_control,
			 SWAMI_CONTROL (root->ctrl_remove), 0);

  /* add patch store to tree store list */
  list = ipatch_list_new ();
  list->items = g_list_append (list->items, g_object_ref (root->patch_store));
  root->tree_stores = list;

  /* create the FluidSynth wavetable object */
  type = swami_type_get_default (SWAMI_TYPE_WAVETBL);

  /* create new wavetable object */
  if (type && (root->wavetbl = g_object_new (type, NULL)))
  {
    swami_object_set (root->wavetbl, "name", "FluidSynth1", NULL);
    swami_root_add_object (SWAMI_ROOT (root), G_OBJECT (root->wavetbl));
  }
}

static void
swamigui_root_finalize (GObject *object)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (object);

  g_object_unref (root->patch_store);
  g_object_unref (root->tree_stores);

  if (root->selection) g_object_unref (root->selection);
  if (root->wavetbl) g_object_unref (root->wavetbl);

  if (root->solo_item) g_object_unref (root->solo_item);
  if (root->solo_item_icon) g_free (root->solo_item_icon);

  g_object_unref (root->ctrl_queue);

  if (root->update_timeout_id)
    g_source_remove (root->update_timeout_id);

  g_object_unref (root->ctrl_prop);
  g_object_unref (root->ctrl_add);
  g_object_unref (root->ctrl_remove);

  if (root->loaded_xml_config) ipatch_xml_destroy (root->loaded_xml_config);

  if (parent_class)
    parent_class->finalize (object);
}

/* GUI queue test function (checks if queuing of events is required) */
static gboolean
swamigui_queue_test_func (SwamiControlQueue *queue, SwamiControl *control,
			  SwamiControlEvent *event)
{ /* are we in the GUI thread? */
  gboolean bval = GPOINTER_TO_UINT (g_static_private_get (&is_gui_thread));
  return (!bval);	/* FALSE sends event immediately, TRUE queues */
}

/* routine called at regular intervals to synchronize GUI with patch objects */
static gboolean
swamigui_update_gui_timeout (gpointer data)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (data);

  swami_control_queue_run (root->ctrl_queue);

  /* Update splits widget if its splits-item has changed */
  if (root->splits_changed)
  {
    swamigui_splits_item_changed (SWAMIGUI_SPLITS (root->splits));
    root->splits_changed = FALSE;
  }

  return (TRUE);
}

/* patch item title property change control value set function (listens for
   item property change events) */
static void
ctrl_prop_set_func (SwamiControl *control, SwamiControlEvent *event,
		    const GValue *value)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (SWAMI_CONTROL_FUNC_DATA (control));
  SwamiEventPropChange *prop_change;

  prop_change = g_value_get_boxed (&event->value);
  swamigui_tree_store_changed (root->patch_store, prop_change->object);
}

/* patch item add control value set function (listens for item add events) */
static void
ctrl_add_set_func (SwamiControl *control, SwamiControlEvent *event,
		   const GValue *value)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (SWAMI_CONTROL_FUNC_DATA (control));
  IpatchItem *item, *splits_item;

  item = IPATCH_ITEM (g_value_get_boxed (&event->value));
  swamigui_tree_store_add (root->patch_store, G_OBJECT (item));

  /* Update splits widget if its active item changed */
  g_object_get (root->splits, "splits-item", &splits_item, NULL);
  if (splits_item == ipatch_item_peek_parent (item))
    root->splits_changed = TRUE;
}

/* patch item remove control value set function (listens for item remove
   events) */
static void
ctrl_remove_set_func (SwamiControl *control, SwamiControlEvent *event,
		      const GValue *value)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (SWAMI_CONTROL_FUNC_DATA (control));
  SwamiEventItemRemove *remove;
  IpatchItem *splits_item;

  remove = g_value_get_boxed (&event->value);
  swamigui_tree_store_remove (root->patch_store, G_OBJECT (remove->item));

  /* Update splits widget if its active item changed */
  g_object_get (root->splits, "splits-item", &splits_item, NULL);
  if (splits_item == ipatch_item_peek_parent (remove->item))
    root->splits_changed = TRUE;
}

/**
 * swamigui_root_new:
 *
 * Create a new Swami root user interface object which is a subclass of
 * #SwamiRoot.
 *
 * Returns: New Swami user interface object
 */
SwamiguiRoot *
swamigui_root_new (void)
{
  return (SWAMIGUI_ROOT (g_object_new (SWAMIGUI_TYPE_ROOT, NULL)));
}

/**
 * swamigui_root_activate:
 * @root: Swami GUI root object
 *
 * Activates Swami GUI by creating main window, loading plugins, and displaying
 * tips and splash image.  Separate from object init function so that preferences
 * can be loaded or not.
 */
void
swamigui_root_activate (SwamiguiRoot *root)
{
  GError *err = NULL;

  g_return_if_fail (SWAMIGUI_IS_ROOT (root));

  /* GUI update timeout */
  root->update_timeout_id = g_timeout_add (root->update_interval,
			(GSourceFunc)swamigui_update_gui_timeout, root);

  if (root->wavetbl)
  {
    if (!swami_wavetbl_open (SWAMI_WAVETBL (root->wavetbl), &err))
    {
      g_warning ("Failed to initialize wavetable driver '%s'",
                 ipatch_gerror_message (err));
      g_clear_error (&err);
    }
  }

  /* create main window and controls */
  swamigui_root_create_main_window (root);

  /* pop up swami tip window if enabled */
  if (root->tips_enable) swamigui_help_swamitips_create (root);

  /* display splash (with timeout) only if not disabled */
  if (root->splash_enable) swamigui_splash_display (root->splash_delay);
}

/**
 * swamigui_root_quit:
 * @root: Swami GUI root object
 *
 * Quit Swami GUI. Pops a quit confirmation depending on preferences.
 */
void
swamigui_root_quit (SwamiguiRoot *root)
{
  IpatchList *list;
  IpatchIter iter;
  GObject *obj;
  gboolean changed = FALSE;
  GtkWidget *popup;
  int quit_confirm;
  char *s;

  list = swami_root_get_patch_items (SWAMI_ROOT (root)); /* ++ ref list */
  ipatch_list_init_iter (list, &iter);
  obj = ipatch_iter_first (&iter);
  while (obj)
    {
      g_object_get (obj, "changed", &changed, NULL);
      if (changed) break;
      obj = ipatch_iter_next (&iter);
    }
  g_object_unref (list);	/* -- unref list */

  g_object_get (root, "quit-confirm", &quit_confirm, NULL);

  if (quit_confirm == SWAMIGUI_QUIT_CONFIRM_NEVER
      || (quit_confirm == SWAMIGUI_QUIT_CONFIRM_UNSAVED && !changed))
    {
      swamigui_real_quit (root);
      return;
    }

  if (changed) s = _("Unsaved files, and you want to quit?");
  else s = _("Are you sure you want to quit?");

  popup = gtk_message_dialog_new (NULL, 0,
				  GTK_MESSAGE_QUESTION,
				  GTK_BUTTONS_NONE, "%s", s);
  gtk_dialog_add_buttons (GTK_DIALOG (popup),
			  GTK_STOCK_CANCEL,
			  GTK_RESPONSE_CANCEL,
			  GTK_STOCK_QUIT,
			  GTK_RESPONSE_OK,
			  NULL);
  g_signal_connect (popup, "response",
		    G_CALLBACK (swamigui_cb_quit_response), root);
  gtk_widget_show (popup);
}

static void
swamigui_cb_quit_response (GtkDialog *dialog, int response, gpointer user_data)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (user_data);

  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response == GTK_RESPONSE_OK) swamigui_real_quit (root);
}

static void
swamigui_real_quit (SwamiguiRoot *root)
{
  g_signal_emit (root, uiroot_signals [QUIT], 0);
}

/**
 * swamigui_root_save_prefs:
 * @root: Swami GUI root object
 *
 * Save application preferences to XML config file.  Only stores config if it
 * differs from existing stored config on disk.
 *
 * Returns: %TRUE on success, %FALSE otherwise (error message will be logged)
 */
gboolean
swamigui_root_save_prefs (SwamiguiRoot *root)
{
  char *config_dir, *pref_filename;
  GError *local_err = NULL;
  GNode *xmltree, *node, *pnode, *dupnode;
  GList *plugins, *p;
  SwamiPlugin *plugin;
  char *xmlstr;
  char *curxml;
  const char *attr_val;

  xmltree = ipatch_xml_new_node (NULL, "swami", NULL,	/* ++ alloc XML tree */
                                 "version", SWAMI_VERSION,
                                 NULL);
  /* Encode the root object */
  if (!ipatch_xml_encode_object (xmltree, G_OBJECT (root), FALSE, &local_err))
  {
    g_critical (_("Failed to save Swami preferences: %s"),
                ipatch_gerror_message (local_err));
    g_clear_error (&local_err);
    ipatch_xml_destroy (xmltree);		/* -- free XML tree */
    return (FALSE);
  }

  /* Encode plugin preferences */
  plugins = swami_plugin_get_list ();	/* ++ alloc list */

  for (p = plugins; p; p = p->next)
  {
    plugin = (SwamiPlugin *)(p->data);

    if (plugin->save_xml)
    {
      node = ipatch_xml_new_node (xmltree, "plugin", NULL,
                                  "name", G_TYPE_MODULE (plugin)->name,
                                  NULL);

      if (!swami_plugin_save_xml (plugin, node, &local_err))
      {
	g_warning (_("Failed to save plugin %s preferences: %s"),
	           G_TYPE_MODULE (plugin)->name, ipatch_gerror_message (local_err));
	ipatch_xml_destroy (node);	/* -- free the plugin node */
	g_clear_error (&local_err);
      }
    }
  }

  g_list_free (plugins);	/* -- free list */

  /* Check if there is any plugin data which was loaded at startup which is no
   * longer present and append it to the config if so.  In particular this
   * prevents plugin preferences from being lost if they are not loaded at
   * startup. */
  if (root->loaded_xml_config)
  {
    for (node = root->loaded_xml_config->children; node; node = node->next)
    {
      if (!ipatch_xml_test_name (node, "plugin")) continue;

      attr_val = ipatch_xml_get_attribute (node, "name");
      if (!attr_val) continue;

      for (pnode = xmltree->children; pnode; pnode = pnode->next)
        if (ipatch_xml_test_name (pnode, "plugin")
            && ipatch_xml_test_attribute (pnode, "name", attr_val))
          break;

      if (!pnode)       /* Not found in new XML config? */
      {
        dupnode = ipatch_xml_copy (node);       /* Duplicate the plugin info */
        g_node_append (xmltree, dupnode);       /* Append to xmltree */
      }
    }
  }

  config_dir = g_build_filename (g_get_user_config_dir(), "swami", NULL);	/* ++ alloc */

  if (!g_file_test (config_dir, G_FILE_TEST_EXISTS))
  {
    if (g_mkdir_with_parents (config_dir, 0755) == -1)
    {
      g_critical (_("Failed to create Swami config directory '%s': %s"),
                  config_dir, g_strerror (errno));
      g_free (config_dir);	/* -- free */
      ipatch_xml_destroy (xmltree);		/* -- free XML tree */
      return (FALSE);
    }
  }

  pref_filename = g_build_filename (config_dir, "preferences.xml", NULL);	/* ++ alloc */
  g_free (config_dir);	/* -- free config dir */

  xmlstr = ipatch_xml_to_str (xmltree, 2);      /* ++ alloc xmlstr */
  ipatch_xml_destroy (xmltree);	/* -- free XML tree */

  /* Check if config matches what is already saved - return TRUE if it matches */
  if (g_file_get_contents (pref_filename, &curxml, NULL, NULL))   /* ++ alloc curxml data */
  {
    if (strcmp (xmlstr, curxml) == 0)
    {
      g_free (curxml);          /* -- free current XML data */
      g_free (xmlstr);          /* -- free xmlstr */
      g_free (pref_filename);	/* -- free pref_filename */
      return (TRUE);
    }

    g_free (curxml);            /* -- free current XML data */
  }

  if (!g_file_set_contents (pref_filename, xmlstr, -1, &local_err))
  {
    g_critical (_("Failed to save XML preferences to '%s': %s"),
                pref_filename, ipatch_gerror_message (local_err));
    g_clear_error (&local_err);
    g_free (xmlstr);            /* -- free xmlstr */
    g_free (pref_filename);	/* -- free */
    return (FALSE);
  }

  g_free (xmlstr);              /* -- free xmlstr */
  g_free (pref_filename);	/* -- free */

  return (TRUE);
}

/**
 * swamigui_root_load_prefs:
 * @root: Swami GUI root object
 *
 * Restore application preferences from XML config files.
 *
 * Returns: %TRUE on success, %FALSE otherwise (error message will be logged)
 */
gboolean
swamigui_root_load_prefs (SwamiguiRoot *root)
{
  GNode *xmltree, *n;
  char *pref_filename;
  GParamSpec *pspec;
  GObjectClass *obj_class;
  SwamiPlugin *plugin;
  GError *err = NULL;
  const char *name;

  /* ++ alloc */
  pref_filename = g_build_filename (g_get_user_config_dir(), "swami",
                                    "preferences.xml", NULL);

  /* No preferences stored yet? - Return success */
  if (!g_file_test (pref_filename, G_FILE_TEST_EXISTS))
    return (TRUE);

  xmltree = ipatch_xml_load_from_file (pref_filename, &err);	/* ++ alloc XML tree */

  if (!xmltree)
  {
    g_critical (_("Failed to load preferences from '%s': %s"), pref_filename,
                ipatch_gerror_message (err));
    g_clear_error (&err);
    g_free (pref_filename);	/* -- free filename */
    return (FALSE);
  }

  name = ipatch_xml_get_name (xmltree);

  if (!name || strcmp (name, "swami") != 0)
  {
    g_critical (_("File '%s' is not a Swami preferences file"), pref_filename);
    ipatch_xml_destroy (xmltree);	/* -- destroy XML tree */
    g_free (pref_filename);	/* -- free filename */
    return (FALSE);
  }

  obj_class = G_OBJECT_GET_CLASS (root);

  for (n = xmltree->children; n; n = n->next)
  {
    name = ipatch_xml_get_name (n);
    if (!name) continue;

    if (strcmp (name, "prop") == 0)	/* Root preference value */
    {
      name = ipatch_xml_get_attribute (n, "name");
      if (!name) continue;

      pspec = g_object_class_find_property (obj_class, name);

      if (!pspec || (pspec->flags & IPATCH_PARAM_NO_SAVE))
      {
	g_warning (_("Invalid Swami property '%s' in preferences"), name);
	continue;
      }

      if (!ipatch_xml_decode_property (n, G_OBJECT (root), pspec, &err))
      {
	g_critical (_("Failed to decode Swami preference property '%s': %s"),
	            pspec->name, ipatch_gerror_message (err));
	g_clear_error (&err);
	continue;
      }
    }
    else if (strcmp (name, "plugin") == 0)	/* Plugin state */
    {
      name = ipatch_xml_get_attribute (n, "name");
      if (!name) continue;

      plugin = swami_plugin_find (name);

      if (plugin)
      {
	if (!swami_plugin_load_xml (plugin, n, &err))
	{
	  g_critical (_("Failed to load plugin '%s' preferences: %s"),
	              name, ipatch_gerror_message (err));
	  g_clear_error (&err);
	}
      }
    }
  }

  g_free (pref_filename);	/* -- free filename */

  /* Set the loaded_xml_config variable so it reflects the last loaded XML config */
  if (root->loaded_xml_config) ipatch_xml_destroy (root->loaded_xml_config);
  root->loaded_xml_config = xmltree;    /* !! root takes over reference */

  return (TRUE);
}

/**
 * swamigui_get_root:
 * @gobject: An object registered to a #SwamiguiRoot object
 *
 * Gets a #SwamiguiObject associated with a @gobject. A convenience function
 * really as swami_get_root() will return the same object but casted to
 * #SwamiRoot instead.
 *
 * Returns: A #SwamiguiRoot object or %NULL if @gobject not registered to one.
 * Returned object's refcount is not incremented.
 */
SwamiguiRoot *
swamigui_get_root (gpointer gobject)
{
  SwamiguiRoot *root;

  g_return_val_if_fail (G_IS_OBJECT (gobject), NULL);

  if (SWAMIGUI_IS_ROOT (gobject)) root = (SwamiguiRoot *)gobject;
  else root = (SwamiguiRoot *)swami_get_root (G_OBJECT (gobject));

  /* if no root object is assigned to 'gobject' then just return the global */
  if (!root) return (swamigui_root);

  return (root);
}

/* Create main window and controls */
static void
swamigui_root_create_main_window (SwamiguiRoot *root)
{
  SwamiPropTree *proptree;
  SwamiControl *ctrl, *rootsel_ctrl;
  SwamiControl *store_ctrl;
  GtkWidget *vbox;
  GtkWidget *tempbox;
  GtkWidget *vpaned, *hpaned;
  GtkWidget *widg;
  GType type;
  SwamiControl *midihub;
  SwamiguiControlMidiKey *midikey;
  GObject *piano;
  SwamiControl *pctrl, *tctrl;
  SwamiControlMidi *wctrl;

  /* get root selection property control */
  rootsel_ctrl = swami_get_control_prop_by_name (G_OBJECT (root), "selection");

  root->main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (root->main_window), 1024, 768);
  gtk_window_set_title (GTK_WINDOW (root->main_window), "Swami");

  g_signal_connect (root->main_window, "delete_event",
		    G_CALLBACK (swamigui_root_cb_main_window_delete), root);

  /* add the item menu keyboard accelerators */
  gtk_window_add_accel_group (GTK_WINDOW (root->main_window),
			      swamigui_item_menu_accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (root->main_window), vbox);

  hpaned = gtk_hpaned_new ();
  gtk_widget_show (hpaned);
  gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

  tempbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (tempbox);
  gtk_paned_pack1 (GTK_PANED (hpaned), tempbox, TRUE, TRUE);

  widg = swamigui_menu_new ();
  gtk_widget_show (widg);
  gtk_box_pack_start (GTK_BOX (tempbox), widg, FALSE, FALSE, 0);

  root->tree = swamigui_tree_new (root->tree_stores);
  gtk_widget_show (root->tree);
  gtk_box_pack_start (GTK_BOX (tempbox), root->tree, TRUE, TRUE, 0);

  /* connect tree selection to root selection (++ ref ctrl) */
  ctrl = swami_get_control_prop_by_name (G_OBJECT (root->tree), "selection");
  swami_control_connect (ctrl, rootsel_ctrl, SWAMI_CONTROL_CONN_BIDIR);
  g_object_unref (ctrl);	/* -- unref tree control */

  /* HACK - Set initial selection (so that swamigui_root "selection" is not NULL)
   * We need an empty selection, since it has the origin of the tree.
   * So no problems occur with tree menu accelerators. */
  swamigui_tree_set_selection (SWAMIGUI_TREE (root->tree), NULL);

  vpaned = gtk_vpaned_new ();
  gtk_widget_show (vpaned);
  gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, TRUE);

  tempbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (tempbox);
  gtk_paned_pack1 (GTK_PANED (vpaned), tempbox, TRUE, TRUE);

  /* create FluidSynth interface */
  type = g_type_from_name ("FluidSynthGuiControl");
  if (type)
  {
    widg = g_object_new (type, NULL);
    gtk_widget_show_all (widg);
    gtk_box_pack_start (GTK_BOX (tempbox), widg, FALSE, FALSE, 2);
  }

  /* create splits widget */
  root->splits = swamigui_splits_new ();
  gtk_widget_show (root->splits);
  gtk_box_pack_start (GTK_BOX (tempbox), root->splits, TRUE, TRUE, 0);

  /* connect root selection to splits (++ ref ctrl) */
  ctrl = swami_get_control_prop_by_name (G_OBJECT (root->splits), "item-selection");
  swami_control_connect (rootsel_ctrl, ctrl, SWAMI_CONTROL_CONN_BIDIR);
  g_object_unref (ctrl);	/* -- unref tree control */

  root->panel_selector = swamigui_panel_selector_new ();
  gtk_widget_show (root->panel_selector);
  gtk_paned_pack2 (GTK_PANED (vpaned), root->panel_selector, FALSE, TRUE);

  /* connect root selection to panel selector (++ ref ctrl) */
  ctrl = swami_get_control_prop_by_name (G_OBJECT (root->panel_selector),
					 "item-selection");
  swami_control_connect (rootsel_ctrl, ctrl, 0);
  g_object_unref (ctrl);	/* -- unref tree control */

  gtk_paned_set_position (GTK_PANED (hpaned), 300);
  gtk_paned_set_position (GTK_PANED (vpaned), 400);

  /* create statusbar and pack it */
  root->statusbar = SWAMIGUI_STATUSBAR (swamigui_statusbar_new ());
  gtk_widget_show (GTK_WIDGET (root->statusbar));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (root->statusbar),
		      FALSE, FALSE, 2);

  /* connect root selection to GUI switcher widget property tree variables */
  proptree = SWAMI_ROOT (root)->proptree;

  swami_prop_tree_add_value (proptree, G_OBJECT (root), 0, "item-selection",
			     rootsel_ctrl);

  /* create SwamiguiRoot tree store property control (++ ref new object) */
  store_ctrl = swami_get_control_prop_by_name (G_OBJECT (root),
					       "tree-store-list");
  /* FIXME - kill hackish property tree (alternatives?) */
  swami_prop_tree_add_value (proptree, G_OBJECT (root), 0, "store-list",
			     store_ctrl);

  g_object_unref (store_ctrl);	/* -- unref creator's reference */
  g_object_unref (rootsel_ctrl);  /* -- unref root control */


  /* Control section of init */


  /* create MIDI hub control */
  midihub = SWAMI_CONTROL	/* ++ ref */
    (swami_root_new_object (SWAMI_ROOT (root), "SwamiControlHub"));

  /* create MIDI keyboard control */
  midikey = SWAMIGUI_CONTROL_MIDI_KEY /* ++ ref */
    (swami_root_new_object (SWAMI_ROOT (root), "SwamiguiControlMidiKey"));

  /* connect keyboard control to MIDI hub */
  swami_control_connect (SWAMI_CONTROL (midikey), midihub, 0);

  /* Connect splits widget keyboard controls to MIDI keyboard object */
  swamigui_root_connect_midi_keyboard_controls (root, G_OBJECT (midikey));

  g_object_unref (midikey);	/* -- unref creator's ref */


  g_object_get (root->splits, "piano", &piano, NULL); /* ++ ref piano */
  g_object_get (piano, "midi-control", &pctrl, NULL); /* ++ ref piano MIDI ctrl */

  /* connect piano to MIDI hub */
  swami_control_connect (pctrl, midihub, SWAMI_CONTROL_CONN_BIDIR);
  g_object_unref (pctrl); 	/* -- unref */
  g_object_unref (piano);	/* -- unref */

  if (root->wavetbl)
  {
    /* get wavetable MIDI control */
    wctrl = swami_wavetbl_get_control (SWAMI_WAVETBL (root->wavetbl), 0);

    /* set initial bank/program number */
    swami_control_midi_send (wctrl, SWAMI_MIDI_BANK_SELECT, 0, 127, -1);
    swami_control_midi_send (wctrl, SWAMI_MIDI_PROGRAM_CHANGE, 0, 127, -1);

    /* connect wavetable to MIDI hub */
    swami_control_connect (midihub, SWAMI_CONTROL (wctrl),
			   SWAMI_CONTROL_CONN_BIDIR
			   | SWAMI_CONTROL_CONN_PRIORITY_HIGH);
    g_object_unref (wctrl); /* -- unref */

    /* get control connected to root selection-single property (++ ref) */
    tctrl = SWAMI_CONTROL (swami_get_control_prop_by_name
			   (G_OBJECT (root), "selection-single"));

    /* get prop control to wavetbl temp item (++ ref ctrl) */
    pctrl = SWAMI_CONTROL (swami_get_control_prop_by_name
			   (G_OBJECT (root->wavetbl), "active-item"));
    swami_control_connect (tctrl, pctrl, 0);

    g_object_unref (tctrl); /* -- unref */
    g_object_unref (pctrl); /* -- unref */

    g_signal_connect (root, "notify::selection-single",
                      G_CALLBACK (swamigui_root_cb_solo_item), NULL);
  }

  g_object_unref (midihub);	/* -- unref creator's reference */

  gtk_widget_show (root->main_window);
}

/* Callback for SwamiguiRoot::selection-single to update Wavetbl solo-item if enabled */
static void
swamigui_root_cb_solo_item (SwamiguiRoot *root, GParamSpec *pspec, gpointer user_data)
{
  GObject *solo_item;

  if (!root->solo_item_enabled) return;

  g_object_get (root, "selection-single", &solo_item, NULL);    /* ++ ref solo item */
  swamigui_root_update_solo_item (root, solo_item);
  if (solo_item) g_object_unref (solo_item);   /* -- unref solo item */
}

static void
swamigui_root_update_solo_item (SwamiguiRoot *root, GObject *solo_item)
{
  GtkTreeIter iter;
  int category;

  /* If no wavetbl driver active - return */
  if (!root->wavetbl) return;

  /* Valid solo items are instrument or sample zones */
  if (solo_item)
  {
    ipatch_type_get (G_OBJECT_TYPE (solo_item), "category", &category, NULL);

    if (category != IPATCH_CATEGORY_SAMPLE_REF && category != IPATCH_CATEGORY_INSTRUMENT_REF)
    {
      g_object_unref (solo_item);       /* -- unref solo item */
      solo_item = NULL;
    }
  }

  /* If previous solo item, restore its icon */
  if (root->solo_item)
  {
    swamigui_tree_store_change (root->patch_store, root->solo_item, NULL,
                                root->solo_item_icon);
    g_object_unref (root->solo_item);
    root->solo_item = NULL;
    root->solo_item_icon = NULL;
  }

  if (solo_item)
  {
    if (swamigui_tree_store_item_get_node (root->patch_store, solo_item, &iter))
    {
      root->solo_item = g_object_ref (solo_item);

      /* !! icon column is a static string - not allocated */
      gtk_tree_model_get (GTK_TREE_MODEL (root->patch_store), &iter,
                          SWAMIGUI_TREE_STORE_ICON_COLUMN, &root->solo_item_icon,
                          -1);

      gtk_tree_store_set (GTK_TREE_STORE (root->patch_store), &iter,
                          SWAMIGUI_TREE_STORE_ICON_COLUMN, GTK_STOCK_MEDIA_PLAY,
                          -1);
    }
    else solo_item = NULL;      /* Shouldn't fail to find item in tree, but.. */
  }

  g_object_set (root->wavetbl, "solo-item", solo_item, NULL);
}

static void
swamigui_root_connect_midi_keyboard_controls (SwamiguiRoot *root, GObject *midikey)
{
  GtkWidget *widget;
  int i;

  struct
  {
    char *widg_name;
    char *prop_name;
  } names[] = {
    { "SpinBtnLowerOctave", "lower-octave" },
    { "ChkBtnJoinOctaves", "join-octaves" },
    { "SpinBtnUpperOctave", "upper-octave" },
    { "SpinBtnLowerVelocity", "lower-velocity" },
    { "ChkBtnSameVelocity", "same-velocity" },
    { "SpinBtnUpperVelocity", "upper-velocity" }
  };

  for (i = 0; i < G_N_ELEMENTS (names); i++)
  {
    widget = swamigui_util_glade_lookup (SWAMIGUI_SPLITS (root->splits)->gladewidg,
                                         names[i].widg_name);
    swamigui_control_prop_connect_widget (midikey, names[i].prop_name,
                                          G_OBJECT (widget));
  }
}

/* called when main window is closed */
static gint
swamigui_root_cb_main_window_delete (GtkWidget *widget, GdkEvent *event,
				     gpointer data)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (data);
  swamigui_root_quit (root);

  return (TRUE);
}

/* converts a string of comma delimited GDK key symbols into a newly allocated
   zero terminated array of key values */
static guint *
swamigui_root_parse_piano_keys (const char *str)
{
  guint *keyvals;
  char **keynames;
  int count = 0, v = 0, n = 0;

  if (!str) return (NULL);

  keynames = g_strsplit (str, ",", 0);
  while (keynames[count]) count++; /* count key symbols */
  keyvals = g_malloc ((count + 1) * sizeof (guint));

  while (keynames[n])
    {
      keyvals[v] = gdk_keyval_from_name (keynames[n]);
      if (keyvals[v] != GDK_VoidSymbol) v++; /* ignore invalid key names */
      n++;
    }
  keyvals[v] = 0;		/* zero terminated */
  g_strfreev (keynames);

  return (keyvals);
}

/* converts a zero terminated array of key values into a newly allocated
   comma delimited GDK key symbol string */
static char *
swamigui_root_encode_piano_keys (const guint *keyvals)
{
  char **keynames;
  char *s;
  int i, count = 0;

  if (!keyvals) return (NULL);

  while (keyvals[count]) count++; /* count number of key values */

  /* allocate for array of key name strings */
  keynames = g_malloc ((count + 1) * sizeof (char *));

  for (i = 0; i < count; i++)	/* loop over each key */
    {
      keynames[i] = gdk_keyval_name (keyvals[i]);  /* get key name */
      if (!keynames[i])  /* if no key name for key, fail */
	{
	  g_critical ("No GDK key name for key '%d'", keyvals[i]);
	  g_free (keynames);
	  return (NULL);
	}
    }

  keynames[count] = NULL;
  s = g_strjoinv (",", keynames);  /* create the comma delimited key string */
  g_free (keynames);

  return (s);
}

/* handler for IpatchSF2 glade property interface.
 * Needed in order to handle "Current Date" button. */
static GtkWidget *
sf2_prop_handler (GtkWidget *widg, GObject *obj)
{
  GtkWidget *btn, *entry;

  if (!widg)	/* create widget? */
  {
    widg = swamigui_util_glade_create ("PropSF2");
    btn = swamigui_util_glade_lookup (widg, "BtnCurrentDate");
    entry = swamigui_util_glade_lookup (widg, "PROP::date");
    g_signal_connect (btn, "clicked", G_CALLBACK (current_date_btn_clicked), entry);
    swamigui_control_glade_prop_connect (widg, obj);
  }
  else swamigui_control_glade_prop_connect (widg, obj);	/* change object */

  return (widg);
}

/* callback when "Current Date" button is clicked */
static void
current_date_btn_clicked (GtkButton *button, gpointer user_data)
{
  struct tm *date;
  char datestr[64];
  time_t t;

  t = time (NULL);
  date = localtime (&t);
  g_return_if_fail (date != NULL);

  /* NOTE: SoundFont standard says that conventionally the date is in the format
   * "April 3, 2008".  This isn't international friendly though, so we instead
   * use the format YYYY-MM-DD which we have deemed OK, because of the use of
   * the word "conventionally" in the spec. */
  strftime (datestr, sizeof (datestr), "%Y-%m-%d", date);

  gtk_entry_set_text (GTK_ENTRY (user_data), datestr);
}
