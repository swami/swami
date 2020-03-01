/*
 * SwamiControlHub.c - Control hub object
 * Re-transmits any events it receives
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

#include "SwamiControlHub.h"
#include "SwamiControl.h"
#include "SwamiLog.h"
#include "swami_priv.h"
#include "util.h"

static void swami_control_hub_class_init(SwamiControlHubClass *klass);
static void control_hub_set_value_method(SwamiControl *control,
        SwamiControlEvent *event,
        const GValue *value);
static void swami_control_hub_init(SwamiControlHub *ctrlhub);

GType
swami_control_hub_get_type(void)
{
    static GType otype = 0;

    if(!otype)
    {
        static const GTypeInfo type_info =
        {
            sizeof(SwamiControlHubClass), NULL, NULL,
            (GClassInitFunc) swami_control_hub_class_init,
            (GClassFinalizeFunc) NULL, NULL,
            sizeof(SwamiControlHub), 0,
            (GInstanceInitFunc) swami_control_hub_init
        };

        otype = g_type_register_static(SWAMI_TYPE_CONTROL, "SwamiControlHub",
                                       &type_info, 0);
    }

    return (otype);
}

static void
swami_control_hub_class_init(SwamiControlHubClass *klass)
{
    SwamiControlClass *control_class = SWAMI_CONTROL_CLASS(klass);
    control_class->set_value = control_hub_set_value_method;
}

static void
control_hub_set_value_method(SwamiControl *control, SwamiControlEvent *event,
                             const GValue *value)
{
    /* re-transmit the event (without loop check) */
    swami_control_transmit_event_loop(control, event);
}

static void
swami_control_hub_init(SwamiControlHub *ctrlhub)
{
    /* hubs send and receive */
    swami_control_set_flags(SWAMI_CONTROL(ctrlhub), SWAMI_CONTROL_SENDRECV);
}

/**
 * swami_control_hub_new:
 *
 * Create a new control hub. Control hubes re-transmit any events they
 * receive and are useful for connecting many controls together.
 *
 * Returns: New control hub with a refcount of 1 which the caller owns.
 */
SwamiControlHub *
swami_control_hub_new(void)
{
    return (SWAMI_CONTROL_HUB(g_object_new(SWAMI_TYPE_CONTROL_HUB, NULL)));
}
