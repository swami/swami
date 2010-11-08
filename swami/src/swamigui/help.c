/*
 * help.c - User interface help routines
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
#include <string.h>
#include <gtk/gtk.h>

#include "SwamiguiRoot.h"

#include "help.h"
#include "splash.h"
#include "icons.h"
#include "i18n.h"
#include "util.h"

static void swamigui_help_swamitips_set_tip (GtkWidget *tips, gint tipnum);
static void swamigui_help_cb_swamitips_next (GtkWidget *btn, GtkWidget *tips);
static void swamigui_help_cb_swamitips_previous (GtkWidget *btn,
						GtkWidget *tips);
static void swamigui_help_cb_again_toggled (GtkWidget *btn, gpointer data);

static gchar *swamitips_msg[] = {
  N_("Welcome to Swami!\n\n"
    "Many operations are performed by Right clicking on the instrument tree."
    " The type of item clicked on determines the options that are available."),
  N_("To select multiple items in the instrument tree:\n"
    "Hold down CTRL to mark individual items or SHIFT to mark a range."),
  N_("To zoom in the sample viewer:\n"
    "Middle click and drag the mouse to the left or right in the sample"
    " viewer. A vertical line will appear to mark the position to zoom into,"
    " and the distance from the marker determines how fast the zoom is"
    " performed. Moving the mouse to the opposite side of"
    " the zoom marker will unzoom. The mouse Wheel can also be used to zoom.\n"
    "SHIFT Middle click and drag will scroll the sample left or right."),
  N_("The right most view in the Sample Editor assists with making seamless"
     " loops. The sample points surrounding the start of the loop are shown"
     " in green while the end sample points are red. They are overlaid on one"
     " another, where they intersect they become yellow. Zooming can be performed"
     " in the loop viewer, just like the normal sample view. The more yellow points"
     " surrounding the middle line, the more seamless the loop!"),
  N_("In the note range view click and drag on the same line as the range and"
     " the nearest endpoint will be adjusted. Multiple ranges can be selected"
     " using CTRL and SHIFT. Clicking and dragging with the Middle mouse button"
     " will move a range. The \"Move\" drop down selector can be used to set"
     " if the ranges, root notes or both are moved together. The root notes"
     " are shown as blue circles on the same line as the range they belong to."),
  N_("To add samples to instruments:\n"
    "Select the samples and/or instrument zones you want to add and then Right"
    " click on the instrument you would like to add to and select"
    " \"Paste\". If an instrument zone was selected all its parameters"
    " will be copied into the newly created zone. The same procedure is used"
    " to add instruments to presets."),
  N_("The sample loop finder is actived by clicking the Finder icon in the"
     " Sample Editor. Two additional range selectors will appear above the sample"
     " and allow for setting the loop start and end search \"windows\". The"
     " Config tab contains additional settings which control the algorithm."
     " The \"Window size\" sets the number of sample points which are compared"
     " around the loop end points, \"Min loop size\" sets a minimum loop size"
     " for the results and sample groups provide settings for grouping results"
     " by their proximity and size. Once the parameters are to your liking,"
     " click the \"Find Loops\" button. The parameter settings can drastically"
     " affect the time it takes. Once complete a list of results will be"
     " displayed, clicking a result will assign the given loop. Click the"
     " \"Revert\" button to return to the loop setting prior to executing the"
     " find loops operation."),
  N_("The FFTune plugin provides semi-automated tuning of samples. To access it"
     " click the FFTune panel tab when a sample or instrument zone is selected."
     " An FFT calculation is performed and the spectrum is displayed in the view."
     " A list of tuning results is shown in the list. Clicking a tuning result"
     " will automatically assign it to the selected sample or zone. Results are"
     " based on interpreting the strongest frequency components as MIDI root"
     " note values and calculating the fine tine adjustment required to play"
     " back the matched frequency component at the given root note. Your milage"
     " may vary, depending on the sample content. The \"Sample data\" dropdown"
     " allows for setting what portion of the sample the calculation is"
     " performed on: All for the entire sample and Loop for just the loop."),
  N_("Adjusting knobs is done by clicking and dragging the mouse up or down."
    " Hold the SHIFT key to make finer adjustments."),
  N_("No more tips!")
};

#define TIPCOUNT	(sizeof(swamitips_msg) / sizeof(swamitips_msg[0]))

static gint swamitip_current;	/* current swami tip # */

void
swamigui_help_about (void)
{
  GtkWidget *boutwin;
  GdkPixbuf *pixbuf;

  if (swamigui_util_activate_unique_dialog ("about", 0)) return;

  boutwin = swamigui_util_glade_create ("About");
  swamigui_util_register_unique_dialog (boutwin, "about", 0);

  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (boutwin), VERSION);

  /* ++ ref Swami logo icon */
  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     "swami_logo", 160, 0, NULL);
  gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (boutwin), pixbuf);
  if (pixbuf) g_object_unref (pixbuf);      /* -- unref pixbuf */

  g_signal_connect_swapped (G_OBJECT(boutwin), "response",
                            G_CALLBACK (gtk_widget_destroy), boutwin);
  gtk_widget_show (boutwin);
}

/* Create swami tips dialog and load it with current tip */
void
swamigui_help_swamitips_create (SwamiguiRoot *root)
{
  GtkWidget *tips;
  GtkWidget *widg;
  int i;

  g_return_if_fail (SWAMIGUI_IS_ROOT (root));

  if (swamigui_util_activate_unique_dialog ("tips", 0)) return;

  tips = swamigui_util_glade_create ("Tips");
  swamigui_util_register_unique_dialog (tips, "tips", 0);

  g_object_set_data (G_OBJECT (tips), "root", root);

  widg = swamigui_util_glade_lookup (tips, "CHKagain");

  /* update check button to state of Tips Enabled on startup config var */
  g_object_get (root, "tips-enable", &i, NULL);
  if (i) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widg), TRUE);

  g_signal_connect (widg, "toggled",
		    G_CALLBACK (swamigui_help_cb_again_toggled), root);

  widg = swamigui_util_glade_lookup (tips, "BTNnext");
  g_signal_connect (G_OBJECT (widg), "clicked",
		    G_CALLBACK (swamigui_help_cb_swamitips_next), tips);

  widg = swamigui_util_glade_lookup (tips, "BTNprev");
  g_signal_connect (G_OBJECT (widg), "clicked",
		    G_CALLBACK (swamigui_help_cb_swamitips_previous), tips);

  widg = swamigui_util_glade_lookup (tips, "BTNclose");
  g_signal_connect_swapped (G_OBJECT (widg), "clicked",
			    G_CALLBACK (gtk_widget_destroy), tips);

  g_object_get (root, "tips-position", &i, NULL);
  swamigui_help_swamitips_set_tip (tips, i);

  gtk_widget_show (tips);
}

static void
swamigui_help_swamitips_set_tip (GtkWidget *tips, gint tipnum)
{
  SwamiguiRoot *root;
  GtkWidget *txtview;
  GtkTextBuffer *buffer;
  GtkWidget *btn;
  gchar *msg;
  gint pos;

  tipnum = CLAMP (tipnum, 0, TIPCOUNT - 1);

  btn = swamigui_util_glade_lookup (tips, "BTNprev");
  gtk_widget_set_sensitive (btn, (tipnum != 0));

  btn = swamigui_util_glade_lookup (tips, "BTNnext");
  gtk_widget_set_sensitive (btn, (tipnum != TIPCOUNT - 1));

  txtview = swamigui_util_glade_lookup (tips, "TXTview");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (txtview));

  msg = _(swamitips_msg[tipnum]); /* get the tip */
  pos = 0;			/* add the new tip at position 0 */
  gtk_text_buffer_set_text (buffer, msg, -1);

  swamitip_current = tipnum;

  root = g_object_get_data (G_OBJECT (tips), "root");
  if (root)
    {
      tipnum++;
      if (tipnum >= TIPCOUNT) tipnum = TIPCOUNT;
      g_object_set (root, "tips-position", tipnum, NULL);
    }
}

/* next tip callback */
static void
swamigui_help_cb_swamitips_next (GtkWidget *btn, GtkWidget *tips)
{
  swamigui_help_swamitips_set_tip (tips, swamitip_current + 1);
}

/* previous tip callback */
static void
swamigui_help_cb_swamitips_previous (GtkWidget *btn, GtkWidget *tips)
{
  swamigui_help_swamitips_set_tip (tips, swamitip_current - 1);
}

static void
swamigui_help_cb_again_toggled (GtkWidget *btn, gpointer data)
{
  SwamiguiRoot *root = SWAMIGUI_ROOT (data);

  g_object_set (root, "tips-enable",
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn)), NULL);
}
