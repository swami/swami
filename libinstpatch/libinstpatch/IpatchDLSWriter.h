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
#ifndef __IPATCH_DLS_WRITER_H__
#define __IPATCH_DLS_WRITER_H__

#include <glib.h>
#include <libinstpatch/IpatchDLSFile.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchDLS2.h>
#include <libinstpatch/IpatchList.h>

typedef struct _IpatchDLSWriter IpatchDLSWriter; /* private structure */
typedef struct _IpatchDLSWriterClass IpatchDLSWriterClass;

#define IPATCH_TYPE_DLS_WRITER   (ipatch_dls_writer_get_type ())
#define IPATCH_DLS_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS_WRITER, \
   IpatchDLSWriter))
#define IPATCH_DLS_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS_WRITER, \
   IpatchDLSWriterClass))
#define IPATCH_IS_DLS_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS_WRITER))
#define IPATCH_IS_DLS_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS_WRITER))

/* DLS writer object */
struct _IpatchDLSWriter
{
  IpatchRiff parent_instance;   /* derived from IpatchRiff */
  IpatchDLS2 *orig_dls;		/* original DLS object */
  IpatchDLS2 *dls;		/* duplicated DLS object to save */
  gboolean is_gig;	        /* set to TRUE if saving a GigaSampler file */
  GHashTable *sample_hash;      /* IpatchDLS2Sample -> sample_array index + 1 */
  guint32 *sample_ofstbl;       /* sample index -> file offset table */
  guint32 *sample_postbl;       /* sample index -> sample file position */
  guint sample_count;		/* count of samples */
  guint32 ptbl_pos;             /* pool table position in file - for later fixup */
  IpatchList *store_list;       /* list of stores, only set if ipatch_dls_writer_create_stores() was called */
};

/* DLS writer class */
struct _IpatchDLSWriterClass
{
  IpatchRiffClass parent_class;
};

GType ipatch_dls_writer_get_type (void);
IpatchDLSWriter *ipatch_dls_writer_new (IpatchFileHandle *handle, IpatchDLS2 *dls);
void ipatch_dls_writer_set_patch (IpatchDLSWriter *writer, IpatchDLS2 *dls);
void ipatch_dls_writer_set_file_handle (IpatchDLSWriter *writer, IpatchFileHandle *handle);
gboolean ipatch_dls_writer_save (IpatchDLSWriter *writer, GError **err);
IpatchList *ipatch_dls_writer_create_stores (IpatchDLSWriter *writer);

#endif
