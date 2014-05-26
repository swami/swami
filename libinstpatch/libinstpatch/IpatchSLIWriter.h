/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Author of this file: (C) 2012 BALATON Zoltan <balaton@eik.bme.hu>
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
#ifndef __IPATCH_SLI_WRITER_H__
#define __IPATCH_SLI_WRITER_H__

#include <glib.h>
#include <libinstpatch/IpatchSLIFile.h>
#include <libinstpatch/IpatchSLI.h>
#include <libinstpatch/IpatchList.h>

typedef struct _IpatchSLIWriter IpatchSLIWriter; /* private structure */
typedef struct _IpatchSLIWriterClass IpatchSLIWriterClass;

#define IPATCH_TYPE_SLI_WRITER   (ipatch_sli_writer_get_type ())
#define IPATCH_SLI_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_WRITER, \
   IpatchSLIWriter))
#define IPATCH_SLI_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_WRITER, \
   IpatchSLIWriterClass))
#define IPATCH_IS_SLI_WRITER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_WRITER))
#define IPATCH_IS_SLI_WRITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_WRITER))

/* Spectralis SLI/SLC writer object
 * Spectralis files do not sufficiently follow RIFF format
 * so this object is not derived from IpatchRiff class */
struct _IpatchSLIWriter
{
  GObject parent_instance;      /* derived from GObject */
  IpatchFileHandle *handle;     /* file object being written */
  IpatchSLI *orig_sli;	        /* original SLI object */
  IpatchSLI *sli;	        /* duplicated SLI object to save */
  GHashTable *sample_hash;	/* sample => SampleHashValue hash */
  IpatchList *store_list;       /* list of stores, only set if ipatch_sli_writer_get_stores() was called */
};

/* Spectralis SLI/SLC writer class */
struct _IpatchSLIWriterClass
{
  GObjectClass parent_class;
};

GType ipatch_sli_writer_get_type (void);
IpatchSLIWriter *ipatch_sli_writer_new (IpatchFileHandle *handle, IpatchSLI *sli);
void ipatch_sli_writer_set_patch (IpatchSLIWriter *writer, IpatchSLI *sli);
void ipatch_sli_writer_set_file_handle (IpatchSLIWriter *writer, IpatchFileHandle *handle);
gboolean ipatch_sli_writer_save (IpatchSLIWriter *writer, GError **err);
IpatchList *ipatch_sli_writer_create_stores (IpatchSLIWriter *writer);

#endif
