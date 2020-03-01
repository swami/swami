/*
 * SwamiguiNoteSelector.h - MIDI note selector widget.
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
#ifndef __SWAMIGUI_NOTE_SELECTOR_H__
#define __SWAMIGUI_NOTE_SELECTOR_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiNoteSelector SwamiguiNoteSelector;
typedef struct _SwamiguiNoteSelectorClass SwamiguiNoteSelectorClass;

#define SWAMIGUI_TYPE_NOTE_SELECTOR   (swamigui_note_selector_get_type ())
#define SWAMIGUI_NOTE_SELECTOR(obj) \
  (GTK_CHECK_CAST ((obj), SWAMIGUI_TYPE_NOTE_SELECTOR, SwamiguiNoteSelector))
#define SWAMIGUI_NOTE_SELECTOR_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_NOTE_SELECTOR, \
   SwamiguiNoteSelectorClass))
#define SWAMIGUI_IS_NOTE_SELECTOR(obj) \
  (GTK_CHECK_TYPE ((obj), SWAMIGUI_TYPE_NOTE_SELECTOR))
#define SWAMIGUI_IS_NOTE_SELECTOR_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_NOTE_SELECTOR))

/* MIDI note selector widget */
struct _SwamiguiNoteSelector
{
    GtkSpinButton parent_instance;
};

/* MIDI note selector class */
struct _SwamiguiNoteSelectorClass
{
    GtkSpinButtonClass parent_class;
};

GType swamigui_note_selector_get_type(void);
GtkWidget *swamigui_note_selector_new();

#endif
