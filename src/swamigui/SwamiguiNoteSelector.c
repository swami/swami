/*
 * SwamiguiNoteSelector.c - MIDI note selector widget.
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libswami/libswami.h>

#include "SwamiguiNoteSelector.h"
#include "i18n.h"

static gint swamigui_note_selector_input (GtkSpinButton *spinbutton,
					  gdouble *newval);
static gboolean swamigui_note_selector_output (GtkSpinButton *spinbutton);


/* define the note selector type */
G_DEFINE_TYPE (SwamiguiNoteSelector, swamigui_note_selector,
	       GTK_TYPE_SPIN_BUTTON);

static void
swamigui_note_selector_class_init (SwamiguiNoteSelectorClass *klass)
{
  GtkSpinButtonClass *spinbtn_class = GTK_SPIN_BUTTON_CLASS (klass);

  spinbtn_class->input = swamigui_note_selector_input;
  spinbtn_class->output = swamigui_note_selector_output;
}

static gint
swamigui_note_selector_input (GtkSpinButton *spinbutton, gdouble *newval)
{
  const char *text;
  int note;

  text = gtk_entry_get_text (GTK_ENTRY (spinbutton));

  if (!text || strchr (text, '|'))
    return (FALSE);

  note = swami_util_midi_str_to_note (text);
  if (note == -1) return GTK_INPUT_ERROR;
  *newval = note;

  return (TRUE);
}

/* override spin button display "output" signal to show note strings */
static gboolean
swamigui_note_selector_output (GtkSpinButton *spinbutton)
{
  char notestr[9] = { 0 };
  GtkAdjustment *adj;
  int note;

  adj = gtk_spin_button_get_adjustment (spinbutton);
  note = (int)adj->value;

  if (note >= 0 && note <= 127)
  {
    sprintf (notestr, "%d | ", note);
    swami_util_midi_note_to_str (note, notestr + strlen (notestr));
  }

  if (strcmp (notestr, gtk_entry_get_text (GTK_ENTRY (spinbutton))))
    gtk_entry_set_text (GTK_ENTRY (spinbutton), notestr);

  return (TRUE);
}

static void
swamigui_note_selector_init (SwamiguiNoteSelector *notesel)
{
  GtkAdjustment *adj;

  adj = (GtkAdjustment *)gtk_adjustment_new (60.0, 0.0, 127.0, 1.0, 12.0, 0.0);
  gtk_spin_button_configure (GTK_SPIN_BUTTON (notesel), adj, 1.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (notesel), 10);
}

/**
 * swamigui_note_selector_new:
 *
 * Create a new MIDI note selector widget.
 *
 * Returns: New MIDI note selector.
 */
GtkWidget *
swamigui_note_selector_new (void)
{
  return (GTK_WIDGET (g_object_new (SWAMIGUI_TYPE_NOTE_SELECTOR, NULL)));
}
