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
#ifndef __IPATCH_SF2_READER_H__
#define __IPATCH_SF2_READER_H__

#include <glib.h>
#include <libinstpatch/IpatchSF2File.h>
#include <libinstpatch/IpatchSF2Gen.h>
#include <libinstpatch/IpatchSF2Mod.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchSF2.h>

typedef struct _IpatchSF2Reader IpatchSF2Reader; /* private structure */
typedef struct _IpatchSF2ReaderClass IpatchSF2ReaderClass;

#define IPATCH_TYPE_SF2_READER   (ipatch_sf2_reader_get_type ())
#define IPATCH_SF2_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_READER, \
   IpatchSF2Reader))
#define IPATCH_SF2_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_READER, \
   IpatchSF2ReaderClass))
#define IPATCH_IS_SF2_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_READER))
#define IPATCH_IS_SF2_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_READER))

/* SoundFont parser object */
struct _IpatchSF2Reader
{
  IpatchRiff parent_instance; /* derived from IpatchRiff */
  IpatchSF2 *sf;	      /* SoundFont object to load file into */

  /*< private >*/
  guint16 *pbag_table;
  guint pbag_count;
  guint16 *ibag_table;
  guint ibag_count;
  IpatchSF2Inst **inst_table;
  guint inst_count;
  IpatchSF2Sample **sample_table;
  guint sample_count;
};

/* RIFF parser class */
struct _IpatchSF2ReaderClass
{
  IpatchRiffClass parent_class;
};

GType ipatch_sf2_reader_get_type (void);
IpatchSF2Reader *ipatch_sf2_reader_new (IpatchFileHandle *handle);
void ipatch_sf2_reader_set_file_handle (IpatchSF2Reader *reader,
                                        IpatchFileHandle *handle);
IpatchSF2 *ipatch_sf2_reader_load (IpatchSF2Reader *reader, GError **err);

#endif
