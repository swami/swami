/*
 * SwamiguiPythonView.c - Python source view script editor and shell.
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

/* FIXME - Hardcoded for now until... */
#define SCRIPT_PATH  "/home/josh/.swami-1/scripts"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libswami/libswami.h>
#include <Python.h>
#include <string.h>

#ifdef GTK_SOURCE_VIEW_SUPPORT
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>
#endif

#include "SwamiguiPythonView.h"
#include "swami_python.h"
#include "util.h"
#include "i18n.h"

static void swamigui_python_view_refresh_scripts (void);
static void swamigui_python_view_class_init (SwamiguiPythonViewClass *klass);
static void swamigui_python_view_set_property (GObject *object,
					       guint property_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void swamigui_python_view_get_property (GObject *object,
					       guint property_id,
					       GValue *value,
					       GParamSpec *pspec);
static void swamigui_python_view_init (SwamiguiPythonView *pyview);
static void swamigui_python_view_scripts_combo_changed (GtkComboBox *combo,
							gpointer user_data);
static gboolean swamigui_python_view_cb_key_press (GtkWidget *widget,
						   GdkEventKey *event,
						   gpointer user_data);
static void swamigui_python_view_python_output_func (const char *output,
						    gboolean is_stderr);
static void swamigui_python_view_cb_execute_clicked (GtkButton *button,
						     gpointer user_data);

/* view to send Python output to */
static SwamiguiPythonView *output_view = NULL;


static GtkListStore *scriptstore = NULL;	/* list store for scripts */

/* refresh the list of python scripts */
static void
swamigui_python_view_refresh_scripts (void)
{
  const char *fname;
  GtkTreeIter iter;
  GDir *dir;

  if (scriptstore)
    gtk_list_store_clear (scriptstore);
  else scriptstore = gtk_list_store_new (1, G_TYPE_STRING);

  dir = g_dir_open (SCRIPT_PATH, 0, NULL);

  while ((fname = g_dir_read_name (dir)))
    {
      gtk_list_store_append (scriptstore, &iter);
      gtk_list_store_set (scriptstore, &iter, 0, fname, -1);
    }

  g_dir_close (dir);
}

GType
swamigui_python_view_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiPythonViewClass), NULL, NULL,
	  (GClassInitFunc) swamigui_python_view_class_init, NULL, NULL,
	  sizeof (SwamiguiPythonView), 0,
	  (GInstanceInitFunc) swamigui_python_view_init,
	};

      obj_type = g_type_register_static (GTK_TYPE_VBOX, "SwamiguiPythonView",
					 &obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_python_view_class_init (SwamiguiPythonViewClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = swamigui_python_view_set_property;
  obj_class->get_property = swamigui_python_view_get_property;

  swamigui_python_view_refresh_scripts ();
}

static void
swamigui_python_view_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
  //  SwamiguiPythonView *pyview = SWAMIGUI_PYTHON_VIEW (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_python_view_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
  //  SwamiguiPythonView *pyview = SWAMIGUI_PYTHON_VIEW (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_python_view_init (SwamiguiPythonView *pyview)
{
  GtkWidget *frame;
  GtkWidget *btn;
  GtkCellRenderer *renderer;
#ifdef GTK_SOURCE_VIEW_SUPPORT
  GtkSourceLanguagesManager *lm;
  GtkSourceLanguage *lang;
#endif

  pyview->glade_widg = swamigui_util_glade_create ("PythonEditor");
  gtk_container_add (GTK_CONTAINER (pyview), pyview->glade_widg);

  pyview->comboscripts = swamigui_util_glade_lookup (pyview->glade_widg,
						     "ComboScripts");
  gtk_combo_box_set_model (GTK_COMBO_BOX (pyview->comboscripts),
			   GTK_TREE_MODEL (scriptstore));
  gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (pyview->comboscripts), 0);

  g_signal_connect (pyview->comboscripts, "changed",
		    G_CALLBACK (swamigui_python_view_scripts_combo_changed),
		    pyview);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (pyview->comboscripts), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (pyview->comboscripts), renderer,
				  "text", 0, NULL);

  frame = swamigui_util_glade_lookup (pyview->glade_widg, "ScrollScriptEditor");

#ifdef GTK_SOURCE_VIEW_SUPPORT
  lm = gtk_source_languages_manager_new ();
  lang = gtk_source_languages_manager_get_language_from_mime_type
    (lm, "text/x-python");

  if (lang) pyview->srcbuf
	      = GTK_TEXT_BUFFER (gtk_source_buffer_new_with_language (lang));
  else pyview->srcbuf = GTK_TEXT_BUFFER (gtk_source_buffer_new (NULL));

  gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (pyview->srcbuf), TRUE);

  pyview->srcview = gtk_source_view_new_with_buffer
    (GTK_SOURCE_BUFFER (pyview->srcbuf));
#else
  pyview->srcbuf = gtk_text_buffer_new (NULL);
  pyview->srcview = gtk_text_view_new_with_buffer (pyview->srcbuf);
#endif

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (pyview->srcview), GTK_WRAP_CHAR);
  g_signal_connect (pyview->srcview, "key-press-event",
		    G_CALLBACK (swamigui_python_view_cb_key_press), pyview);
  gtk_widget_show (pyview->srcview);
  gtk_container_add (GTK_CONTAINER (frame), pyview->srcview);

  pyview->conbuf = gtk_text_buffer_new (NULL);

  pyview->conview = swamigui_util_glade_lookup (pyview->glade_widg,
						"TxtPyConsole");
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (pyview->conview), pyview->conbuf);

  btn = swamigui_util_glade_lookup (pyview->glade_widg, "BtnExecute");
  g_signal_connect (btn, "clicked",
		    G_CALLBACK (swamigui_python_view_cb_execute_clicked),
		    pyview);
}

static void
swamigui_python_view_scripts_combo_changed (GtkComboBox *combo, gpointer user_data)
{
  SwamiguiPythonView *pyview = SWAMIGUI_PYTHON_VIEW (user_data);
  char *fname, *fullname, *buf;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_combo_box_get_active_iter (combo, &iter))
    return;

  model = gtk_combo_box_get_model (combo);
  gtk_tree_model_get (model, &iter, 0, &fname, -1);

  fullname = g_strconcat (SCRIPT_PATH, G_DIR_SEPARATOR_S, fname, NULL);
  g_free (fname);

  if (!g_file_get_contents (fullname, &buf, NULL, NULL))
    {
      g_free (fullname);
      return;
    }

  g_free (fullname);

  gtk_text_buffer_set_text (pyview->srcbuf, buf, -1);
  g_free (buf);
}

static gboolean
swamigui_python_view_cb_key_press (GtkWidget *widget, GdkEventKey *event,
				   gpointer user_data)
{
  SwamiguiPythonView *pyview = SWAMIGUI_PYTHON_VIEW (user_data);
  GtkWidget *btn;
  GtkTextMark *cursor;
  GtkTextIter start_iter, end_iter;
  char *cmd;

  if (event->keyval != GDK_Return) return (FALSE);

  /* return if not in "shell mode" (ENTER executes line) */
  btn = swamigui_util_glade_lookup (pyview->glade_widg, "BtnShellMode");
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn)))
    return (FALSE);

  /* get cursor position */
  cursor = gtk_text_buffer_get_insert (pyview->srcbuf);

  /* get an iterator for the cursor (end of line) */
  gtk_text_buffer_get_iter_at_mark (pyview->srcbuf, &end_iter, cursor);

  start_iter = end_iter;	/* copy iterator */

  /* set the iterator to the beginning of the line */
  gtk_text_iter_set_line_offset (&start_iter, 0);

  cmd = gtk_text_buffer_get_text (pyview->srcbuf, &start_iter, &end_iter, FALSE);

  if (cmd && cmd[0])		/* some command to execute? */
    { /* set current output view and Python log function */
      output_view = pyview;
      swamigui_python_set_output_func (swamigui_python_view_python_output_func);

      PyRun_SimpleString (cmd);

      output_view = NULL;
    }

  g_free (cmd);

  return (FALSE);
}

static void
swamigui_python_view_python_output_func (const char *output,
					 gboolean is_stderr)
{
  GtkTextIter iter;

  if (!output_view) return;

  gtk_text_buffer_get_end_iter (output_view->conbuf, &iter);
  gtk_text_buffer_insert (output_view->conbuf, &iter, output, -1);
}

/* Glade callback for Execute toolbar button */
static void
swamigui_python_view_cb_execute_clicked (GtkButton *button, gpointer user_data)
{
  SwamiguiPythonView *pyview = SWAMIGUI_PYTHON_VIEW (user_data);
  GtkTextIter start, end;
  char *script;

  gtk_text_buffer_get_bounds (pyview->srcbuf, &start, &end);
  script = gtk_text_buffer_get_text (pyview->srcbuf, &start, &end, FALSE);

  output_view = pyview;
  swamigui_python_set_output_func (swamigui_python_view_python_output_func);

  PyRun_SimpleString (script);

  output_view = NULL;

  g_free (script);
}

/**
 * swamigui_python_view_new:
 *
 * Create a new GUI python view shell widget.
 *
 * Returns: New swami GUI python view.
 */
GtkWidget *
swamigui_python_view_new (void)
{
  return (GTK_WIDGET (g_object_new (SWAMIGUI_TYPE_PYTHON_VIEW, NULL)));
}
