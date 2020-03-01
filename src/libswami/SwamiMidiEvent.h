/*
 * SwamiMidiEvent.h - Header for MIDI event object
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
#ifndef __SWAMI_MIDI_EVENT_H__
#define __SWAMI_MIDI_EVENT_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _SwamiMidiEvent SwamiMidiEvent;
typedef struct _SwamiMidiEventNote SwamiMidiEventNote;
typedef struct _SwamiMidiEventControl SwamiMidiEventControl;

#define SWAMI_TYPE_MIDI_EVENT   (swami_midi_event_get_type ())

/* MIDI event type (in comments Control = SwamiMidiEventControl and
   Note = SwamiMidiEventNote) */
typedef enum
{
    SWAMI_MIDI_NONE,		/* NULL event */
    SWAMI_MIDI_NOTE,		/* a note interval (Note) */
    SWAMI_MIDI_NOTE_ON,		/* note on event (Note) */
    SWAMI_MIDI_NOTE_OFF,		/* note off event (Note) */
    SWAMI_MIDI_KEY_PRESSURE,	/* key pressure (Note) */
    SWAMI_MIDI_PITCH_BEND, /* pitch bend event -8192 - 8191 (Control) */
    SWAMI_MIDI_PROGRAM_CHANGE,	/* program change (Control) */
    SWAMI_MIDI_CONTROL,		/* 7 bit controller (Control) */
    SWAMI_MIDI_CONTROL14,		/* 14 bit controller (Control) */
    SWAMI_MIDI_CHAN_PRESSURE,	/* channel pressure (Control) */
    SWAMI_MIDI_RPN,		/* registered param (Control) */
    SWAMI_MIDI_NRPN,		/* non registered param (Control) */


    /* these are used as a convenience for swami_midi_event_set() but they
       should not appear in the event type field, they are handled by other
       events above */
    SWAMI_MIDI_BEND_RANGE,
    SWAMI_MIDI_BANK_SELECT
} SwamiMidiEventType;

/* structure defining parameters of a note event */
struct _SwamiMidiEventNote
{
    guint8 note;
    guint8 velocity; /* _NOTE_ON, _NOTE_OFF, _KEY_PRESSURE, or _NOTE events */
    guint8 off_velocity;		/* _NOTE event only */
    guint8 unused;
    guint duration;		/* for SWAMI_MIDI_NOTE event only */
};

/* some standard General MIDI custom controllers */
#define SWAMI_MIDI_CC_BANK_MSB		0
#define SWAMI_MIDI_CC_MODULATION	1
#define SWAMI_MIDI_CC_VOLUME		7
#define SWAMI_MIDI_CC_PAN		10
#define SWAMI_MIDI_CC_EXPRESSION	11
#define SWAMI_MIDI_CC_BANK_LSB		32
#define SWAMI_MIDI_CC_SUSTAIN		64
#define SWAMI_MIDI_CC_REVERB		91
#define SWAMI_MIDI_CC_CHORUS		93

/* standard registered parameter numbers */
#define SWAMI_MIDI_RPN_BEND_RANGE	0
#define SWAMI_MIDI_RPN_MASTER_TUNE	1

struct _SwamiMidiEventControl
{
    guint param;			/* control number */
    int value;			/* control value */
};

struct _SwamiMidiEvent
{
    SwamiMidiEventType type;
    int channel;	     /* most events send on a specific MIDI channel */

    union
    {
        SwamiMidiEventNote note;
        SwamiMidiEventControl control;
    } data;
};

GType swami_midi_event_get_type(void);
SwamiMidiEvent *swami_midi_event_new(void);
void swami_midi_event_free(SwamiMidiEvent *event);
SwamiMidiEvent *swami_midi_event_copy(SwamiMidiEvent *event);

void swami_midi_event_set(SwamiMidiEvent *event, SwamiMidiEventType type,
                          int channel, int param1, int param2);

void swami_midi_event_note_on(SwamiMidiEvent *event, int channel,
                              int note, int velocity);
void swami_midi_event_note_off(SwamiMidiEvent *event, int channel,
                               int note, int velocity);
void swami_midi_event_bank_select(SwamiMidiEvent *event, int channel,
                                  int bank);
void swami_midi_event_program_change(SwamiMidiEvent *event, int channel,
                                     int program);
void swami_midi_event_bend_range(SwamiMidiEvent *event, int channel,
                                 int cents);
void swami_midi_event_pitch_bend(SwamiMidiEvent *event, int channel,
                                 int value);
void swami_midi_event_control(SwamiMidiEvent *event, int channel,
                              int ctrlnum, int value);
void swami_midi_event_control14(SwamiMidiEvent *event, int channel,
                                int ctrlnum, int value);
void swami_midi_event_rpn(SwamiMidiEvent *event, int channel, int paramnum,
                          int value);
void swami_midi_event_nrpn(SwamiMidiEvent *event, int channel, int paramnum,
                           int value);

#endif
