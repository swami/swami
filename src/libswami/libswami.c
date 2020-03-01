/*
 * libswami.c - libswami library functions and sub systems
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
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>

#include "libswami.h"
#include "builtin_enums.h"
#include "i18n.h"

/* inactive control event expiration interval in milliseconds */
#define SWAMI_CONTROL_EVENT_EXPIRE_INTERVAL 10000


/* --- private function prototypes --- */

static gboolean swami_control_event_expire_timeout(gpointer data);
static void container_add_notify(IpatchContainer *container, IpatchItem *item,
                                 gpointer user_data);
static void container_remove_notify(IpatchContainer *container,
                                    IpatchItem *item, gpointer user_data);

/* local libswami prototypes (in separate source files) */

void _swami_object_init(void);	/* SwamiObject.c */
void _swami_plugin_initialize(void);  /* SwamiPlugin.c */
void _swami_value_transform_init(void);  /* value_transform.c */

/* Ipatch property and container add/remove event controls */
/* public variables */
/* useful when libswami is used as a static library */
SwamiControl *swami_patch_prop_title_control;
SwamiControl *swami_patch_add_control;
SwamiControl *swami_patch_remove_control;

/*
 Getter function returning swami_patch_prop_title_control.
 Useful when libswami library is used as a shared library linked at load time.
*/
SwamiControl *
swami_patch_get_prop_title_control(void)
{
    return swami_patch_prop_title_control;
}

/*
 Getter function returning swami_patch_add_control.
 Useful when libswami library is used as a shared library linked at load time.
*/
SwamiControl *
swami_patch_get_add_control(void)
{
    return swami_patch_add_control;
}

/*
 Getter function returning swami_patch_remove_control.
 Useful when libswami library is used as a shared library linked at load time.
*/
SwamiControl *
swami_patch_get_remove_control(void)
{
    return swami_patch_remove_control;
}

/**
 * swami_init:
 *
 * Initialize libSwami (should be called before any other libSwami functions)
 */
void
swami_init(void)
{
    static gboolean initialized = FALSE;
    char *swap_dir, *swap_filename;

    if(initialized)
    {
        return;
    }

    initialized = TRUE;

    ipatch_init();		/* initialize libInstPatch */

    /* bind the gettext domain */
#if defined(ENABLE_NLS)
    bindtextdomain("libswami", LOCALEDIR);
#endif

    g_type_class_ref(SWAMI_TYPE_ROOT);

    /* initialize child properties and type rank systems */
    _swami_object_init();

    /* Register additional value transform functions */
    _swami_value_transform_init();

    /* initialize libswami types */
    g_type_class_ref(SWAMI_TYPE_CONTROL);
    g_type_class_ref(SWAMI_TYPE_CONTROL_FUNC);
    g_type_class_ref(SWAMI_TYPE_CONTROL_HUB);
    g_type_class_ref(SWAMI_TYPE_CONTROL_MIDI);
    g_type_class_ref(SWAMI_TYPE_CONTROL_PROP);
    g_type_class_ref(SWAMI_TYPE_CONTROL_QUEUE);
    g_type_class_ref(SWAMI_TYPE_CONTROL_VALUE);
    g_type_class_ref(SWAMI_TYPE_LOCK);
    g_type_class_ref(SWAMI_TYPE_MIDI_DEVICE);
    swami_midi_event_get_type();
    g_type_class_ref(SWAMI_TYPE_PLUGIN);
    g_type_class_ref(SWAMI_TYPE_PROP_TREE);
    g_type_class_ref(SWAMI_TYPE_WAVETBL);

    _swami_plugin_initialize();	/* initialize plugin system */

    /* create IpatchItem title property control */
    swami_patch_prop_title_control	 /* ++ ref forever */
        = SWAMI_CONTROL(swami_control_prop_new(NULL, ipatch_item_get_pspec_title()));

    /* create ipatch container event controls */
    swami_patch_add_control = swami_control_new();  /* ++ ref forever */
    swami_patch_remove_control = swami_control_new();  /* ++ ref forever */

    /* connect libInstPatch item notifies */
    ipatch_container_add_connect(NULL, container_add_notify, NULL, NULL);
    ipatch_container_remove_connect(NULL, NULL, container_remove_notify,
                                    NULL, NULL);

    /* install periodic control event expiration process */
    g_timeout_add(SWAMI_CONTROL_EVENT_EXPIRE_INTERVAL,
                  swami_control_event_expire_timeout, NULL);

    /* Construct Swami directory name for swap file (uses XDG cache directory - seems the most appropriate) */
    swap_dir = g_build_filename(g_get_user_cache_dir(), "swami", NULL);	/* ++ alloc */

    if(!g_file_test(swap_dir, G_FILE_TEST_EXISTS))
    {
        if(g_mkdir_with_parents(swap_dir, 0700) == -1)
        {
            g_critical(_("Failed to create sample swap file directory '%s': %s"),
                       swap_dir, g_strerror(errno));
            g_free(swap_dir);	/* -- free */
            return;
        }
    }

    swap_filename = g_build_filename(swap_dir, "sample_swap.dat", NULL);	/* ++ alloc */
    g_free(swap_dir);	/* -- free swap dir */

    /* assign libInstPatch sample store swap file name (uses XDG cache directory) */
    ipatch_set_sample_store_swap_file_name(swap_filename);
    g_free(swap_filename);        // -- free swap file name
}

static gboolean
swami_control_event_expire_timeout(gpointer data)
{
    swami_control_do_event_expiration();
    return (TRUE);
}


#if 0
/**
 *
 */
SwamiControlEvent *
swami_item_prop_assign_change_event(IpatchItemPropNotify *info)
{
    SwamiControlEvent *event, *origin;
    SwamiEventPropChange *prop_change;

    /* if event has already been created, return it */
    if(info->event_ptrs[0])
    {
        return (info->event_ptrs[0]);
    }

    event = swami_control_event_new(TRUE);  /* ++ ref new control event */
    g_value_init(&event->value, SWAMI_TYPE_EVENT_PROP_CHANGE);

    prop_change = swami_event_prop_change_new();
    prop_change->object = g_object_ref(info->item);  /* ++ ref object */
    prop_change->pspec = g_param_spec_ref(info->pspec);  /* ++ ref parameter spec */

    g_value_init(&prop_change->value, G_VALUE_TYPE(info->new_value));
    g_value_copy(info->new_value, &prop_change->value);

    g_value_set_boxed_take_ownership(&event->value, prop_change);

    /* IpatchItem property loop prevention, get current IpatchItem
       property origin event for this thread (if any) */
    if((origin = g_static_private_get(&prop_notify_origin)))
    {
        swami_control_event_set_origin(event, origin);
    }

    /* assign the pointer and destroy function to the event notify structure */
    IPATCH_ITEM_PROP_NOTIFY_SET_EVENT(info, 0, event,
                                      (GDestroyNotify)swami_control_event_unref);
    return (event);
}
#endif

/* IpatchContainer "add" notify callback */
static void
container_add_notify(IpatchContainer *container, IpatchItem *item,
                     gpointer user_data)
{
    SwamiControlEvent *event;

    event = swami_control_event_new(TRUE);  /* ++ ref new control event */
    g_value_init(&event->value, SWAMI_TYPE_EVENT_ITEM_ADD);

    /* boxed copy handles reference counting */
    g_value_set_boxed(&event->value, item);

    swami_control_transmit_event(swami_patch_add_control, event);

    swami_control_event_unref(event);  /* -- unref creator's reference */
}

/* IpatchContainer "remove" notify */
static void
container_remove_notify(IpatchContainer *container, IpatchItem *item,
                        gpointer user_data)
{
    SwamiControlEvent *event;
    SwamiEventItemRemove *item_remove;

    event = swami_control_event_new(TRUE);  /* ++ ref new control event */
    g_value_init(&event->value, SWAMI_TYPE_EVENT_ITEM_REMOVE);

    item_remove = swami_event_item_remove_new();
    item_remove->item = g_object_ref(item);
    item_remove->parent = ipatch_item_get_parent(item);
    g_value_take_boxed(&event->value, item_remove);

    swami_control_transmit_event(swami_patch_remove_control, event);

    swami_control_event_unref(event);  /* -- unref creator's reference */
}
