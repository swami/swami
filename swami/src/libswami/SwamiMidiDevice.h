/*
 * SwamiMidiDevice.h - Swami MIDI object header file
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
#ifndef __SWAMI_MIDI_DEVICE_H__
#define __SWAMI_MIDI_DEVICE_H__

#include <glib.h>
#include <libinstpatch/libinstpatch.h>

#include <glib-object.h>

#include <libswami/SwamiControl.h>
#include <libswami/SwamiLock.h>

typedef struct _SwamiMidiDevice SwamiMidiDevice;
typedef struct _SwamiMidiDeviceClass SwamiMidiDeviceClass;

#define SWAMI_TYPE_MIDI_DEVICE   (swami_midi_device_get_type ())
#define SWAMI_MIDI_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_MIDI_DEVICE, SwamiMidiDevice))
#define SWAMI_MIDI_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_MIDI_DEVICE, SwamiMidiDeviceClass))
#define SWAMI_IS_MIDI_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_MIDI_DEVICE))
#define SWAMI_IS_MIDI_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_MIDI_DEVICE))

/* Swami MIDI device object */
struct _SwamiMidiDevice
{
  SwamiLock parent_instance;	/* derived from GObject */

  /*< private >*/
  gboolean active;		/* driver is active? */
};

/* Swami MIDI device class */
struct _SwamiMidiDeviceClass
{
  SwamiLockClass parent_class;

  /*< public >*/

  /* methods */
  gboolean (*open) (SwamiMidiDevice *device, GError **err);
  void (*close) (SwamiMidiDevice *device);
  SwamiControl * (*get_control) (SwamiMidiDevice *device, int index);
};

GType swami_midi_device_get_type (void);

gboolean swami_midi_device_open (SwamiMidiDevice *device, GError **err);
void swami_midi_device_close (SwamiMidiDevice *device);
SwamiControl *swami_midi_device_get_control (SwamiMidiDevice *device,
					     int index);
#endif
