/*
 * SwamiControlMidi.h - Header for Swami MIDI control
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
#ifndef __SWAMI_CONTROL_MIDI_H__
#define __SWAMI_CONTROL_MIDI_H__

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiControl.h>
#include <libswami/SwamiMidiEvent.h>
#include <libswami/SwamiControlFunc.h>

typedef struct _SwamiControlMidi SwamiControlMidi;
typedef struct _SwamiControlMidiClass SwamiControlMidiClass;

#define SWAMI_TYPE_CONTROL_MIDI   (swami_control_midi_get_type ())
#define SWAMI_CONTROL_MIDI(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_MIDI, \
   SwamiControlMidi))
#define SWAMI_CONTROL_MIDI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_MIDI, \
			    SwamiControlMidiClass))
#define SWAMI_IS_CONTROL_MIDI(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_MIDI))
#define SWAMI_IS_CONTROL_MIDI_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTROL_MIDI))

/* MIDI control object */
struct _SwamiControlMidi
{
    SwamiControlFunc parent_instance; /* derived from SwamiControlFunc */
};

/* MIDI control class */
struct _SwamiControlMidiClass
{
    SwamiControlFuncClass parent_class;
};

GType swami_control_midi_get_type(void);
SwamiControlMidi *swami_control_midi_new(void);
void swami_control_midi_set_callback(SwamiControlMidi *midi,
                                     SwamiControlSetValueFunc callback,
                                     gpointer data);
void swami_control_midi_send(SwamiControlMidi *midi,
                             SwamiMidiEventType type,
                             int channel, int param1, int param2);
void swami_control_midi_transmit(SwamiControlMidi *midi,
                                 SwamiMidiEventType type,
                                 int channel, int param1, int param2);

#endif
