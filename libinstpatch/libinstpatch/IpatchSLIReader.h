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
#ifndef __IPATCH_SLI_READER_H__
#define __IPATCH_SLI_READER_H__

#include <glib.h>
#include <libinstpatch/IpatchSLIFile.h>
#include <libinstpatch/IpatchSLI.h>

typedef struct _IpatchSLIReader IpatchSLIReader; /* private structure */
typedef struct _IpatchSLIReaderClass IpatchSLIReaderClass;

#define IPATCH_TYPE_SLI_READER   (ipatch_sli_reader_get_type ())
#define IPATCH_SLI_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_READER, \
   IpatchSLIReader))
#define IPATCH_SLI_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_READER, \
   IpatchSLIReaderClass))
#define IPATCH_IS_SLI_READER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_READER))
#define IPATCH_IS_SLI_READER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_READER))

/* Spectralis SLI/SLC file parser object
 * Spectralis files do not sufficiently follow RIFF format
 * so this object is not derived from IpatchRiff class */
struct _IpatchSLIReader
{
  GObject parent_instance;    /* derived from GObject */
  IpatchFileHandle *handle;   /* file object being parsed */
  IpatchSLI *sli;	      /* Spectralis object to load file into */
};

/* Spectralis SLI/SLC file parser class */
struct _IpatchSLIReaderClass
{
  GObjectClass parent_class;
};

GType ipatch_sli_reader_get_type (void);
IpatchSLIReader *ipatch_sli_reader_new (IpatchFileHandle *handle);
void ipatch_sli_reader_set_file_handle (IpatchSLIReader *reader,
                                        IpatchFileHandle *handle);
IpatchSLI *ipatch_sli_reader_load (IpatchSLIReader *reader, GError **err);

#endif
