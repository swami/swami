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
 * SECTION: IpatchDLSReader
 * @short_description: DLS version 2 file reader
 * @see_also: #IpatchDLS
 * @stability: Stable
 *
 * Parses a DLS file into an object tree (#IpatchDLS).
 */
#ifndef __IPATCH_DLS_READER_H__
#define __IPATCH_DLS_READER_H__

#include <glib.h>
#include <libinstpatch/IpatchDLSFile.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchDLS2.h>
#include <libinstpatch/IpatchDLS2Info.h>
#include <libinstpatch/IpatchDLS2Region.h>
#include <libinstpatch/IpatchGigInst.h>
#include <libinstpatch/IpatchGigRegion.h>
#include <libinstpatch/IpatchGigDimension.h>

typedef struct _IpatchDLSReader IpatchDLSReader; /* private structure */
typedef struct _IpatchDLSReaderClass IpatchDLSReaderClass;

#define IPATCH_TYPE_DLS_READER   (ipatch_dls_reader_get_type ())
#define IPATCH_DLS_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS_READER, \
   IpatchDLSReader))
#define IPATCH_DLS_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS_READER, \
   IpatchDLSReaderClass))
#define IPATCH_IS_DLS_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS_READER))
#define IPATCH_IS_DLS_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS_READER))

/* error domain for DLS Reader */
#define IPATCH_DLS_READER_ERROR  ipatch_dls_reader_error_quark()

typedef enum
{
  /* this error is returned if a file originally thought to be a plain DLS
     file turns out to be a GigaSampler file, in which case loading should
     be restarted in GigaSampler mode */
  IPATCH_DLS_READER_ERROR_GIG
} IpatchDLSReaderError;

/* DLS reader object */
struct _IpatchDLSReader
{
  IpatchRiff parent_instance; /* derived from IpatchRiff */
  IpatchDLS2 *dls;   /* DLS or GigaSampler object to load file into */
  gboolean is_gig;	      /* set if dls is a GigaSampler object */
  gboolean needs_fixup;		/* set if regions in dls need fixup */
  GHashTable *wave_hash;	/* wave chunk file offset -> sample hash */
  guint32 *pool_table; /* wave pool table (index -> wave chunk file offset) */
  guint pool_table_size;     /* size of pool table (in cue entries) */
};

/* DLS reader class */
struct _IpatchDLSReaderClass
{
  IpatchRiffClass parent_class;
};

GType ipatch_dls_reader_get_type (void);
IpatchDLSReader *ipatch_dls_reader_new (IpatchFileHandle *handle);
IpatchDLS2 *ipatch_dls_reader_load (IpatchDLSReader *reader, GError **err);
gboolean ipatch_dls_reader_start (IpatchDLSReader *reader, GError **err);
void ipatch_dls_reader_set_pool_table (IpatchDLSReader *reader,
				       const guint32 pool_table[], guint size);
gboolean ipatch_dls_reader_fixup (IpatchDLSReader *reader, GError **err);

gboolean ipatch_dls_reader_load_level_0 (IpatchDLSReader *reader,
					 GError **err);
gboolean ipatch_dls_reader_load_inst_list (IpatchDLSReader *reader,
					   GError **err);
gboolean ipatch_dls_reader_load_region_list (IpatchDLSReader *reader,
				      IpatchDLS2Inst *inst, GError **err);
gboolean ipatch_gig_reader_load_region_list (IpatchDLSReader *reader,
					     IpatchGigInst *giginst,
					     GError **err);
gboolean ipatch_dls_reader_load_art_list (IpatchDLSReader *reader,
					  GSList **conn_list, GError **err);
gboolean ipatch_dls_reader_load_wave_pool (IpatchDLSReader *reader,
					   GError **err);
gboolean ipatch_gig_reader_load_sub_regions (IpatchDLSReader *reader,
					     IpatchGigRegion *region,
					     GError **err);
gboolean ipatch_gig_reader_load_inst_lart (IpatchDLSReader *reader,
					   IpatchGigInst *inst, GError **err);

gboolean ipatch_dls_load_info (IpatchRiff *riff, IpatchDLS2Info **info,
			       GError **err);
gboolean ipatch_dls_load_region_header (IpatchRiff *riff,
					IpatchDLS2Region *region, GError **err);
gboolean ipatch_gig_load_region_header (IpatchRiff *riff,
				        IpatchGigRegion *region, GError **err);
gboolean ipatch_dls_load_wave_link (IpatchRiff *riff, IpatchDLS2Region *region,
				    GError **err);
gboolean ipatch_dls_load_sample_info (IpatchRiff *riff,
                                      IpatchDLS2SampleInfo *info,
				      GError **err);
gboolean ipatch_dls_load_connection (IpatchRiff *riff,
				     GSList **conn_list,
				     GError **err);
gboolean ipatch_dls_load_sample_format (IpatchRiff *riff,
					IpatchDLS2Sample *sample,
					int *bitwidth, int *channels,
					GError **err);
guint32 *ipatch_dls_load_pool_table (IpatchRiff *riff, guint *size, GError **err);
gboolean ipatch_dls_load_dlid (IpatchRiff *riff, guint8 *dlid, GError **err);

gboolean ipatch_gig_load_sample_info (IpatchRiff *riff,
				      IpatchDLS2SampleInfo *info,
				      GError **err);
gboolean ipatch_gig_load_dimension_info (IpatchRiff *riff,
					 IpatchGigRegion *region,
					 GError **err);
gboolean ipatch_gig_load_dimension_names (IpatchRiff *riff,
					  IpatchGigRegion *region,
					  GError **err);
gboolean ipatch_gig_load_group_names (IpatchRiff *riff,
				      GSList **name_list, GError **err);

#endif
