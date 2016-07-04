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

#endif
