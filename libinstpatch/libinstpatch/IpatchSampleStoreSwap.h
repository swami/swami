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
#ifndef __IPATCH_SAMPLE_STORE_SWAP_H__
#define __IPATCH_SAMPLE_STORE_SWAP_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleStoreFile.h>


typedef struct _IpatchSampleStoreSwap IpatchSampleStoreSwap;
typedef struct _IpatchSampleStoreSwapClass IpatchSampleStoreSwapClass;

#define IPATCH_TYPE_SAMPLE_STORE_SWAP \
  (ipatch_sample_store_swap_get_type ())
#define IPATCH_SAMPLE_STORE_SWAP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_SWAP, \
  IpatchSampleStoreSwap))
#define IPATCH_SAMPLE_STORE_SWAP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_SWAP, \
  IpatchSampleStoreSwapClass))
#define IPATCH_IS_SAMPLE_STORE_SWAP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_SWAP))
#define IPATCH_IS_SAMPLE_STORE_SWAP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_SWAP))

/* Swap file sample store instance (derived from FILE sample store) */
struct _IpatchSampleStoreSwap
{
  IpatchSampleStore parent_instance;
  gpointer ram_location;        // Pointer to memory location or NULL if stored in file
  guint location;               // Position in file of the sample data (if ram_location is NULL)
};

/* Swap file sample store class (derived from FILE sample store) */
struct _IpatchSampleStoreSwapClass
{
  IpatchSampleStoreClass parent_class;
};

/**
 * IPATCH_SAMPLE_STORE_SWAP_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve 1 private flag */
#define IPATCH_SAMPLE_STORE_SWAP_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT + 1)

GType ipatch_sample_store_swap_get_type (void);
void ipatch_set_sample_store_swap_file_name (const char *filename);
char *ipatch_get_sample_store_swap_file_name (void);
IpatchSample *ipatch_sample_store_swap_new (void);
void ipatch_sample_store_swap_close (void);
int ipatch_get_sample_store_swap_unused_size (void);
void ipatch_set_sample_store_swap_max_memory (int size);
int ipatch_get_sample_store_swap_max_memory (void);
gboolean ipatch_compact_sample_store_swap (GError **err);

#endif

