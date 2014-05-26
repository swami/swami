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
#ifndef __IPATCH_SF2_WRITER_H__
#define __IPATCH_SF2_WRITER_H__

#include <glib.h>
#include <libinstpatch/IpatchSF2File.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchSF2.h>
#include <libinstpatch/IpatchSF2Mod.h>
#include <libinstpatch/IpatchSF2Gen.h>
#include <libinstpatch/IpatchList.h>

typedef struct _IpatchSF2Writer IpatchSF2Writer; /* private structure */
typedef struct _IpatchSF2WriterClass IpatchSF2WriterClass;

#define IPATCH_TYPE_SF2_WRITER   (ipatch_sf2_writer_get_type ())
#define IPATCH_SF2_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_WRITER, \
   IpatchSF2Writer))
#define IPATCH_SF2_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_WRITER, \
   IpatchSF2WriterClass))
#define IPATCH_IS_SF2_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_WRITER))
#define IPATCH_IS_SF2_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_WRITER))

/* SF2 writer object */
struct _IpatchSF2Writer
{
  IpatchRiff parent_instance; /* derived from IpatchRiff */
  IpatchSF2 *orig_sf;		/* original SF2 object */
  IpatchSF2 *sf;		/* duplicated SF2 object to save */
  gboolean migrate_samples; /* set to TRUE to migrate samples to new file */
  GHashTable *inst_hash;	/* instrument => index hash */
  GHashTable *sample_hash;	/* sample => SampleHashValue hash */
  IpatchList *store_list;       /* list of stores, only set if ipatch_sf2_writer_create_stores() was called */
};

/* SF2 writer class */
struct _IpatchSF2WriterClass
{
  IpatchRiffClass parent_class;
};

GType ipatch_sf2_writer_get_type (void);
IpatchSF2Writer *ipatch_sf2_writer_new (IpatchFileHandle *handle, IpatchSF2 *sfont);
void ipatch_sf2_writer_set_patch (IpatchSF2Writer *writer, IpatchSF2 *sfont);
void ipatch_sf2_writer_set_file_handle (IpatchSF2Writer *writer,
                                        IpatchFileHandle *handle);
gboolean ipatch_sf2_writer_save (IpatchSF2Writer *writer, GError **err);
IpatchList *ipatch_sf2_writer_create_stores (IpatchSF2Writer *writer);

void ipatch_sf2_write_phdr (IpatchFileHandle *handle, const IpatchSF2Phdr *phdr);
void ipatch_sf2_write_ihdr (IpatchFileHandle *handle, const IpatchSF2Ihdr *ihdr);
void ipatch_sf2_write_shdr (IpatchFileHandle *handle, const IpatchSF2Shdr *shdr);
void ipatch_sf2_write_bag (IpatchFileHandle *handle, const IpatchSF2Bag *bag);
void ipatch_sf2_write_mod (IpatchFileHandle *handle, const IpatchSF2Mod *mod);
void ipatch_sf2_write_gen (IpatchFileHandle *handle, int genid,
			   const IpatchSF2GenAmount *amount);

#endif
