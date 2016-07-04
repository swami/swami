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
#ifndef __IPATCH_GIG_FILE_H__
#define __IPATCH_GIG_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchDLSFile.h>
#include <libinstpatch/IpatchRiff.h>

typedef struct _IpatchGigFile IpatchGigFile;
typedef struct _IpatchGigFileClass IpatchGigFileClass;

#define IPATCH_TYPE_GIG_FILE   (ipatch_gig_file_get_type ())
#define IPATCH_GIG_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_FILE, IpatchGigFile))
#define IPATCH_GIG_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_FILE, IpatchGigFileClass))
#define IPATCH_IS_GIG_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_FILE))
#define IPATCH_IS_GIG_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_FILE))
#define IPATCH_GIG_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_FILE, \
   IpatchGigFileClass))

/* GigaSampler file object (derived from IpatchDLSFile) */
struct _IpatchGigFile
{
  IpatchDLSFile parent_instance;
};

/* GigaSampler file class (derived from IpatchDLSFile) */
struct _IpatchGigFileClass
{
  IpatchDLSFileClass parent_class;
};

GType ipatch_gig_file_get_type (void);
IpatchGigFile *ipatch_gig_file_new (void);

#endif
