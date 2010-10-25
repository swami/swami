/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
/**
 * SECTION: IpatchSampleStoreSwap
 * @short_description: Sample storage object for audio in a temporary swap file
 * @see_also: 
 * @stability: Stable
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
  IpatchSampleStoreFile parent_instance;
};

/* Swap file sample store class (derived from FILE sample store) */
struct _IpatchSampleStoreSwapClass
{
  IpatchSampleStoreFileClass parent_class;
};

/* we reserve 1 private flag */
#define IPATCH_SAMPLE_STORE_SWAP_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_FILE_UNUSED_FLAG_SHIFT + 1)

GType ipatch_sample_store_swap_get_type (void);
IpatchSample *ipatch_sample_store_swap_new (void);
int ipatch_sample_store_swap_get_unused_size (void);

#endif
