/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#include "compat.h"

#ifdef IPATCH_COMPAT_GWEAKREF

/* Older glib implementation of GWeakRef (not thread safe!) */

/**
 * g_weak_ref_init: (skip)
 */
void
g_weak_ref_init (GWeakRef *weak_ref, gpointer object)
{
  weak_ref->obj = object;
  if (object) g_object_add_weak_pointer ((GObject *)object, &weak_ref->obj);
}

/**
 * g_weak_ref_clear: (skip)
 */
void
g_weak_ref_clear (GWeakRef *weak_ref)
{
  GObject *object;

  object = weak_ref->obj;

  if (object)
  {
    g_object_ref (object);      // ++ ref
    g_object_remove_weak_pointer ((GObject *)object, &weak_ref->obj);
    g_object_unref (object);    // -- unref
    weak_ref->obj = NULL;
  }
}

/**
 * g_weak_ref_get: (skip)
 */
gpointer
g_weak_ref_get (GWeakRef *weak_ref)
{
  GObject *object;

  object = weak_ref->obj;

  if (object) g_object_ref (object);      // ++ ref

  return (object);              // !! caller takes over reference
}

/**
 * g_weak_ref_set: (skip)
 */
void
g_weak_ref_set (GWeakRef *weak_ref, gpointer object)
{
  g_weak_ref_clear (weak_ref);
  g_weak_ref_init (weak_ref, object);
}

#endif


#ifdef IPATCH_COMPAT_SLIST_FREE_FULL

/**
 * g_slist_free_full: (skip)
 */
void
g_slist_free_full (GSList *list, GDestroyNotify free_func)
{
  GSList *p;

  for (p = list; p; p = g_slist_delete_link (p, p))
    if (free_func)
      free_func (p->data);
}

#endif

