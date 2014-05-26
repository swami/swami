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
#ifndef __IPATCH_SAMPLE_STORE_ROM_H__
#define __IPATCH_SAMPLE_STORE_ROM_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSampleStore.h>

typedef struct _IpatchSampleStoreRom IpatchSampleStoreRom;
typedef struct _IpatchSampleStoreRomClass IpatchSampleStoreRomClass;

#define IPATCH_TYPE_SAMPLE_STORE_ROM (ipatch_sample_store_rom_get_type ())
#define IPATCH_SAMPLE_STORE_ROM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_ROM, \
  IpatchSampleStoreRom))
#define IPATCH_SAMPLE_STORE_ROM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_ROM, \
  IpatchSampleStoreRomClass))
#define IPATCH_IS_SAMPLE_STORE_ROM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_ROM))
#define IPATCH_IS_SAMPLE_STORE_ROM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_ROM))

/* ROM sample store instance */
struct _IpatchSampleStoreRom
{
  IpatchSampleStore parent_instance;
  guint location;
};

/* ROM sample store class */
struct _IpatchSampleStoreRomClass
{
  IpatchSampleStoreClass parent_class;
};

GType ipatch_sample_store_rom_get_type (void);
IpatchSample *ipatch_sample_store_rom_new (guint location);

#endif
