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
#ifndef __IPATCH_SAMPLE_STORE_VIRTUAL_H__
#define __IPATCH_SAMPLE_STORE_VIRTUAL_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleList.h>
#include <libinstpatch/IpatchSampleStore.h>
#include <libinstpatch/IpatchSample.h>

/* forward type declarations */

typedef struct _IpatchSampleStoreVirtual IpatchSampleStoreVirtual;
typedef struct _IpatchSampleStoreVirtualClass IpatchSampleStoreVirtualClass;

#define IPATCH_TYPE_SAMPLE_STORE_VIRTUAL \
  (ipatch_sample_store_virtual_get_type ())
#define IPATCH_SAMPLE_STORE_VIRTUAL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_VIRTUAL, \
  IpatchSampleStoreVirtual))
#define IPATCH_SAMPLE_STORE_VIRTUAL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_VIRTUAL, \
  IpatchSampleStoreVirtualClass))
#define IPATCH_IS_SAMPLE_STORE_VIRTUAL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_VIRTUAL))
#define IPATCH_IS_SAMPLE_STORE_VIRTUAL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_VIRTUAL))

/* Virtual sample store instance */
struct _IpatchSampleStoreVirtual
{
  IpatchSampleStore parent_instance;

  /*< private >*/

  IpatchSampleList *lists[2];	/* Edit lists, one per channel (max = stereo currently) */
  int access_speed;             /* Cached access speed value (0 if not yet calculated) */
};

/* Virtual sample store class */
struct _IpatchSampleStoreVirtualClass
{
  IpatchSampleStoreClass parent_class;
};


GType ipatch_sample_store_virtual_get_type (void);
IpatchSample *ipatch_sample_store_virtual_new (void);
IpatchSampleList *
ipatch_sample_store_virtual_get_list (IpatchSampleStoreVirtual *store, guint chan);
void ipatch_sample_store_virtual_set_list (IpatchSampleStoreVirtual *store,
					   guint chan, IpatchSampleList *list);

#endif
