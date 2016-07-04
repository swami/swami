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
#ifndef __IPATCH_SAMPLE_STORE_FILE_H__
#define __IPATCH_SAMPLE_STORE_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchSampleStore.h>

/* forward type declarations */

typedef struct _IpatchSampleStoreFile IpatchSampleStoreFile;
typedef struct _IpatchSampleStoreFileClass IpatchSampleStoreFileClass;

#define IPATCH_TYPE_SAMPLE_STORE_FILE (ipatch_sample_store_file_get_type ())
#define IPATCH_SAMPLE_STORE_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_STORE_FILE, \
  IpatchSampleStoreFile))
#define IPATCH_SAMPLE_STORE_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_STORE_FILE, \
  IpatchSampleStoreFileClass))
#define IPATCH_IS_SAMPLE_STORE_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_STORE_FILE))
#define IPATCH_IS_SAMPLE_STORE_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_STORE_FILE))

/* File sample store instance */
struct _IpatchSampleStoreFile
{
  IpatchSampleStore parent_instance;
  IpatchFile *file;     /* File object */
  guint location;       /* Position in file of the sample data */
};

/* File sample store class */
struct _IpatchSampleStoreFileClass
{
  IpatchSampleStoreClass parent_class;
};

/**
 * IPATCH_SAMPLE_STORE_FILE_UNUSED_FLAG_SHIFT: (skip)
 */
/* reserve 1 private flag (IpatchSampleStoreFile.c) */
#define IPATCH_SAMPLE_STORE_FILE_UNUSED_FLAG_SHIFT \
  (IPATCH_SAMPLE_STORE_UNUSED_FLAG_SHIFT + 1)

GType ipatch_sample_store_file_get_type (void);
IpatchSample *ipatch_sample_store_file_new (IpatchFile *file, guint location);

#endif
