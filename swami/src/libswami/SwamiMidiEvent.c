/*
 * SwamiMidiEvent.c - MIDI event object
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
#include <glib.h>

#include "SwamiMidiEvent.h"
#include "swami_priv.h"

GType
swami_midi_event_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
    item_type = g_boxed_type_register_static ("SwamiMidiEvent",
				(GBoxedCopyFunc)swami_midi_event_copy,
				(GBoxedFreeFunc)swami_midi_event_free);

  return (item_type);
}

/**
 * swami_midi_event_new:
 *
 * Create a new MIDI event structure.
 *
 * Returns: New MIDI event structure. Free it with swami_midi_event_free()
 * when finished.
 */
SwamiMidiEvent *
swami_midi_event_new (void)
{
  SwamiMidiEvent *event;

  event = g_slice_new0 (SwamiMidiEvent);
  event->type = SWAMI_MIDI_NONE;
  return (event);
}

/**
 * swami_midi_event_free:
 * @event: MIDI event structure to free
 * 
 * Free a MIDI event previously allocated by swami_midi_event_new().
 */
void
swami_midi_event_free (SwamiMidiEvent *event)
{
  g_slice_free (SwamiMidiEvent, event);
}

/**
 * swami_midi_event_copy:
 * @event: MIDI event structure to duplicate
 * 
 * Duplicate a MIDI event structure.
 * 
 * Returns: New duplicate MIDI event. Free it with swami_midi_event_free()
 * when finished.
 */
SwamiMidiEvent *
swami_midi_event_copy (SwamiMidiEvent *event)
{
  SwamiMidiEvent *new_event;

  new_event = swami_midi_event_new ();
  *new_event = *event;
  return (new_event);
}

/**
 * swami_midi_event_set:
 * @event: MIDI event structure
 * @type: Type of event to assign
 * @channel: MIDI channel to send to
 * @param1: First parameter of event @type
 * @param2: Second parameter of event @type (only for some types)
 *
 * A single entry point for all event types if one does not care to use
 * the event type specific functions.
 */
void
swami_midi_event_set (SwamiMidiEvent *event, SwamiMidiEventType type,
		      int channel, int param1, int param2)
{
  SwamiMidiEventNote *note;
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  note = &event->data.note;
  ctrl = &event->data.control;

  event->type = type;
  event->channel = channel;

  switch (type)
    {
    case SWAMI_MIDI_NOTE_ON:
      if (param2 == 0) event->type = SWAMI_MIDI_NOTE_OFF; /* velocity is 0? */
      /* fall through */
    case SWAMI_MIDI_NOTE_OFF:
    case SWAMI_MIDI_KEY_PRESSURE:
      note->note = param1;
      note->velocity = param2;
      break;
    case SWAMI_MIDI_PITCH_BEND:
      ctrl->param = param1;
      ctrl->value = param2;
      break;
    case SWAMI_MIDI_PROGRAM_CHANGE:
      ctrl->value = param1;
      break;
    case SWAMI_MIDI_CONTROL:
    case SWAMI_MIDI_CONTROL14:
      ctrl->param = param1;
      ctrl->value = param2;
      break;
    case SWAMI_MIDI_CHAN_PRESSURE:
      ctrl->value = param1;
      break;
    case SWAMI_MIDI_RPN:
    case SWAMI_MIDI_NRPN:
      ctrl->param = param1;
      ctrl->value = param2;
      break;

    /* these are handled by other event types, convenience only */

    case SWAMI_MIDI_BEND_RANGE:
      event->type = SWAMI_MIDI_RPN;
      ctrl->param = SWAMI_MIDI_RPN_BEND_RANGE;
      ctrl->value = param1;
      break;
    case SWAMI_MIDI_BANK_SELECT:
      event->type = SWAMI_MIDI_CONTROL14;
      ctrl->param = SWAMI_MIDI_CC_BANK_MSB;
      ctrl->value = param1;
      break;
    default:
      g_warning ("Unknown MIDI event type");
      event->type = SWAMI_MIDI_NONE;
      break;
    }
}

/**
 * swami_midi_event_note_on:
 * @event: MIDI event structure
 * @channel: MIDI channel to send note on event to
 * @note: MIDI note number
 * @velocity: MIDI note velocity
 *
 * Make a MIDI event structure a note on event.
 */
void
swami_midi_event_note_on (SwamiMidiEvent *event, int channel, int note,
			  int velocity)
{
  SwamiMidiEventNote *n;

  g_return_if_fail (event != NULL);

  event->type = (velocity != 0) ? SWAMI_MIDI_NOTE_ON : SWAMI_MIDI_NOTE_OFF;
  event->channel = channel;
  n = &event->data.note;
  n->note = note;
  n->velocity = velocity;
}

/**
 * swami_midi_event_note_off:
 * @event: MIDI event structure
 * @channel: MIDI channel to send note off event to
 * @note: MIDI note number
 * @velocity: MIDI note velocity
 *
 * Make a MIDI event structure a note off event.
 */
void
swami_midi_event_note_off (SwamiMidiEvent *event, int channel, int note,
			   int velocity)
{
  SwamiMidiEventNote *n;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_NOTE_OFF;
  event->channel = channel;
  n = &event->data.note;
  n->note = note;
  n->velocity = velocity;
}

/**
 * swami_midi_event_bank_select:
 * @event: MIDI event structure
 * @channel: MIDI channel to send bank select to
 * @bank: Bank number to change to
 *
 * Make a MIDI event structure a bank select event. A convenience function
 * since a bank select is just a controller event, and is sent as such.
 */
void
swami_midi_event_bank_select (SwamiMidiEvent *event, int channel, int bank)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_CONTROL14;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = SWAMI_MIDI_CC_BANK_MSB;
  ctrl->value = bank;
}

/**
 * swami_midi_event_program_change:
 * @event: MIDI event structure
 * @channel: MIDI channel to send program change to
 * @program: Program number to change to
 *
 * Set a MIDI event structure to a program change event.
 */
void
swami_midi_event_set_program (SwamiMidiEvent *event, int channel, int program)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_PROGRAM_CHANGE;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->value = program;
}

/**
 * swami_midi_event_bend_range:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event to
 * @cents: Bend range in cents (100th of a semitone)
 *
 * Make a MIDI event structure a bend range event. A convenience since one
 * could also use the swami_midi_event_rpn() function.
 */
void
swami_midi_event_set_bend_range (SwamiMidiEvent *event, int channel, int cents)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_RPN;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = SWAMI_MIDI_RPN_BEND_RANGE;
  ctrl->value = cents;
}

/**
 * swami_midi_event_pitch_bend:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event to
 * @value: MIDI pitch bend value (-8192 - 8191, 0 = center)
 *
 * Make a MIDI event structure a pitch bend event.
 */
void
swami_midi_event_pitch_bend (SwamiMidiEvent *event, int channel, int value)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_PITCH_BEND;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->value = value;
}

/**
 * swami_midi_event_control:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event on
 * @ctrlnum: Control number
 * @value: 7 bit value to set control to
 *
 * Make a MIDI event structure a 7 bit controller event.
 */
void
swami_midi_event_control (SwamiMidiEvent *event, int channel,
			  int ctrlnum, int value)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_CONTROL;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = ctrlnum;
  ctrl->value = value;
}

/**
 * swami_midi_event_control14:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event on
 * @ctrlnum: Control number
 * @value: 14 bit value to set control to
 *
 * Make a MIDI event structure a 14 bit controller event. The ctrlnum should
 * be one of the first 32 MIDI controllers that have MSB and LSB controllers
 * for sending 14 bit values. Either the MSB or LSB controller number may
 * be used (example: either 0 or 32 can be used for setting 14 bit bank
 * number). Internally the MSB controller numbers are used (0-31).
 */
void
swami_midi_event_control14 (SwamiMidiEvent *event, int channel,
			    int ctrlnum, int value)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);
  g_return_if_fail (ctrlnum >= 0 && ctrlnum <= 63);

  if (ctrlnum > 31) ctrlnum -= 32;

  event->type = SWAMI_MIDI_CONTROL14;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = ctrlnum;
  ctrl->value = value;
}

/**
 * swami_midi_event_rpn:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event to
 * @paramnum: RPN parameter number
 * @value: 14 bit value to set parameter to
 *
 * Make a MIDI event structure a RPN (registered parameter number) event.
 */
void
swami_midi_event_rpn (SwamiMidiEvent *event, int channel, int paramnum,
		      int value)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_RPN;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = paramnum;
  ctrl->value = value;
}

/**
 * swami_midi_event_nrpn:
 * @event: MIDI event structure
 * @channel: MIDI channel to send event to
 * @paramnum: NRPN parameter number
 * @value: 14 bit value to set parameter to
 *
 * Make a MIDI event structure a NRPN (non-registered parameter number) event.
 */
void
swami_midi_event_nrpn (SwamiMidiEvent *event, int channel, int paramnum,
		       int value)
{
  SwamiMidiEventControl *ctrl;

  g_return_if_fail (event != NULL);

  event->type = SWAMI_MIDI_NRPN;
  event->channel = channel;
  ctrl = &event->data.control;
  ctrl->param = paramnum;
  ctrl->value = value;
}
