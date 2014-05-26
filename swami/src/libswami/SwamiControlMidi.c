/*
 * SwamiControlMidi.c - Swami MIDI control
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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "SwamiControlMidi.h"
#include "SwamiControl.h"
#include "SwamiLog.h"
#include "swami_priv.h"
#include "util.h"

static void swami_control_midi_class_init (SwamiControlMidiClass *klass);
static void swami_control_midi_init (SwamiControlMidi *ctrlmidi);

GType
swami_control_midi_get_type (void)
{
  static GType otype = 0;

  if (!otype)
    {
      static const GTypeInfo type_info =
	{
	  sizeof (SwamiControlMidiClass), NULL, NULL,
	  (GClassInitFunc) swami_control_midi_class_init,
	  (GClassFinalizeFunc) NULL, NULL,
	  sizeof (SwamiControlMidi), 0,
	  (GInstanceInitFunc) swami_control_midi_init
	};

      otype = g_type_register_static (SWAMI_TYPE_CONTROL_FUNC,
				      "SwamiControlMidi", &type_info, 0);
    }

  return (otype);
}

static void
swami_control_midi_class_init (SwamiControlMidiClass *klass)
{
  SwamiControlClass *control_class = SWAMI_CONTROL_CLASS (klass);
  control_class->set_spec = NULL; /* clear set_spec method */
}

static void
swami_control_midi_init (SwamiControlMidi *ctrlmidi)
{
  swami_control_set_flags (SWAMI_CONTROL (ctrlmidi), SWAMI_CONTROL_SENDRECV
			   | SWAMI_CONTROL_NO_CONV | SWAMI_CONTROL_NATIVE);
}

/**
 * swami_control_midi_new:
 *
 * Create a new MIDI control.
 *
 * Returns: New MIDI control with a refcount of 1 which the caller owns.
 */
SwamiControlMidi *
swami_control_midi_new (void)
{
  return (SWAMI_CONTROL_MIDI (g_object_new (SWAMI_TYPE_CONTROL_MIDI, NULL)));
}

/**
 * swami_control_midi_set_callback:
 * @midi: MIDI control
 * @callback: Function to callback when a new event is received or %NULL
 * @data: User defined data to pass to the callback
 *
 * Set a callback function for received events on a MIDI control.
 */
void
swami_control_midi_set_callback (SwamiControlMidi *midi,
				 SwamiControlSetValueFunc callback,
				 gpointer data)
{
  g_return_if_fail (SWAMI_IS_CONTROL_MIDI (midi));

  swami_control_func_assign_funcs (SWAMI_CONTROL_FUNC (midi), NULL,
				   callback, NULL, data);
}

/**
 * swami_control_midi_send:
 * @midi: MIDI control
 * @type: MIDI event type
 * @channel: MIDI channel to send on
 * @param1: First parameter
 * @param2: Second parameter (only used with certain event types)
 *
 * A convenience function to send an event TO a MIDI control. One
 * could do the same by creating a #SwamiMidiEvent, calling
 * swami_midi_event_set() on it and then setting the
 * control with swami_control_set_value().
 */
void
swami_control_midi_send (SwamiControlMidi *midi, SwamiMidiEventType type,
			 int channel, int param1, int param2)
{
  SwamiMidiEvent *event;
  GValue value = { 0 };

  g_return_if_fail (SWAMI_IS_CONTROL_MIDI (midi));

  event = swami_midi_event_new ();
  swami_midi_event_set (event, type, channel, param1, param2);

  g_value_init (&value, SWAMI_TYPE_MIDI_EVENT);
  g_value_take_boxed (&value, event);

  swami_control_set_value (SWAMI_CONTROL (midi), &value);

  g_value_unset (&value);
}

/**
 * swami_control_midi_transmit:
 * @midi: MIDI control
 * @type: MIDI event type
 * @channel: MIDI channel to send on
 * @param1: First parameter
 * @param2: Second parameter (only used with certain event types)
 *
 * A convenience function to send an event FROM a MIDI control. One
 * could do the same by creating a #SwamiMidiEvent, calling
 * swami_midi_event_set() on it and then transmitting it from the
 * control with swami_control_transmit_value().
 */
void
swami_control_midi_transmit (SwamiControlMidi *midi, SwamiMidiEventType type,
			     int channel, int param1, int param2)
{
  SwamiMidiEvent *event;
  GValue value = { 0 };

  g_return_if_fail (SWAMI_IS_CONTROL_MIDI (midi));

  event = swami_midi_event_new ();
  swami_midi_event_set (event, type, channel, param1, param2);

  g_value_init (&value, SWAMI_TYPE_MIDI_EVENT);
  g_value_take_boxed (&value, event);

  swami_control_transmit_value (SWAMI_CONTROL (midi), &value);

  g_value_unset (&value);
}
