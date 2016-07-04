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
#ifndef __IPATCH_SLI_FILE_H__
#define __IPATCH_SLI_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchRiff.h>

/* forward type declarations */

typedef struct _IpatchSLIFile IpatchSLIFile;
typedef struct _IpatchSLIFileClass IpatchSLIFileClass;

#define IPATCH_TYPE_SLI_FILE   (ipatch_sli_file_get_type ())
#define IPATCH_SLI_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_FILE, IpatchSLIFile))
#define IPATCH_SLI_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_FILE, \
  IpatchSLIFileClass))
#define IPATCH_IS_SLI_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_FILE))
#define IPATCH_IS_SLI_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_FILE))
#define IPATCH_SLI_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SLI_FILE, \
  IpatchSLIFileClass))

/**
 * IPATCH_SLI_NAME_SIZE: (skip)
 */
#define IPATCH_SLI_NAME_SIZE  24  /* name string size (Inst/Sample) */

/* Spectralis file object (derived from IpatchFile) */
struct _IpatchSLIFile
{
  IpatchFile parent_instance;
};

/* Spectralis file class (derived from IpatchFile) */
struct _IpatchSLIFileClass
{
  IpatchFileClass parent_class;
};

GType ipatch_sli_file_get_type (void);
IpatchSLIFile *ipatch_sli_file_new (void);

#endif
