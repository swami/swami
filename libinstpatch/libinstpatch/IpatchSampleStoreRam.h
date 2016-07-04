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
#ifndef __IPATCH_SAMPLE_STORE_RAM_H__
#define __IPATCH_SAMPLE_STORE_RAM_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleStore.h>

/* forward type declarations */

typedef struct _IpatchSampleStoreRam IpatchSampleStoreRam;
typedef struct _IpatchSampleStoreRamClass IpatchSampleStoreRamClass;

#define IPATCH_TYPE_SAMPLE_STORE_RAM (ipatch_sample_store_ram_get_type ())
#define IPATCH_SAMPLE_STORE_RAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_RAM, \
  IpatchSampleStoreRam))
#define IPATCH_SAMPLE_STORE_RAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_RAM, \
  IpatchSampleStoreRamClass))
#define IPATCH_IS_SAMPLE_STORE_RAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_RAM))
#define IPATCH_IS_SAMPLE_STORE_RAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_RAM))

/* RAM sample store instance */
struct _IpatchSampleStoreRam
{
  IpatchSampleStore parent_instance;
  gpointer location;    /* Pointer to the sample data in memory */
};

/* RAM sample store class */
struct _IpatchSampleStoreRamClass
{
  IpatchSampleStoreClass parent_class;
};

/**
 * IpatchSampleStoreRamFlags:
 * @IPATCH_SAMPLE_STORE_RAM_ALLOCATED: Indicates if sample data was allocated
 *   and therefore should be freed when finalized.
 *
 * Flags crammed into #IpatchItem flags field.
 */
typedef enum
{
  IPATCH_SAMPLE_STORE_RAM_ALLOCATED = 1 << IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT
} IpatchSampleStoreRamFlags;

/**
 * IPATCH_SAMPLE_STORE_RAM_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve 1 bits for defined flags above and 3 bits for future expansion */
#define IPATCH_SAMPLE_STORE_RAM_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT + 4)

GType ipatch_sample_store_ram_get_type (void);
IpatchSample *ipatch_sample_store_ram_new (gpointer location, gboolean free_data);
IpatchSample *ipatch_sample_store_ram_get_blank (void);

#endif
