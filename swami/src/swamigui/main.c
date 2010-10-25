/*
 * main.c - Main routine to kick things off
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

#ifdef PYTHON_SUPPORT
#include <Python.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>		/* for getenv() */
#include <gtk/gtk.h>

#include <libswami/libswami.h>
#include "SwamiguiRoot.h"
#include "swami_python.h"
#include "i18n.h"

static void log_python_output_func (const char *output, gboolean is_stderr);

/* global boolean feature hacks */
extern gboolean swamigui_disable_python;
extern gboolean swamigui_disable_plugins;


int
main (int argc, char *argv[])
{
  SwamiguiRoot *root = NULL;
  gboolean show_version = FALSE;
  gboolean default_prefs = FALSE;
  gchar **scripts = NULL;
  gchar **files = NULL;
  char *loadfile, *fname;
  GOptionContext *context;
  GError *err = NULL;
  gchar **sptr;

  GOptionEntry entries[] = {
    { "version", 'V', 0, G_OPTION_ARG_NONE, &show_version, "Display Swami version number", NULL },
    { "run-script", 'r', 0, G_OPTION_ARG_FILENAME_ARRAY, &scripts, "Run one or more Python scripts on startup", NULL },
    { "no-plugins", 'p', 0, G_OPTION_ARG_NONE, &swamigui_disable_plugins, "Don't load plugins", NULL},
    { "default-prefs", 'd', 0, G_OPTION_ARG_NONE, &default_prefs, "Use default preferences", NULL },
    { "disable-python", 'y', 0, G_OPTION_ARG_NONE, &swamigui_disable_python, "Disable runtime Python support", NULL },
    { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, "[file1.sf2 file2.sf2 ...]" },
    { NULL }
  };

#if defined(ENABLE_NLS)
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (!g_option_context_parse (context, &argc, &argv, &err))
  {
    g_print ("option parsing failed: %s\n", err->message);
    return (1);
  }

  if (show_version)
  {
    g_print ("Swami %s\n", VERSION);
    return (0);
  }

  swamigui_init (&argc, &argv);

  root = swamigui_root_new ();	/* ++ ref root */

  /* Load preferences unless disabled */
  if (!default_prefs)
    swamigui_root_load_prefs (root);

  /* Activate the Swami object */
  swamigui_root_activate (root);

  /* just for me for that added convenience, yah  -:] */
  if ((loadfile = getenv ("SWAMI_LOAD_FILE")))
    swami_root_patch_load (SWAMI_ROOT (root), loadfile, NULL, NULL);

  /* loop over command line non-options, assuming they're files to open */
  if (files)
  {
    GtkRecentManager *manager;
    char *file_uri;

    for (sptr = files; *sptr; sptr++)
    {
      /* first try and parse argument as a URI (in case we are called from
	 recent files subsystem), then just use it as a plain file name */
      if (!(fname = g_filename_from_uri (*sptr, NULL, NULL)))
	fname = g_strdup (*sptr);

      if (swami_root_patch_load (SWAMI_ROOT (root), fname, NULL, &err))
      { /* Add file to recent chooser list */
        if ((file_uri = g_filename_to_uri (fname, NULL, NULL)))
        {
          manager = gtk_recent_manager_get_default ();

          if (!gtk_recent_manager_add_item (manager, file_uri))
            g_warning ("Error while adding file name to recent manager.");

          g_free (file_uri);
        }
      }
      else
      {
	g_critical (_("Failed to open file '%s' given as program argument: %s"),
		    fname, ipatch_gerror_message (err));
	g_clear_error (&err);
      }

      g_free (fname);
    }

    g_strfreev (files);
  }

#ifdef PYTHON_SUPPORT
  if (scripts && !swamigui_disable_python)	/* any scripts to run? */
  {
    /* set to stdout Python output function (FIXME - Use logging?) */
    swamigui_python_set_output_func (log_python_output_func);

    for (sptr = scripts; *sptr; sptr++)
    {
      char *script;
      GError *err = NULL;

      if (g_file_get_contents (*sptr, &script, NULL, &err))
      {
	PyRun_SimpleString (script);
	g_free (script);
      }
      else
      {
	g_critical ("Failed to read Python script '%s': %s", *sptr,
		    ipatch_gerror_message (err));
	g_clear_error (&err);
      }
    }
  }
#else
  if (scripts) g_critical ("No Python support, '-r' commands ignored");
#endif

  if (scripts) g_strfreev (scripts);

  gdk_threads_enter ();
  gtk_main ();			/* kick it in the main GTK loop */
  gdk_threads_leave ();

  /* we destroy it all so refdbg can tell us what objects leaked */
  g_object_unref (root);	/* -- unref root */

  exit (0);
}

/* output function which uses glib logging facility */
static void
log_python_output_func (const char *output, gboolean is_stderr)
{
  GString *gs;
  char *found;
  int pos = 0;

  /* must escape % chars */
  gs = g_string_new (output);

  while ((found = strchr (&gs->str[pos], '%')))
    {
      pos = found - gs->str;
      g_string_insert_c (gs, pos + 1, '%');
      pos += 2;
    }

  if (is_stderr)
    fputs (gs->str, stderr);
  else puts (gs->str);

  g_string_free (gs, TRUE);
}
