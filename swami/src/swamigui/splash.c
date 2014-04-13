/*
 * splash.c - Swami startup splash image functions
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
#include <gtk/gtk.h>
#include "splash.h"

#include <png.h>
#include "util.h"

static void cb_win_destroy (GtkWidget *win);
static gboolean cb_button_press (GtkWidget *widg, GdkEventButton *ev);

static GtkWidget *splash_win = NULL;
static gboolean splash_timeout_active = FALSE;
static guint splash_timeout_h;

/**
 * swamigui_splash_display:
 * @timeout: Timeout in milliseconds or 0 to wait for button click
 *
 * Display the Swami splash startup image. If @timeout is %TRUE then the
 * splash image will be destroyed after a timeout period.
 */
void
swamigui_splash_display (guint timeout)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gchar *resdir, *filename;

  if (splash_win)		/* Only one instance at a time :) */
    {
      swamigui_splash_kill ();	/* Kill current instance of splash */
      return;
    }

  /* ++ alloc resdir */
  resdir = swamigui_util_get_resource_path (SWAMIGUI_RESOURCE_PATH_IMAGES);
  /* ++ alloc filename */
  filename = g_build_filename (resdir, "splash.png", NULL);
  g_free (resdir); /* -- free resdir */
  pixbuf = gdk_pixbuf_new_from_file (filename, NULL); /* ++ ref new pixbuf */
  g_free (filename); /* -- free filename */
  if (!pixbuf) return;	/* fail silently if splash image load fails */

  /* splash popup window */
  splash_win = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (splash_win), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
  gtk_window_set_resizable (GTK_WINDOW (splash_win), FALSE);
  gtk_signal_connect (GTK_OBJECT (splash_win), "destroy",
		      GTK_SIGNAL_FUNC (cb_win_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (splash_win), "button-press-event",
		      GTK_SIGNAL_FUNC (cb_button_press), NULL);
  gtk_widget_add_events (splash_win, GDK_BUTTON_PRESS_MASK);

  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_container_add (GTK_CONTAINER (splash_win), image);
  gtk_widget_show (image);

  gtk_window_set_position (GTK_WINDOW (splash_win), GTK_WIN_POS_CENTER);
  gtk_widget_show (splash_win);

  g_object_unref (pixbuf);	/* -- unref pixbuf creator's ref */

  if (timeout)
    {
      splash_timeout_active = TRUE;
      splash_timeout_h = g_timeout_add (timeout, (GSourceFunc)swamigui_splash_kill, NULL);
    }
}

/**
 * swamigui_splash_kill:
 *
 * Kills a currently displayed splash image.
 *
 * Returns: Always returns %FALSE, since it is used as a GSourceFunc for the
 * timeout.
 */
gboolean			/* so it can be used as timeout GSourceFunc */
swamigui_splash_kill (void)
{
  if (splash_win)
    {
      if (splash_timeout_active)
	gtk_timeout_remove (splash_timeout_h);
      gtk_widget_destroy (splash_win);
    }

  splash_timeout_active = FALSE;

  return (FALSE);
}

static void
cb_win_destroy (GtkWidget *win)
{
  splash_win = NULL;
}

static gboolean
cb_button_press (GtkWidget *widg, GdkEventButton *ev)
{
  swamigui_splash_kill ();

  return (FALSE);
}
