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
#ifndef __IPATCH_SAMPLE_DATA_H__
#define __IPATCH_SAMPLE_DATA_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

/* forward type declarations */

typedef struct _IpatchSampleData IpatchSampleData;
typedef struct _IpatchSampleDataClass IpatchSampleDataClass;

#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSampleStore.h>
#include <libinstpatch/IpatchFile.h>

#define IPATCH_TYPE_SAMPLE_DATA  (ipatch_sample_data_get_type ())
#define IPATCH_SAMPLE_DATA(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SAMPLE_DATA, IpatchSampleData))
#define IPATCH_SAMPLE_DATA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SAMPLE_DATA, IpatchSampleDataClass))
#define IPATCH_IS_SAMPLE_DATA(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SAMPLE_DATA))
#define IPATCH_IS_SAMPLE_DATA_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SAMPLE_DATA))
#define IPATCH_SAMPLE_DATA_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SAMPLE_DATA, IpatchSampleDataClass))

/* Sample data instance */
struct _IpatchSampleData
{
  IpatchItem parent_instance;
  GSList *samples;              /* list of IpatchSampleStore (1st is native sample) */
  int usecount;                 /* Count of users of this sample data object */
};

struct _IpatchSampleDataClass
{
  IpatchItemClass parent_class;
};

/**
 * IPATCH_SAMPLE_DATA_UNUSED_FLAG_SHIFT: (skip)
 */
/* Reserve 3 item flags for expansion */
#define IPATCH_SAMPLE_DATA_UNUSED_FLAG_SHIFT \
  (IPATCH_CONTAINER_UNUSED_FLAG_SHIFT + 3)

/**
 * IpatchSampleDataMigrateFlags:
 * @IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED: Remove unused #IpatchSampleStore objects referencing newfile.
 * @IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE: Migrate all #IpatchSampleData objects to newfile which have a #IpatchSampleStore
 *   with the same format as the native format therein, default is to only migrate those with their native samples in
 *   oldfile or swap storage.
 * @IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP: Leave native samples in #IpatchSampleData objects in swap, even if present in
 *   newfile, default is to migrate samples out of swap in preference for newfile.
 * @IPATCH_SAMPLE_DATA_MIGRATE_REPLACE: Replace oldfile with newfile (has no effect if newfile is %NULL)
 *
 * Since: 1.1.0
 */
typedef enum
{
  IPATCH_SAMPLE_DATA_MIGRATE_REMOVE_NEW_IF_UNUSED       = 1 << 0,
  IPATCH_SAMPLE_DATA_MIGRATE_TO_NEWFILE                 = 1 << 1,
  IPATCH_SAMPLE_DATA_MIGRATE_LEAVE_IN_SWAP              = 1 << 2,
  IPATCH_SAMPLE_DATA_MIGRATE_REPLACE                    = 1 << 3
} IpatchSampleDataMigrateFlags;

IpatchList *ipatch_get_sample_data_list (void);
gboolean ipatch_migrate_file_sample_data (IpatchFile *oldfile, IpatchFile *newfile, const char *filename,
                                          guint flags, GError **err);

GType ipatch_sample_data_get_type (void);
IpatchSampleData *ipatch_sample_data_new (void);

void ipatch_sample_data_used (IpatchSampleData *sampledata);
void ipatch_sample_data_unused (IpatchSampleData *sampledata);

void ipatch_sample_data_add (IpatchSampleData *sampledata, IpatchSampleStore *store);
void ipatch_sample_data_remove (IpatchSampleData *sampledata, IpatchSampleStore *store);
void ipatch_sample_data_replace_native_sample (IpatchSampleData *sampledata,
                                               IpatchSampleStore *store);

IpatchList *ipatch_sample_data_get_samples (IpatchSampleData *sampledata);
guint ipatch_sample_data_get_size (IpatchSampleData *sampledata);
IpatchSampleStore *ipatch_sample_data_get_native_sample (IpatchSampleData *sampledata);
int ipatch_sample_data_get_native_format (IpatchSampleData *sampledata);

gboolean ipatch_sample_data_open_native_sample (IpatchSampleData *sampledata,
                                                IpatchSampleHandle *handle, char mode,
                                                int format, guint32 channel_map,
                                                GError **err);

IpatchSampleStore *ipatch_sample_data_get_cache_sample (IpatchSampleData *sampledata,
                                                        int format, guint32 channel_map,
                                                        GError **err);
IpatchSampleStore *ipatch_sample_data_lookup_cache_sample (IpatchSampleData *sampledata,
                                                           int format, guint32 channel_map);
gboolean ipatch_sample_data_open_cache_sample (IpatchSampleData *sampledata,
                                               IpatchSampleHandle *handle,
                                               int format, guint32 channel_map,
                                               GError **err);
guint64 ipatch_sample_cache_get_unused_size (void);
void ipatch_sample_cache_clean (guint64 max_unused_size, guint max_unused_age);

IpatchSampleData *ipatch_sample_data_get_blank (void);

#endif

