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
#ifndef __IPATCH_SAMPLE_STORE_H__
#define __IPATCH_SAMPLE_STORE_H__

/* forward type declarations */

typedef struct _IpatchSampleStore IpatchSampleStore;
typedef struct _IpatchSampleStoreClass IpatchSampleStoreClass;

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSample.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSampleTransform.h>
#include <libinstpatch/sample.h>

#define IPATCH_TYPE_SAMPLE_STORE (ipatch_sample_store_get_type ())
#define IPATCH_SAMPLE_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE, \
   IpatchSampleStore))
#define IPATCH_SAMPLE_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE, \
   IpatchSampleStoreClass))
#define IPATCH_IS_SAMPLE_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE))
#define IPATCH_IS_SAMPLE_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE))
#define IPATCH_SAMPLE_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SAMPLE_STORE, \
   IpatchSampleStoreClass))

/* sample store base instance */
struct _IpatchSampleStore
{
  IpatchItem parent_instance;

  guint32 size;			/* size of sample data (in samples) */
  guint32 rate;			/* sample rate in hz */
};

/* sample store base class */
struct _IpatchSampleStoreClass
{
  IpatchItemClass parent_class;
};


/* IpatchSampleWidth | sign | endian | channels stored in flags field
   (9 bits) - see sample.h */

/* flags bit shift value to sample format field */
#define IPATCH_SAMPLE_STORE_FORMAT_SHIFT IPATCH_ITEM_UNUSED_FLAG_SHIFT

/**
 * ipatch_sample_store_get_format:
 *
 * Macro for getting the sample format from a sample store.  No lock is required
 * since format can only be set prior to the store being actively used.
 *
 * Returns: Sample format field.  See #sample.
 */
#define ipatch_sample_store_get_format(store) \
  ((ipatch_item_get_flags (store) >> IPATCH_SAMPLE_STORE_FORMAT_SHIFT) \
   & IPATCH_SAMPLE_FORMAT_MASK)

/**
 * ipatch_sample_store_get_size:
 *
 * Macro for getting the sample size in frames of a sample store.  No lock is required
 * since size can only be set prior to the store being actively used.
 *
 * Returns: Sample store size in frames.
 */
#define ipatch_sample_store_get_size(store) ((store)->size)

/**
 * ipatch_sample_store_get_rate:
 *
 * Macro for getting the sample rate from a sample store.  No lock is required
 * since rate can only be set prior to the store being actively used.
 *
 * Returns: Sample rate in HZ.
 */
#define ipatch_sample_store_get_rate(store) ((store)->rate)

/**
 * ipatch_sample_store_get_size_bytes:
 *
 * Macro for getting the sample store data size in bytes.  No lock is required
 * since format and size can only be set prior to the store being actively used.
 *
 * Returns: Sample store size in bytes.
 */
#define ipatch_sample_store_get_size_bytes(store) \
  (ipatch_sample_format_size (ipatch_sample_store_get_format (store)) * (store)->size)

/* we reserve flags for format and 3 for expansion */
#define IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT \
  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + IPATCH_SAMPLE_FORMAT_BITCOUNT + 3)

GType ipatch_sample_store_get_type (void);

IpatchSampleStore *ipatch_sample_store_first (IpatchIter *iter);
IpatchSampleStore *ipatch_sample_store_next (IpatchIter *iter);

#endif
