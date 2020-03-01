/*
 * SwamiMidiDevice.c - MIDI device base class
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
#include "config.h"

#include <stdio.h>
#include <glib.h>

#include "SwamiMidiDevice.h"
#include "SwamiLog.h"


/* --- signals and properties --- */

enum
{
    LAST_SIGNAL
};

enum
{
    PROP_0,
};

/* --- private function prototypes --- */

static void swami_midi_device_class_init(SwamiMidiDeviceClass *klass);
static void swami_midi_device_init(SwamiMidiDevice *device);


/* --- private data --- */


// static guint midi_signals[LAST_SIGNAL] = { 0 };


/* --- functions --- */


GType
swami_midi_device_get_type(void)
{
    static GType item_type = 0;

    if(!item_type)
    {
        static const GTypeInfo item_info =
        {
            sizeof(SwamiMidiDeviceClass), NULL, NULL,
            (GClassInitFunc) swami_midi_device_class_init, NULL, NULL,
            sizeof(SwamiMidiDevice), 0,
            (GInstanceInitFunc) swami_midi_device_init,
        };

        item_type = g_type_register_static(SWAMI_TYPE_LOCK, "SwamiMidiDevice",
                                           &item_info, G_TYPE_FLAG_ABSTRACT);
    }

    return (item_type);
}

static void
swami_midi_device_class_init(SwamiMidiDeviceClass *klass)
{
}

static void
swami_midi_device_init(SwamiMidiDevice *midi)
{
    midi->active = FALSE;
}

SwamiMidiDevice *
swami_midi_device_new(void)
{
    return SWAMI_MIDI_DEVICE(g_object_new(SWAMI_TYPE_MIDI_DEVICE, NULL));
}

/**
 * swami_midi_device_open:
 * @device: Swami MIDI device
 * @err: Location to store error information or %NULL
 *
 * Open a MIDI device.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_midi_device_open(SwamiMidiDevice *device, GError **err)
{
    SwamiMidiDeviceClass *oclass;
    int retval = TRUE;

    g_return_val_if_fail(SWAMI_IS_MIDI_DEVICE(device), FALSE);
    g_return_val_if_fail(!err || !*err, FALSE);

    oclass = SWAMI_MIDI_DEVICE_CLASS(G_OBJECT_GET_CLASS(device));

    SWAMI_LOCK_WRITE(device);

    if(!device->active)
    {
        if(oclass->open)
        {
            retval = (*oclass->open)(device, err);
        }

        if(retval)
        {
            device->active = TRUE;
        }
    }

    SWAMI_UNLOCK_WRITE(device);

    return (retval);
}

/**
 * swami_midi_device_close:
 * @device: MIDI device object
 *
 * Close an active MIDI device.
 */
void
swami_midi_device_close(SwamiMidiDevice *device)
{
    SwamiMidiDeviceClass *oclass;

    g_return_if_fail(SWAMI_IS_MIDI_DEVICE(device));

    oclass = SWAMI_MIDI_DEVICE_CLASS(G_OBJECT_GET_CLASS(device));

    SWAMI_LOCK_WRITE(device);

    if(device->active)
    {
        if(oclass->close)
        {
            (*oclass->close)(device);
        }

        device->active = FALSE;
    }

    SWAMI_UNLOCK_WRITE(device);
}

/**
 * swami_midi_device_get_control:
 * @device: MIDI device object
 * @index: Index of control
 *
 * Get a MIDI control object from a MIDI device. A MIDI device may have
 * multiple MIDI control interface channels (if supporting more than 16
 * MIDI channels for example), so index can be used to iterate over them.
 * The MIDI device does NOT need to be active when calling this function.
 *
 * Returns: The MIDI control for the given @index, or %NULL if no control
 * with that index. The reference count of the returned control has been
 * incremented and is owned by the caller, remember to unref it when finished.
 */
SwamiControl *
swami_midi_device_get_control(SwamiMidiDevice *device, int index)
{
    SwamiMidiDeviceClass *oclass;
    SwamiControl *control = NULL;

    g_return_val_if_fail(SWAMI_IS_MIDI_DEVICE(device), NULL);

    oclass = SWAMI_MIDI_DEVICE_CLASS(G_OBJECT_GET_CLASS(device));

    if(oclass->get_control)
    {
        control = (*oclass->get_control)(device, index);

        if(control)
        {
            g_object_ref(control);    /* ++ ref for caller */
        }
    }

    return (control);
}
