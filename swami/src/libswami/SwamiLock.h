/*
 * SwamiLock.h - Header for Swami multi-threaded locked base object class
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
#ifndef __SWAMI_LOCK_H__
#define __SWAMI_LOCK_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _SwamiLock SwamiLock;
typedef struct _SwamiLockClass SwamiLockClass;

#define SWAMI_TYPE_LOCK   (swami_lock_get_type ())
#define SWAMI_LOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_LOCK, SwamiLock))
#define SWAMI_LOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_LOCK, SwamiLockClass))
#define SWAMI_IS_LOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_LOCK))
#define SWAMI_IS_LOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_LOCK))

struct _SwamiLock
{
  GObject parent_instance;
  GStaticRecMutex mutex;
};

struct _SwamiLockClass
{
  GObjectClass parent_class;
};

/* Multi-thread locking macros. For now there is no distinction between
   write and read locking since GStaticRWLock is not recursive. */
#define SWAMI_LOCK_WRITE(lock)	\
    g_static_rec_mutex_lock (&((SwamiLock *)(lock))->mutex)
#define SWAMI_UNLOCK_WRITE(lock) \
    g_static_rec_mutex_unlock (&((SwamiLock *)(lock))->mutex)
#define SWAMI_LOCK_READ(lock)	SWAMI_LOCK_WRITE(lock)
#define SWAMI_UNLOCK_READ(lock)	SWAMI_UNLOCK_WRITE(lock)

GType swami_lock_get_type (void);
void swami_lock_set_atomic (gpointer lock,
			    const char *first_property_name, ...);
void swami_lock_get_atomic (gpointer lock,
			    const char *first_property_name, ...);

#endif
