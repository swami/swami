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
#ifndef __IPATCH_SAMPLE_STORE_CACHE_H__
#define __IPATCH_SAMPLE_STORE_CACHE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleStore.h>

/* forward type declarations */

typedef struct _IpatchSampleStoreCache IpatchSampleStoreCache;
typedef struct _IpatchSampleStoreCacheClass IpatchSampleStoreCacheClass;

#define IPATCH_TYPE_SAMPLE_STORE_CACHE (ipatch_sample_store_cache_get_type ())
#define IPATCH_SAMPLE_STORE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_CACHE, \
   IpatchSampleStoreCache))
#define IPATCH_SAMPLE_STORE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_CACHE, \
   IpatchSampleStoreCacheClass))
#define IPATCH_IS_SAMPLE_STORE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_CACHE))
#define IPATCH_IS_SAMPLE_STORE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_CACHE))

/* RAM sample store instance */
struct _IpatchSampleStoreCache
{
  IpatchSampleStore parent_instance;
  gpointer location;    /* Pointer to the sample data in memory */
  guint32 channel_map;  /* Channel map of cached sample in reference to native sample */
  glong last_open;      /* Unix time of last open or 0 if currently open */
  int open_count;       /* Current number of opens (atomic int) */
};

/* RAM sample store class */
struct _IpatchSampleStoreCacheClass
{
  IpatchSampleStoreClass parent_class;
};

/**
 * ipatch_sample_store_cache_get_location:
 * @store: Sample store to get sample data location from
 *
 * Macro to quickly fetch a cache sample store's data location pointer.
 *
 * Returns: Sample data pointer.
 */
#define ipatch_sample_store_cache_get_location(store) \
  ((store)->location)   /* No lock needed, set only once during creation */

/**
 * ipatch_sample_store_cache_get_channel_map:
 * @store: Sample store to get channel map from
 *
 * Macro to quickly fetch a cache sample store's channel map value.  Cached
 * samples store a channel map in reference to the native sample of their
 * parent #IpatchSampleData.
 *
 * Returns: Channel map value.
 */
#define ipatch_sample_store_cache_get_channel_map(store) \
  ((store)->channel_map)   /* No lock needed, set only once during creation */

/* Used by #IpatchSampleData */
#define ipatch_sample_store_cache_get_open_count(store) \
  g_atomic_int_get (&((store)->open_count))

/**
 * IPATCH_SAMPLE_STORE_CACHE_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve 4 bits for future expansion */
#define IPATCH_SAMPLE_STORE_CACHE_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT + 4)

GType ipatch_sample_store_cache_get_type (void);
IpatchSample *ipatch_sample_store_cache_new (gpointer location);
void ipatch_sample_store_cache_open (IpatchSampleStoreCache *store);
void ipatch_sample_store_cache_close (IpatchSampleStoreCache *store);

#endif
