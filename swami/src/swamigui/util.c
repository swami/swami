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
static void
ui_dep_xml_start_element (GMarkupParseContext *context, const gchar *element_name,
                          const gchar **attribute_names, const gchar **attribute_values,
                          gpointer user_data, GError **error);
static void
ui_dep_xml_end_element (GMarkupParseContext *context, const gchar *element_name,
                        gpointer user_data, GError **error);
static void
ui_dep_xml_text (GMarkupParseContext *context, const gchar *text,
                 gsize text_len, gpointer user_data, GError **error);


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

/* Structure used by XML parser when finding UI dependencies */
typedef struct
{
  GHashTable *dephash;
  char *object_id;              // Current toplevel object ID (if tracking for deps)
  gboolean in_prop;             // TRUE if in dependent property element (model or adjustment) 
  GPtrArray *deparray;          // Array of strings of the object ID dependencies
} UiDepBag;

/**
 * swamigui_util_glade_create:
 * @name: Name of the GtkBuilder widget to create
 *
 * Creates a GtkBuilder widget, by @name, from the main Swami UI XML file.
 * Prints a warning if the named widget does not exist.
 *
 * Returns: Newly created GtkBuilder widget (which the caller owns a ref to)
 *    or %NULL on error
 */
GtkWidget *
swamigui_util_glade_create (const char *name)
{
  static GHashTable *dephash = NULL;    // Object dependency hash
  char **object_ids, **dep_ids;
  GtkBuilder *builder;
  GError *err = NULL;
  GtkWidget *widg;
  gchar *resdir, *filename;
  int count;

  resdir = swamigui_util_get_resource_path (SWAMIGUI_RESOURCE_PATH_UIXML); /* ++ alloc */
  filename = g_build_filename (resdir, "swami-2.ui", NULL);  /* ++ alloc */
  g_free (resdir); /* -- free resdir */

  /* One time creation of hash of object dependencies - Wish GtkBuilder did this */
  if (!dephash)
  {
    GMarkupParser parser = { 0 };
    GMarkupParseContext *context;
    UiDepBag depbag = { 0 };
    char *uixml = NULL;
    gsize len;

    dephash = g_hash_table_new (g_str_hash, g_str_equal);

    depbag.dephash = dephash;
    depbag.deparray = g_ptr_array_new ();
    parser.start_element = ui_dep_xml_start_element;
    parser.end_element = ui_dep_xml_end_element;
    parser.text = ui_dep_xml_text;
    context = g_markup_parse_context_new (&parser, 0, &depbag, NULL);

    if (g_file_get_contents (filename, &uixml, &len, &err))
    {
      if (!g_markup_parse_context_parse (context, uixml, len, &err)
          || !g_markup_parse_context_end_parse (context, &err))
      {
        g_critical ("Failed to parse UI XML file '%s': %s", filename,
                    err->message);
        g_clear_error (&err);
      }

      g_free (uixml);   /* -- free XML content */
    }
    else
    {
      g_critical ("Failed to load UI XML file '%s': %s", filename,
                  err->message);
      g_clear_error (&err);
    }

    g_ptr_array_free (depbag.deparray, TRUE);
    g_markup_parse_context_free (context);
  }

  dep_ids = g_hash_table_lookup (dephash, name);

  if (dep_ids)
    for (count = 0; dep_ids[count]; count++);
  else count = 0;

  object_ids = g_new (char *, count + 2);       /* ++ object_ids */
  if (dep_ids) memcpy (object_ids, dep_ids, count * sizeof (char *));
  object_ids[count] = (char *)name;
  object_ids[count + 1] = NULL;

  builder = gtk_builder_new ();         /* ++ ref new builder */

  if (gtk_builder_add_objects_from_file (builder, filename, object_ids, &err) == 0)
  {
    g_critical ("Failed to load UI interface '%s': %s", name,
                err->message);
    g_clear_error (&err);

    g_free (filename);          /* -- free allocated filename */
    g_object_unref (builder);   /* -- unref builder */
    g_free (object_ids);        /* -- free object IDs */

    return (NULL);
  }

  g_free (filename);         /* -- free allocated filename */
  g_free (object_ids);        /* -- free object IDs */

  gtk_builder_connect_signals (builder, NULL);
  widg = g_object_ref (gtk_builder_get_object (builder, name));  /* ++ ref for caller */
  g_object_unref (builder);   /* -- unref builder */

  return (widg);
}

/* UI object dependencies XML start element callback */
static void
ui_dep_xml_start_element (GMarkupParseContext *context, const gchar *element_name,
                          const gchar **attribute_names, const gchar **attribute_values,
                          gpointer user_data, GError **error)
{
  UiDepBag *bag = user_data;
  const GSList *stack;
  int i;

  if (!bag->object_id)
  {
    stack = g_markup_parse_context_get_element_stack (context);

    if (stack && stack->next && !stack->next->next
        && strcmp (element_name, "object") == 0)
    {
      for (i = 0; attribute_names[i]; i++)
        if (strcmp (attribute_names[i], "id") == 0)
        {
          bag->object_id = g_strdup (attribute_values[i]);
          break;
        }
    }
  }
  else if (strcmp (element_name, "property") == 0)
  {
    for (i = 0; attribute_names[i]; i++)
      if (strcmp (attribute_names[i], "name") == 0)
      {
        if (strcmp (attribute_values[i], "model") == 0
            || strcmp (attribute_values[i], "adjustment") == 0)
          bag->in_prop = TRUE;
        break;
      }
  }
}

/* Callback for XML end of element for UI dependency search */
static void
ui_dep_xml_end_element (GMarkupParseContext *context, const gchar *element_name,
                        gpointer user_data, GError **error)
{
  UiDepBag *bag = user_data;
  const GSList *stack;
  char **depids;

  if (bag->in_prop)
  {
    bag->in_prop = FALSE;
    return;
  }
  else if (bag->object_id)
  {
    stack = g_markup_parse_context_get_element_stack (context);

    if (stack && stack->next && !stack->next->next)
    {
      if (bag->deparray->len > 0)
      {
        depids = g_new (char *, bag->deparray->len + 1);
        memcpy (depids, bag->deparray->pdata, bag->deparray->len * sizeof (gpointer));
        depids[bag->deparray->len] = NULL;    // NULL terminated

        /* Hash takes over allocation of object_id and dependency object IDs (forever) */
        g_hash_table_insert (bag->dephash, bag->object_id, depids);
        g_ptr_array_set_size (bag->deparray, 0);
      }
      else g_free (bag->object_id);

      bag->object_id = NULL;
    }
  }
}

/* Called for text values in GtkBuilder UI XML dependency search. */
static void
ui_dep_xml_text (GMarkupParseContext *context, const gchar *text,
                 gsize text_len, gpointer user_data, GError **error)
{
  UiDepBag *bag = user_data;

  if (!bag->in_prop) return;
  g_ptr_array_add (bag->deparray, g_strdup (text));
}

/**
 * swamigui_util_glade_lookup:
 * @widget: A GtkBuilder generated widget or a child there of.
 * @name: Name of widget in same GtkBuilder interface as @widget to get.
 *
 * Find a GtkBuilder generated widget, by @name, via any other widget in
 * the same widget interface.  A warning is printed if the widget is not found
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

typedef struct
{
  GtkWidget *found;
  const char *name;
  GtkWidget *skip;
} FindBag;

/* Recursive function to walk a GtkContainer looking for a GtkBuilder widget by name */
static void
swamigui_util_glade_lookup_container_foreach (GtkWidget *widget, gpointer data)
{
  FindBag *findbag = data;
  const char *name;

  if (findbag->found || widget == findbag->skip) return;

  name = gtk_buildable_get_name (GTK_BUILDABLE (widget));

  if (name && strcmp (name, findbag->name) == 0)
  {
    findbag->found = widget;
    return;
  }

  if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget),
                           swamigui_util_glade_lookup_container_foreach, findbag);
}

/**
 * swamigui_util_glade_lookup_nowarn:
 * @widget: A GtkBuilder generated widget or a child there of.
 * @name: Name of widget in same GtkBuilder interface as @widget to get.
 *
 * Like swamigui_util_glade_lookup() but does not print a warning if named
 * widget is not found.
 *
 * Returns: The widget or %NULL if not found.
 */
GtkWidget *
swamigui_util_glade_lookup_nowarn (GtkWidget *widget, const char *name)
{
  FindBag findbag = { 0 };

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  findbag.name = name;

  for (; widget != NULL; widget = gtk_widget_get_parent (widget))
  {
    gtk_container_foreach (GTK_CONTAINER (widget),
                           swamigui_util_glade_lookup_container_foreach, &findbag);
    findbag.skip = widget;
  }

  return (findbag.found);
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
