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
#ifndef __IPATCH_DLS_FILE_H__
#define __IPATCH_DLS_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchRiff.h>

typedef struct _IpatchDLSFile IpatchDLSFile;
typedef struct _IpatchDLSFileClass IpatchDLSFileClass;

#define IPATCH_TYPE_DLS_FILE   (ipatch_dls_file_get_type ())
#define IPATCH_DLS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_DLS_FILE, IpatchDLSFile))
#define IPATCH_DLS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_DLS_FILE, \
   IpatchDLSFileClass))
#define IPATCH_IS_DLS_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_DLS_FILE))
#define IPATCH_IS_DLS_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_DLS_FILE))
#define IPATCH_DLS_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_DLS_FILE, \
   IpatchDLSFileClass))

/* INFO FOURCC ids (user friendly defines in IpatchDLS2Info.h) */
#define IPATCH_DLS_FOURCC_IARL  IPATCH_FOURCC ('I','A','R','L')
#define IPATCH_DLS_FOURCC_IART  IPATCH_FOURCC ('I','A','R','T')
#define IPATCH_DLS_FOURCC_ICMS  IPATCH_FOURCC ('I','C','M','S')
#define IPATCH_DLS_FOURCC_ICMT  IPATCH_FOURCC ('I','C','M','T')
#define IPATCH_DLS_FOURCC_ICOP  IPATCH_FOURCC ('I','C','O','P')
#define IPATCH_DLS_FOURCC_ICRD  IPATCH_FOURCC ('I','C','R','D')
#define IPATCH_DLS_FOURCC_IENG  IPATCH_FOURCC ('I','E','N','G')
#define IPATCH_DLS_FOURCC_IGNR  IPATCH_FOURCC ('I','G','N','R')
#define IPATCH_DLS_FOURCC_IKEY  IPATCH_FOURCC ('I','K','E','Y')
#define IPATCH_DLS_FOURCC_IMED  IPATCH_FOURCC ('I','M','E','D')
#define IPATCH_DLS_FOURCC_INAM  IPATCH_FOURCC ('I','N','A','M')
#define IPATCH_DLS_FOURCC_IPRD  IPATCH_FOURCC ('I','P','R','D')
#define IPATCH_DLS_FOURCC_ISBJ  IPATCH_FOURCC ('I','S','B','J')
#define IPATCH_DLS_FOURCC_ISFT  IPATCH_FOURCC ('I','S','F','T')
#define IPATCH_DLS_FOURCC_ISRC  IPATCH_FOURCC ('I','S','R','C')
#define IPATCH_DLS_FOURCC_ISRF  IPATCH_FOURCC ('I','S','R','F')
#define IPATCH_DLS_FOURCC_ITCH  IPATCH_FOURCC ('I','T','C','H')

/**
 * IPATCH_DLS_DLID_SIZE: (skip)
 */
#define IPATCH_DLS_DLID_SIZE        16 /* DLID unique ID chunk size */

/* DLS file object (derived from IpatchFile) */
struct _IpatchDLSFile
{
  IpatchFile parent_instance;
};

/* DLS file class (derived from IpatchFile) */
struct _IpatchDLSFileClass
{
  IpatchFileClass parent_class;
};

GType ipatch_dls_file_get_type (void);
IpatchDLSFile *ipatch_dls_file_new (void);

#endif
