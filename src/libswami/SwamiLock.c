/*
 * SwamiLock.c - Base Swami multi-thread locked object
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
#include <glib-object.h>

#include "SwamiLock.h"


/* --- private function prototypes --- */

static void swami_lock_class_init(SwamiLockClass *klass);
static void swami_lock_init(SwamiLock *lock);
static void swami_lock_finalize(GObject *object);

G_DEFINE_ABSTRACT_TYPE(SwamiLock, swami_lock, G_TYPE_OBJECT);

/* --- functions --- */


static void
swami_lock_class_init(SwamiLockClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);
    obj_class->finalize = swami_lock_finalize;
}

static void
swami_lock_init(SwamiLock *lock)
{
    g_static_rec_mutex_init(&lock->mutex);
}

static void
swami_lock_finalize(GObject *object)
{
    SwamiLock *lock = SWAMI_LOCK(object);

    g_static_rec_mutex_free(&lock->mutex);

    if(G_OBJECT_CLASS(swami_lock_parent_class)->finalize)
    {
        G_OBJECT_CLASS(swami_lock_parent_class)->finalize(object);
    }
}

/**
 * swami_lock_set_atomic:
 * @lock: SwamiLock derived object to set properties of
 * @first_property_name: Name of first property
 * @Varargs: Variable list of arguments that should start with the value to
 * set @first_property_name to, followed by property name/value pairs. List is
 * terminated with a %NULL property name.
 *
 * Sets properties on a Swami lock item atomically (i.e. item is
 * multi-thread locked while all properties are set). This avoids
 * critical parameter sync problems when multiple threads are
 * accessing the same item. See g_object_set() for more information on
 * setting properties. This function is rarely needed, only useful for cases
 * where multiple properties depend on each other.
 */
void
swami_lock_set_atomic(gpointer lock, const char *first_property_name, ...)
{
    va_list args;

    g_return_if_fail(SWAMI_IS_LOCK(lock));

    va_start(args, first_property_name);

    SWAMI_LOCK_WRITE(lock);
    g_object_set_valist(G_OBJECT(lock), first_property_name, args);
    SWAMI_UNLOCK_WRITE(lock);

    va_end(args);
}

/**
 * swami_lock_get_atomic:
 * @lock: SwamiLock derived object to get properties from
 * @first_property_name: Name of first property
 * @Varargs: Variable list of arguments that should start with a
 * pointer to store the value from @first_property_name, followed by
 * property name/value pointer pairs. List is terminated with a %NULL
 * property name.
 *
 * Gets properties from a Swami lock item atomically (i.e. item is
 * multi-thread locked while all properties are retrieved). This
 * avoids critical parameter sync problems when multiple threads are
 * accessing the same item. See g_object_get() for more information on
 * getting properties. This function is rarely needed, only useful when
 * multiple properties depend on each other.
 */
void
swami_lock_get_atomic(gpointer lock, const char *first_property_name, ...)
{
    va_list args;

    g_return_if_fail(SWAMI_IS_LOCK(lock));

    va_start(args, first_property_name);

    SWAMI_LOCK_WRITE(lock);
    g_object_get_valist(G_OBJECT(lock), first_property_name, args);
    SWAMI_UNLOCK_WRITE(lock);

    va_end(args);
}
