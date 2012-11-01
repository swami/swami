/*
 * util.c - GUI utility functions
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
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glade/glade.h>

#include <libinstpatch/libinstpatch.h>

#include "util.h"
#include "i18n.h"

#ifdef __APPLE__
#include <CoreFoundation.h>
#endif

#if 0

/* time interval (in milliseconds) to check if log view should be popped */
#define LOG_POPUP_CHECK_INTERVAL 200

static gboolean log_viewactive = FALSE;	/* log window active? */
static gint log_poplevel = LogBad;
static gboolean log_popview = FALSE;	/* log view popup scheduled? */

static GtkWidget *log_view_widg = NULL;	/* currently active error view widg */

#endif

/* unique dialog system data */
typedef struct {
  GtkWidget *dialog;
  gchar *strkey;
  int key2;
} UniqueDialogKey;

gboolean unique_dialog_inited = FALSE;
GArray *unique_dialog_array;

static void swamigui_util_cb_waitfor_widget_destroyed (GtkWidget *widg, gpointer data);
static GtkWidget *
swamigui_util_custom_glade_widget_handler (GladeXML *xml, gchar *func_name,
                                           gchar *name, gchar *string1,
                                           gchar *string2, gint int1,
                                           gint int2, gpointer user_data);

// static gboolean log_check_popup (gpointer data);
// static void log_view_cb_destroy (void);


/* initialize various utility services (unique dialog, log view timer, etc) */
void
swamigui_util_init (void)
{
  unique_dialog_array = g_array_new (FALSE, FALSE, sizeof (UniqueDialogKey));
  unique_dialog_inited = TRUE;

  //  g_timeout_add (LOG_POPUP_CHECK_INTERVAL, (GSourceFunc)log_check_popup, NULL);
}

guint
swamigui_util_unit_rgba_color_get_type (void)
{
  static guint unit_type = 0;

  if (!unit_type)
  {
    IpatchUnitInfo *info;

    info = ipatch_unit_info_new ();
    info->value_type = G_TYPE_UINT;
    info->name = "rgba-color";
    info->label = _("Color");
    info->descr = _("RGBA color value (in the form 0xRRGGBBAA)");

    unit_type = ipatch_unit_register (info);

    ipatch_unit_info_free (info);
  }

  return (unit_type);
}

/**
 * swamigui_util_canvas_line_set:
 * @line: #GnomeCanvasLine object or an item with a "points" property
 * @x1: First X coordinate of line
 * @y1: First Y coordinate of line
 * @x2: Second X coordinate of line
 * @y2: Second Y coordinate of line
 *
 * A convenience function to set a #GnomeCanvasLine to a single line segment.
 * Can also be used on other #GnomeCanvasItem types which have a "points"
 * property.
 */
void
swamigui_util_canvas_line_set (GnomeCanvasItem *item, double x1, double y1,
			       double x2, double y2)
{
  GnomeCanvasPoints *points;

  points = gnome_canvas_points_new (2);

  points->coords[0] = x1;
  points->coords[1] = y1;
  points->coords[2] = x2;
  points->coords[3] = y2;

  g_object_set (item, "points", points, NULL);
  gnome_canvas_points_free (points);
}

/* Unique dialog system is for allowing unique non-modal dialogs for
   resources identified by a string key and an optional additional
   integer key, attempting to open up a second dialog for the same
   resource will cause the first dialog to be brought to the front of
   view and no additional dialog will be created */

/* looks up a unique dialog widget by its keys, returns the widget or NULL */
GtkWidget *
swamigui_util_lookup_unique_dialog (gchar *strkey, gint key2)
{
  UniqueDialogKey *udkeyp;
  gint i;

  for (i = unique_dialog_array->len - 1; i >= 0; i--)
    {
      udkeyp = &g_array_index (unique_dialog_array, UniqueDialogKey, i);
      if ((udkeyp->strkey == strkey || strcmp (udkeyp->strkey, strkey) == 0)
	  && udkeyp->key2 == key2)
	return (udkeyp->dialog);
    }

  return (NULL);
}

/* register a unique dialog, if a dialog already exists with the same keys,
   then activate the existing dialog and return FALSE, otherwise register the
   new dialog and return TRUE */
gboolean
swamigui_util_register_unique_dialog (GtkWidget *dialog, gchar *strkey,
				     gint key2)
{
  UniqueDialogKey udkey;
  GtkWidget *widg;

  if ((widg = swamigui_util_lookup_unique_dialog (strkey, key2)))
    {
      gtk_widget_activate (widg);
      return (FALSE);
    }

  udkey.dialog = dialog;
  udkey.strkey = strkey;
  udkey.key2 = key2;

  g_array_append_val (unique_dialog_array, udkey);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
	      (GtkSignalFunc)swamigui_util_unregister_unique_dialog, NULL);

  return (TRUE);
}

void
swamigui_util_unregister_unique_dialog (GtkWidget *dialog)
{
  UniqueDialogKey *udkeyp;
  gint i;

  for (i = unique_dialog_array->len - 1; i >= 0; i--)
    {
      udkeyp = &g_array_index (unique_dialog_array, UniqueDialogKey, i);
      if (udkeyp->dialog == dialog)
	break;
    }

  if (i >= 0)
    g_array_remove_index (unique_dialog_array, i);
}

/* activate (or raise) a unique dialog into view */
gboolean
swamigui_util_activate_unique_dialog (gchar *strkey, gint key2)
{
  GtkWidget *dialog;

  if ((dialog = swamigui_util_lookup_unique_dialog (strkey, key2)))
    {
      gdk_window_raise (GTK_WIDGET (dialog)->window);
      return (TRUE);
    }

  return (FALSE);
}

/* run gtk_main loop until the GtkObject data property "action" is !=
   NULL, mates nicely with swamigui_util_quick_popup, returns value of
   "action".  Useful for complex routines that require a lot of user
   dialog interaction.  Rather than having to save state info and exit
   and return to a routine, a call to this routine can be made which
   will wait for the user's choice and return the index of the button
   (or other user specified value), -1 if the widget was destroyed or
   -2 if gtk_main_quit was called */
gpointer
swamigui_util_waitfor_widget_action (GtkWidget *widg)
{
  GQuark quark;
  gpointer val = NULL;
  gboolean destroyed = FALSE;
  guint sigid;

  /* initialize the action variable to NULL */
  quark = gtk_object_data_force_id ("action");
  gtk_object_set_data_by_id (GTK_OBJECT (widg), quark, NULL);

  /* already passing one variable to destroy signal handler, so bind this one
     as a GtkObject data item, will notify us if widget was destroyed */
  gtk_object_set_data (GTK_OBJECT (widg), "_destroyed", &destroyed);

  /* val is set to "action" by swamigui_util_cb_waitfor_widget_destroyed if the
     widget we are waiting for gets killed */
  sigid =
    gtk_signal_connect (GTK_OBJECT (widg), "destroy",
		GTK_SIGNAL_FUNC (swamigui_util_cb_waitfor_widget_destroyed),
		&val);
  do
    {
      if (gtk_main_iteration ()) /* run the gtk main loop, wait if no events */
	val = GINT_TO_POINTER (-2); /* gtk_main_quit was called, return -2 */
      else if (val == NULL)	/* check the "action" data property */
	val = gtk_object_get_data_by_id (GTK_OBJECT (widg), quark);
    }
  while (val == NULL);		/* loop until "action" is set */

  if (!destroyed)
    gtk_signal_disconnect (GTK_OBJECT (widg), sigid);

  return (val);
}

static void
swamigui_util_cb_waitfor_widget_destroyed (GtkWidget *widg, gpointer data)
{
  gpointer *val = data;
  gpointer action;
  gboolean *destroyed;

  action = gtk_object_get_data (GTK_OBJECT (widg), "action");
  destroyed = gtk_object_get_data (GTK_OBJECT (widg), "_destroyed");

  *destroyed = TRUE;

  if (action)
    *val = action;
  else *val = GINT_TO_POINTER (-1);
}

/* a callback for widgets (buttons, etc) within a "parent" widget used by
  swamigui_util_waitfor_widget_action, sets "action" to the specified "value" */
void
swamigui_util_widget_action (GtkWidget *cbwidg, gpointer value)
{
  GtkWidget *parent;

  parent = gtk_object_get_data (GTK_OBJECT (cbwidg), "parent");
  gtk_object_set_data (GTK_OBJECT (parent), "action", value);
}

/* Glade handler to create custom widgets */
static GtkWidget *
swamigui_util_custom_glade_widget_handler (GladeXML *xml, gchar *func_name,
                                           gchar *name, gchar *string1,
                                           gchar *string2, gint int1,
                                           gint int2, gpointer user_data)
{
  GModule *module;
  gpointer funcptr;
  GtkWidget * (*newwidg)(void);
  GtkWidget *widg = NULL;

  module = g_module_open (NULL, 0);

  if (g_module_symbol (module, func_name, &funcptr))
  {
    newwidg = funcptr;
    widg = newwidg ();
    gtk_widget_show (widg);
  }
  else g_warning ("Failed to create glade custom widget '%s' with function '%s'",
                  name, func_name);

  g_module_close (module);

  return (widg);
}

/**
 * swamigui_util_glade_create:
 * @name: Name of the glade widget to create
 *
 * Creates a glade widget, by @name, from the main Swami glade XML file.
 * Prints a warning if the named widget does not exist.
 *
 * Returns: Newly created glade widget or %NULL on error
 */
GtkWidget *
swamigui_util_glade_create (const char *name)
{
  static gboolean first_time = TRUE;
  gchar *resdir, *filename;
  GladeXML *xml;

  resdir = swamigui_util_get_resource_path (SWAMIGUI_RESOURCE_PATH_UIXML); /* ++ alloc */
  filename = g_build_filename (resdir, "swami-2.glade", NULL);  /* ++ alloc */
  g_free (resdir); /* -- free resdir */

  if (first_time)
  {
    glade_set_custom_handler (swamigui_util_custom_glade_widget_handler, NULL);
    first_time = FALSE;
  }

  xml = glade_xml_new (filename, name, NULL);

  g_free (filename); /* -- free allocated filename */

  glade_xml_signal_autoconnect (xml);

  return (glade_xml_get_widget (xml, name));
}

/**
 * swamigui_util_glade_lookup:
 * @widget: A libglade generated widget or a child there of.
 * @name: Name of widget in same XML tree as @widget to get.
 *
 * Find a libglade generated widget, by @name, via any other widget in
 * the same XML widget tree.  A warning is printed if the widget is not found
 * to help with debugging, when a widget is expected.  Use
 * swamigui_util_glade_lookup_nowarn() to check if the named
 * widget does not exist, and not display a warning.
 *
 * Returns: The widget or %NULL if not found.
 */
GtkWidget *
swamigui_util_glade_lookup (GtkWidget *widget, const char *name)
{
  GtkWidget *w;

  w = swamigui_util_glade_lookup_nowarn (widget, name);
  if (!w) g_warning ("libglade widget not found: %s", name);

  return (w);
}

/**
 * swamigui_util_glade_lookup_nowarn:
 * @widget: A libglade generated widget or a child there of.
 * @name: Name of widget in same tree as @widget to get.
 *
 * Like swamigui_util_glade_lookup() but does not print a warning if named
 * widget is not found.
 *
 * Returns: The widget or %NULL if not found.
 */
GtkWidget *
swamigui_util_glade_lookup_nowarn (GtkWidget *widget, const char *name)
{
  GladeXML *glade_xml;
  GtkWidget *w;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  glade_xml = glade_get_widget_tree (widget);
  g_return_val_if_fail (glade_xml != NULL, NULL);

  w =  glade_xml_get_widget (glade_xml, name);
  if (!w) g_warning ("libglade widget not found: %s", name);

  return (w);
}

int
swamigui_util_option_menu_index (GtkWidget *opmenu)
{
  GtkWidget *menu, *actv;

  g_return_val_if_fail (GTK_IS_OPTION_MENU (opmenu), 0);

  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (opmenu));
  actv = gtk_menu_get_active (GTK_MENU (menu));

  return (g_list_index (GTK_MENU_SHELL (menu)->children, actv));
}

#if 0


/* a callback for a glib timeout that periodically checks if the log view
   should be popped by the GTK thread (see swamigui_util_init) */
static gboolean
log_check_popup (gpointer data)
{
  if (log_popview)
    {
      log_popview = FALSE;
      log_view (NULL);
    }
  return (TRUE);
}

/* log_view - Display log view window */
void
log_view (gchar * title)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *msgarea;
  GtkAdjustment *adj;
  GtkWidget *vscrollbar;
  GtkWidget *btn;
  static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

  /* need to lock test and set of log_viewactive */
  g_static_mutex_lock (&mutex);
  if (log_viewactive)
    {
      g_static_mutex_unlock (&mutex);
      return;
    }
  log_viewactive = TRUE;
  g_static_mutex_unlock (&mutex);

  if (title) dialog = gtk_dialog_new (title);
  else dialog = gtk_dialog_new (_("Swami log"));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  msgarea = gtk_text_new (NULL, NULL);
  gtk_widget_set_default_size (msgarea, 400, 100);
  gtk_text_set_editable (GTK_TEXT (msgarea), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (msgarea), FALSE);

  /* have to lock on log read, another thread might be writing to it */
  g_mutex_lock (log_mutex);
  gtk_text_insert (GTK_TEXT (msgarea), NULL, NULL, NULL, log_buf->str, -1);
  g_mutex_unlock (log_mutex);

  gtk_box_pack_start (GTK_BOX (hbox), msgarea, TRUE, TRUE, 0);
  gtk_widget_show (msgarea);

  adj = GTK_TEXT (msgarea)->vadj;	/* get the message area's vert adj */

  vscrollbar = gtk_vscrollbar_new (adj);
  gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, FALSE, 0);
  gtk_widget_show (vscrollbar);

  btn = gtk_button_new_with_label (_("OK"));
  gtk_signal_connect_object (GTK_OBJECT (btn), "clicked",
    (GtkSignalFunc) gtk_widget_destroy, GTK_OBJECT (dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), btn,
    FALSE, FALSE, 0);
  gtk_widget_show (btn);

  gtk_signal_connect_object (GTK_OBJECT (dialog), "destroy",
    GTK_SIGNAL_FUNC (log_view_cb_destroy), NULL);

  gtk_widget_show (dialog);

  log_view_widg = NULL;
}

/* reset dialog active variables */
static void
log_view_cb_destroy (void)
{
  log_viewactive = FALSE;
  log_view_widg = NULL;
}

#endif


/**
 * swamigui_util_str_crlf2lf:
 * @str: String to convert
 *
 * Convert all dos newlines ("\r\n") to unix newlines "\n" in a string
 *
 * Returns: New string with converted newlines, should be freed when done with
 */
char *
swamigui_util_str_crlf2lf (char *str)
{
  char *newstr, *s;

  newstr = g_new (char, strlen (str) + 1);
  s = newstr;

  while (*str != '\0')
    {
      if (*str != '\r' || *(str + 1) != '\n')
	*(s++) = *str;
      str++;
    }
  *s = '\0';

  return (newstr);
}

/**
 * swamigui_util_str_lf2crlf:
 * @str: String to convert
 *
 * Convert all unix newlines "\n" to dos newlines ("\r\n") in a string
 *
 * Returns: New string with converted newlines, should be freed when done with
 */
char *
swamigui_util_str_lf2crlf (char *str)
{
  GString *gs;
  char *s;

  gs = g_string_sized_new (sizeof (str));

  while (*str != '\0')
    {
      if (*str != '\n')
	gs = g_string_append_c (gs, *str);
      else
	gs = g_string_append (gs, "\r\n");
      str++;
    }
  s = gs->str;
  g_string_free (gs, FALSE);	/* character segment is not free'd */

  return (s);
}

/**
 * swamigui_util_substrcmp:
 * @sub: Partial string to search for in str
 * @str: String to search for sub string in
 *
 * Search for a sub string in a string
 *
 * Returns: %TRUE if "sub" is found in "str", %FALSE otherwise
 */
int
swamigui_util_substrcmp (char *sub, char *str)
{
  char *s, *s2;

  if (!*sub)
    return (TRUE);		/* null string, matches */

  while (*str)
    {
      if (tolower (*str) == tolower (*sub))
	{
	  s = sub + 1;
	  s2 = str + 1;
	  while (*s && *s2)
	    {
	      if (tolower (*s) != tolower (*s2))
		break;
	      s++;
	      s2++;
	    }
	  if (!*s)
	    return (TRUE);
	}
      str++;
    }
  return (FALSE);
}

gchar *
swamigui_util_get_resource_path (SwamiResourcePath kind)
{
  static gchar *res_root;

  /* Init res_root if not done yet */
  if (!res_root) {
#if defined (G_OS_WIN32)
    res_root = g_win32_get_package_installation_directory_of_module (NULL);
#elif defined (__APPLE__)
    CFURLRef ResDirURL;
    gchar buf[PATH_MAX];

    /* Use bundled resources if we are in an app bundle */
    ResDirURL = CFBundleCopyResourcesDirectoryURL (CFBundleGetMainBundle());
    if (CFURLGetFileSystemRepresentation (ResDirURL, TRUE, (UInt8 *)buf, PATH_MAX)
        && g_str_has_suffix (buf, ".app/Contents/Resources"))
      res_root = g_strdup (buf);
    CFRelease (ResDirURL);
#endif
#ifdef SOURCE_BUILD
    g_free (res_root); /* drop previous value if any and replace with src dir */
    res_root = g_build_filename (SOURCE_DIR, "/src/swamigui", NULL);
#endif
  }
  /* default/fallback if all else fails: use default paths */
  if (!res_root)
    res_root = g_strdup ("");

  switch (kind)
  {
    case SWAMIGUI_RESOURCE_PATH_ROOT:
      return (*res_root ? g_strdup (res_root) : NULL);
    case SWAMIGUI_RESOURCE_PATH_UIXML:
      return (*res_root ? g_strdup (res_root) : g_strdup (UIXML_DIR));
    case SWAMIGUI_RESOURCE_PATH_IMAGES:
      return (*res_root ?
              g_build_filename (res_root, "images", NULL) :
              g_strdup (IMAGES_DIR));
    default:
      return NULL;
  }
}
