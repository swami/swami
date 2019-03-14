/*
 * SwamiguiControlMidiKey.h - Header for MIDI keyboard control
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
#ifndef __SWAMIGUI_CONTROL_MIDI_KEY_H__
#define __SWAMIGUI_CONTROL_MIDI_KEY_H__

#include <gtk/gtk.h>
#include <libswami/SwamiControlMidi.h>

typedef struct _SwamiguiControlMidiKey SwamiguiControlMidiKey;
typedef struct _SwamiguiControlMidiKeyClass SwamiguiControlMidiKeyClass;

#define SWAMIGUI_TYPE_CONTROL_MIDI_KEY   (swamigui_control_midi_key_get_type ())
#define SWAMIGUI_CONTROL_MIDI_KEY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_CONTROL_MIDI_KEY, \
   SwamiguiControlMidiKey))
#define SWAMIGUI_CONTROL_MIDI_KEY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_CONTROL_MIDI_KEY, \
			    SwamiguiControlMidiKeyClass))
#define SWAMIGUI_IS_CONTROL_MIDI_KEY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_CONTROL_MIDI_KEY))
#define SWAMIGUI_IS_CONTROL_MIDI_KEY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_CONTROL_MIDI_KEY))

/* MIDI keyboard control object */
struct _SwamiguiControlMidiKey
{
  SwamiControlMidi parent_instance; /* derived from SwamiControlMidi */

  guint snooper_id;	/* key snooper handler ID */

  GArray *lower_keys;	 /* array of lower keys (see MidiKey in .c) */
  GArray *upper_keys;	 /* array of upper keys (see MidiKey in .c) */
  gint8 lower_octave;		/* lower octave (-2 - 8) */
  gint8 upper_octave;		/* upper octave (-2 - 8) */
  gboolean join_octaves;        /* if TRUE then setting lower_octave sets upper_octave + 1 */
  guint8 lower_velocity;	/* lower MIDI velocity */
  guint8 upper_velocity;	/* upper MIDI velocity */
  gboolean same_velocity;       /* if TRUE then setting lower_velocity sets upper_velocity */
  guint8 lower_channel;		/* lower MIDI channel */
  guint8 upper_channel;		/* upper MIDI channel */
};

/* MIDI keyboard control class */
struct _SwamiguiControlMidiKeyClass
{
  SwamiControlMidiClass parent_class;
};

GType swamigui_control_midi_key_get_type (void);
SwamiguiControlMidiKey *swamigui_control_midi_key_new (void);
void swamigui_control_midi_key_press (SwamiguiControlMidiKey *keyctrl,
				     guint key);
void swamigui_control_midi_key_release (SwamiguiControlMidiKey *keyctrl,
				       guint key);
void swamigui_control_midi_key_set_lower (SwamiguiControlMidiKey *keyctrl,
					 const guint *keys, guint count);
void swamigui_control_midi_key_set_upper (SwamiguiControlMidiKey *keyctrl,
					 const guint *keys, guint count);
#endif
