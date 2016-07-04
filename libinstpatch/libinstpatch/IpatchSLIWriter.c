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
/**
 * SECTION: IpatchSLIWriter
 * @short_description: Spectralis SLI/SLC instrument file writer
 * @see_also: #IpatchSLI
 *
 * Writes an SLI instrument object tree (#IpatchSLI) to an SLI or SLC file.
 */
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <libinstpatch/IpatchSLIWriter.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchSF2GenItem.h>
#include <libinstpatch/IpatchSampleStoreFile.h>
#include "IpatchSLIFile_priv.h"
#include "compat.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* NOTICE: A duplicate SLI object is used for saving. This
 * solves all multi-thread issues and allows one to continue editing
 * even while a file is being saved. It also means that the
 * duplicate SLI object can be accessed directly without
 * locking. Sample data objects are not duplicated though, so they
 * still need to be dealt with properly.
 */

/* Hash value used for sample_hash */
typedef struct
{
  guint index;          /* sample index */
  guint position;       /* position in file */
  guint offset;         /* offset in chunk */
  guint length;         /* data length in bytes */
  guint channels;       /* channel count */
} SampleHashValue;

#define FORMAT_16BIT  IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_SIGNED | IPATCH_SAMPLE_LENDIAN

static void ipatch_sli_writer_finalize (GObject *object);

static GQuark ipatch_sli_writer_error_quark (void);
static SampleHashValue *ipatch_sli_writer_sample_hash_value_new (void);
static void ipatch_sli_writer_sample_hash_value_destroy (gpointer value);
static gint ipatch_sli_writer_inst_find_func (gconstpointer a, gconstpointer b);

static GSList *ipatch_sli_writer_find_groups (IpatchSLI *sli);
static gboolean ipatch_sli_writer_write_group (IpatchSLIWriter *writer,
                                               GPtrArray *ig, GError **err);
static gboolean ipatch_sli_writer_write_sifi (IpatchFileHandle *handle,
                                              guint ignum, GError **err);
static void ipatch_sli_writer_write_siig (IpatchFileHandle *handle,
                                          IpatchSLISiIg *siig);
static void ipatch_sli_writer_write_inst_header (IpatchFileHandle *handle,
                                                 IpatchSLIInstHeader *ihdr);
static void ipatch_sli_writer_write_zone_header (IpatchFileHandle *handle,
                                                 IpatchSLIZone *zone,
                                                 guint sample_idx);
static void ipatch_sli_writer_write_sample_header (IpatchFileHandle *handle,
                                                   SampleHashValue *sample_info,
                                                   IpatchSLISample *sample);
static void ipatch_sli_writer_write_sidp (IpatchFileHandle *handle,
                                          IpatchSLISiDp *sidp);
static gboolean ipatch_sli_writer_write_sample_data (IpatchFileHandle *handle,
                                                     IpatchSLISample *sample,
                                                     GError **err);

G_DEFINE_TYPE (IpatchSLIWriter, ipatch_sli_writer, G_TYPE_OBJECT);

static void
ipatch_sli_writer_class_init (IpatchSLIWriterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_sli_writer_finalize;
}

static void
ipatch_sli_writer_init (IpatchSLIWriter *writer)
{
  writer->sli = NULL;
  writer->sample_hash = g_hash_table_new_full (NULL, NULL, NULL,
                                   ipatch_sli_writer_sample_hash_value_destroy);
}

static void
ipatch_sli_writer_finalize (GObject *object)
{
  IpatchSLIWriter *writer = IPATCH_SLI_WRITER (object);

  if (writer->handle)
    ipatch_file_close (writer->handle); /* -- unref file object */

  if (writer->orig_sli)
  {
    g_object_unref (writer->orig_sli);
    writer->orig_sli = NULL;
  }

  if (writer->sli)
  {
    g_object_unref (writer->sli);
    writer->sli = NULL;
  }

  if (writer->sample_hash)
  {
    g_hash_table_destroy (writer->sample_hash);
    writer->sample_hash = NULL;
  }

  if (writer->store_list)
  {
    g_object_unref (writer->store_list);
    writer->store_list = NULL;
  }

  if (G_OBJECT_CLASS (ipatch_sli_writer_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sli_writer_parent_class)->finalize (object);
}

static GQuark
ipatch_sli_writer_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("sli-writer-error-quark");

  return (q);
}

static SampleHashValue *
ipatch_sli_writer_sample_hash_value_new (void)
{
  return (g_slice_new0 (SampleHashValue));
}

static void
ipatch_sli_writer_sample_hash_value_destroy (gpointer value)
{
  g_slice_free (SampleHashValue, value);
}

/**
 * ipatch_sli_writer_new:
 * @handle: SLI file handle to save to or %NULL to set later, taken over by
 *   writer object and will be closed on finalize.
 * @sli: SLI object to save or %NULL to set later
 *
 * Create a new SLI file writer.
 *
 * Returns: The new SLI writer
 */
IpatchSLIWriter *
ipatch_sli_writer_new (IpatchFileHandle *handle, IpatchSLI *sli)
{
  IpatchSLIWriter *writer;

  g_return_val_if_fail (!handle || IPATCH_IS_SLI_FILE (handle->file), NULL);
  g_return_val_if_fail (!sli || IPATCH_IS_SLI (sli), NULL);

  writer = g_object_new (IPATCH_TYPE_SLI_WRITER, NULL);
  if (handle) ipatch_sli_writer_set_file_handle (writer, handle);
  if (sli) ipatch_sli_writer_set_patch (writer, sli);

  return (writer);
}

/**
 * ipatch_sli_writer_set_patch:
 * @writer: SLI writer object
 * @sli: SLI patch to save
 *
 * Set the SLI patch object to save with a SLI writer.
 */
void
ipatch_sli_writer_set_patch (IpatchSLIWriter *writer, IpatchSLI *sli)
{
  g_return_if_fail (IPATCH_IS_SLI_WRITER (writer));
  g_return_if_fail (IPATCH_IS_SLI (sli));

  if (writer->orig_sli) g_object_unref (writer->orig_sli);
  writer->orig_sli = g_object_ref (sli);
}

/**
 * ipatch_sli_writer_set_file_handle:
 * @writer: SLI writer object
 * @handle: (nullable): SLI file handle or %NULL to clear
 *
 * Set the SLI file handle of an SLI writer.
 */
void
ipatch_sli_writer_set_file_handle (IpatchSLIWriter *writer, IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_SLI_WRITER (writer));
  g_return_if_fail (handle && IPATCH_IS_SLI_FILE (handle->file));

  /* Close old handle, if any */
  if (writer->handle) ipatch_file_close (writer->handle);

  writer->handle = handle;
}

/**
 * ipatch_sli_writer_save:
 * @writer: SLI writer object
 * @err: Location to store error info or %NULL
 *
 * Write an SLI object to a file.
 *
 * Returns: %TRUE on success, %FALSE on error
 */
gboolean
ipatch_sli_writer_save (IpatchSLIWriter *writer, GError **err)
{
  GSList *ig, *igs = NULL;
  guint32 len;
  gboolean ret;

  g_return_val_if_fail (IPATCH_IS_SLI_WRITER (writer), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);
  g_return_val_if_fail (writer->handle != NULL, FALSE);
  g_return_val_if_fail (writer->orig_sli != NULL, FALSE);

  if (writer->sli) g_object_unref (writer->sli); /* shouldn't be set, but.. */

  /* ++ref new duplicate sli object */
  writer->sli = IPATCH_SLI (ipatch_item_duplicate (IPATCH_ITEM (writer->orig_sli)));
  g_return_val_if_fail (writer->sli != NULL, FALSE);

  /* build instrument groups and write main file header */
  igs = ipatch_sli_writer_find_groups (writer->sli); /* ++ ref igs */
  if (!igs)
  {
    g_set_error (err, ipatch_sli_writer_error_quark(),
                 IPATCH_RIFF_ERROR_INVALID_DATA,
                 "Could not determine groups for SLI");
    return (FALSE);
  }
  if (!ipatch_sli_writer_write_sifi (writer->handle, g_slist_length (igs), err))
  {
    ret = FALSE;
    goto end;
  }

  /* write groups */
  for (ig = igs; ig; ig = g_slist_next (ig))
  {
    if(!ipatch_sli_writer_write_group (writer, ig->data, err))
    {
      ret = FALSE;
      goto end;
    }
  }

  /* fixup cklen field in SiFi header */
  len = ipatch_file_get_position (writer->handle);
  if (!ipatch_file_seek (writer->handle, IPATCH_RIFF_FOURCC_SIZE, G_SEEK_SET, err) ||
      !ipatch_file_write_u32 (writer->handle, len, err))
  {
    ret = FALSE;
    goto end;
  }

  ret = TRUE;
  g_object_set (writer->orig_sli,
                "changed", FALSE, /* file and object are in sync */
                "saved", TRUE, /* has now been saved */
                NULL);
end:
  g_slist_free_full (igs, (GDestroyNotify)g_ptr_array_unref); /* -- unref igs */
  /* keep duplicated sli object for create_stores unless there was an error */
  if (!ret)
  {
    g_object_unref (writer->sli); /* -- unref duplicate sli object */
    writer->sli = NULL;
  }
  return ret;
}


/**
 * ipatch_sli_writer_create_stores:
 * @writer: SLI writer object
 *
 * Create sample stores and add them to applicable #IpatchSampleData objects and return object list.
 * This function can be called multiple times, additional calls will return the same list.
 *
 * Returns: (transfer full): List of sample stores which the caller owns a reference to or %NULL
 */
IpatchList *
ipatch_sli_writer_create_stores (IpatchSLIWriter *writer)
{
  SampleHashValue *hash_value;
  IpatchSample *newstore;
  IpatchSLISample *sample;
  IpatchIter iter;
  IpatchList *list;
  int rate, format;
  guint size;

  g_return_val_if_fail (writer->sli != NULL, NULL);

  /* Return existing store list (if this function has been called before) */
  if (writer->store_list)
    return (g_object_ref (writer->store_list));  /* ++ ref for caller */

  list = ipatch_list_new ();  /* ++ ref list */

  ipatch_container_init_iter(IPATCH_CONTAINER (writer->sli), &iter,
                             IPATCH_TYPE_SLI_SAMPLE);

  /* traverse samples */
  for (sample = ipatch_sli_sample_first (&iter); sample;
       sample = ipatch_sli_sample_next (&iter))
  {
    hash_value = g_hash_table_lookup (writer->sample_hash, sample);

    /* hash_value should never be NULL, but.. */
    if (!hash_value) continue;

    g_object_get (sample,
                  "sample-format", &format,
                  "sample-size", &size,
                  "sample-rate", &rate,
                  NULL);

    /* ++ ref newstore */
    newstore = ipatch_sample_store_file_new (writer->handle->file,
                                             hash_value->position);
    format &= IPATCH_SAMPLE_CHANNEL_MASK;
    format |= FORMAT_16BIT;
    g_object_set (newstore,
                  "sample-format", format,
                  "sample-size", size,
                  "sample-rate", rate,
                  NULL);

    ipatch_sample_data_add (sample->sample_data, (IpatchSampleStore *)newstore);
    /* !! list takes over reference */
    list->items = g_list_prepend (list->items, newstore);
  }

  writer->store_list = g_object_ref (list);  /* ++ ref for writer object */
  return (list);  /* !! caller takes over reference */
}

static gint
ipatch_sli_writer_inst_find_func (gconstpointer a, gconstpointer b)
{
  const GPtrArray *arr = a;
  guint i;

  for (i = 0; i < arr->len; i++)
    if (g_ptr_array_index(arr, i) == b)
      return 0;

  return -1;
}

static GSList *
ipatch_sli_writer_find_groups (IpatchSLI *sli)
{
  IpatchIter inst_iter, iz_iter;
  IpatchItem *item;
  GSList *ig, *igs = NULL;
  GPtrArray *insts;
  IpatchList *inst_zones, *zlist;
  IpatchSLIZone *iz, *z;

  ipatch_container_init_iter ((IpatchContainer *)sli, &inst_iter,
                              IPATCH_TYPE_SLI_INST);
  for (item = ipatch_iter_first (&inst_iter);
       item;
       item = ipatch_iter_next (&inst_iter))
  {
    ig = g_slist_find_custom (igs, item, ipatch_sli_writer_inst_find_func);
    if (ig) /* found in a group */
      insts = ig->data;
    else /* if not already in a group then we create a new one */
    {
      insts = g_ptr_array_new ();
      g_ptr_array_add (insts, item);
      igs = g_slist_prepend (igs, insts);
    }
    inst_zones = ipatch_sli_inst_get_zones (item); /* ++ ref inst_zones */
    ipatch_list_init_iter (inst_zones, &iz_iter);
    for (iz = ipatch_sli_zone_first (&iz_iter);
         iz;
         iz = ipatch_sli_zone_next (&iz_iter))
    {
      IpatchIter iter;
      IpatchItem *inst;
      /* ++ ref zlist (list of zones also referencing sample of this zone) */
      zlist = ipatch_sli_get_zone_references (ipatch_sli_zone_peek_sample (iz));
      ipatch_list_init_iter (zlist, &iter);
      for (z = ipatch_sli_zone_first(&iter); z; z = ipatch_sli_zone_next(&iter))
      {
        inst = ipatch_item_peek_parent (IPATCH_ITEM (z));
        ig = g_slist_find_custom (igs, inst, ipatch_sli_writer_inst_find_func);
        if (!ig) /* not yet in any group: add to current */
        {
          g_ptr_array_add (insts, inst);
        }
        else if (ig->data != insts) /* already in another group: merge groups */
        {
          GPtrArray *igrp = ig->data;
          int i;

          for (i = igrp->len - 1; i >= 0; i--)
          {
            g_ptr_array_add (insts, g_ptr_array_index (igrp, i));
            g_ptr_array_remove_index_fast (igrp, i);
          }
          g_ptr_array_free (igrp, TRUE);
          igs = g_slist_delete_link (igs, ig);
        }
      }
      g_object_unref (zlist); /* -- unref zlist */
    }
    g_object_unref (inst_zones); /* -- unref inst_zones */
  }
  igs = g_slist_reverse (igs); /* fixup for prepending to preserve order */
  return (igs);
}

static gboolean
ipatch_sli_writer_write_group (IpatchSLIWriter *writer, GPtrArray *ig, GError **err)
{
  guint i, cnt, allzones = 0, maxzones = 0, smpdata_size = 0;
  IpatchList *inst_zones;
  IpatchIter iz_iter;
  IpatchSLIInst *inst;
  IpatchSLIZone *zone;
  IpatchSLISample *sample;
  SampleHashValue *sample_info;
  GPtrArray *samples;
  IpatchSLISiIg siig;
  IpatchSLIInstHeader ihdr;
  IpatchSLISiDp sidp = { IPATCH_SLI_FOURCC_SIDP, IPATCH_SLI_SIDP_SIZE,
                         IPATCH_SLI_SPECHDR_VAL, 0 };
  guint pos;

  pos = ipatch_file_get_position (writer->handle);
  samples = g_ptr_array_new();
  /* prepare buffer for headers */
  ipatch_file_buf_zero (writer->handle, IPATCH_SLI_HEAD_SIZE);
  /* loop over instruments in group */
  for (i = 0; i < ig->len; i++)
  {
    inst = g_ptr_array_index (ig, i);
    inst_zones = ipatch_sli_inst_get_zones (inst); /* ++ ref inst_zones */
    ipatch_list_init_iter (inst_zones, &iz_iter);
    cnt = ipatch_iter_count (&iz_iter);
    /* write instrument header */
    strncpy (ihdr.name, inst->name, IPATCH_SLI_NAME_SIZE);
    ihdr.sound_id = (inst->sound_id ? inst->sound_id : g_str_hash (inst->name));
    ihdr.unused1 = 0;
    ihdr.category = inst->category;
    ihdr.unused2 = 0;
    ihdr.zone_idx = allzones;
    ihdr.zones_num = cnt;
    if (cnt > maxzones) maxzones = cnt;
    allzones += cnt;
    ipatch_file_buf_seek (writer->handle,
                          IPATCH_SLI_SIIG_SIZE + i * IPATCH_SLI_INST_SIZE,
                          G_SEEK_SET);
    ipatch_sli_writer_write_inst_header (writer->handle, &ihdr);
    /* loop over zones of this instrument */
    ipatch_file_buf_seek (writer->handle,
                          IPATCH_SLI_SIIG_SIZE +
                          ig->len * IPATCH_SLI_INST_SIZE +
                          ihdr.zone_idx * IPATCH_SLI_ZONE_SIZE,
                          G_SEEK_SET);
    for (zone = ipatch_sli_zone_first (&iz_iter);
         zone;
         zone = ipatch_sli_zone_next (&iz_iter))
    {
      int format;
      /* write zone header */
      sample = ipatch_sli_zone_peek_sample (zone);
      sample_info = g_hash_table_lookup (writer->sample_hash, sample);
      ipatch_sli_writer_write_zone_header (writer->handle, zone,
                        (sample_info ? sample_info->index : samples->len));
      /* check referenced sample: if already counted then continue */
      if (sample_info) continue;
      /* else check sample format and add info to sample hash */
      format = ipatch_sample_get_format ((IpatchSample *)sample);
      if (IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT(format) > 2)
      {
        char *n = ipatch_sli_sample_get_name (sample);
        g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNSUPPORTED,
                     _("Unsupported channel count in sample '%s'"), n);
        if (n) free(n);
        g_object_unref (inst_zones); /* -- unref inst_zones */
        return (FALSE);
      }
      format &= IPATCH_SAMPLE_CHANNEL_MASK;
      format |= FORMAT_16BIT;
      sample_info = ipatch_sli_writer_sample_hash_value_new ();
      sample_info->index = samples->len;
      sample_info->channels = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT(format);
      g_hash_table_insert (writer->sample_hash, sample, sample_info);
      g_ptr_array_add (samples, sample);
      sample_info->offset = smpdata_size;
      sample_info->position = pos + sample_info->offset;
      sample_info->length =
        ipatch_sample_get_size (IPATCH_SAMPLE (sample), NULL) *
        ipatch_sample_format_size (format);
      /* plus 64 zero bytes written for each channel*/
      smpdata_size += sample_info->length + sample_info->channels * 64;
    }
    g_object_unref (inst_zones); /* -- unref inst_zones */
  }

  /* now rewind to the beginning and write group header */
  ipatch_file_buf_seek (writer->handle, 0, G_SEEK_SET);
  siig.ckid = IPATCH_SLI_FOURCC_SIIG;
  siig.cklen = IPATCH_SLI_SIIG_SIZE; /* group header */
  siig.cklen += ig->len * IPATCH_SLI_INST_SIZE;  /* instrument headers */
  siig.cklen += allzones * IPATCH_SLI_ZONE_SIZE; /* zone headers */
  siig.cklen += samples->len * IPATCH_SLI_SMPL_SIZE;  /* sample headers */
  if (siig.cklen >= IPATCH_SLI_HEAD_SIZE)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_RIFF_ERROR_SIZE_EXCEEDED,
            _("Too many instruments, zones or samples. Header size exceeded."));
    return (FALSE);
  }
  siig.cklen += smpdata_size; /* sample data */
  siig.spechdr = IPATCH_SLI_SPECHDR_VAL;
  siig.unused1 = 0;
  siig.inst_offs = IPATCH_SLI_SIIG_SIZE;
  siig.instnum = ig->len;
  siig.zones_offs = siig.inst_offs + siig.instnum * IPATCH_SLI_INST_SIZE;
  siig.allzones_num = allzones;
  siig.smphdr_offs = siig.zones_offs + siig.allzones_num * IPATCH_SLI_ZONE_SIZE;
  siig.maxzones_num = maxzones;
  siig.smpdata_offs = siig.smphdr_offs + samples->len * IPATCH_SLI_SMPL_SIZE;
  siig.unused2 = 0;
  ipatch_sli_writer_write_siig (writer->handle, &siig);

  /* write sample headers */
  ipatch_file_buf_seek (writer->handle, siig.smphdr_offs, G_SEEK_SET);
  for (i = 0; i < samples->len; i++)
  {
    sample_info = g_hash_table_lookup (writer->sample_hash, sample);
    sample_info->position += siig.smphdr_offs;
    ipatch_sli_writer_write_sample_header (writer->handle, sample_info,
                                           g_ptr_array_index (samples, i));
  }

  /* finished assembling headers, commit to the file now */
  ipatch_file_buf_set_size (writer->handle,
                            ipatch_file_get_position(writer->handle) - pos);
  if (!ipatch_file_buf_commit (writer->handle, err))
    return FALSE;

  /* write sample data */
  /* write sample headers */
  for (i = 0; i < samples->len; i++)
    if (!ipatch_sli_writer_write_sample_data (writer->handle,
                                              g_ptr_array_index (samples, i),
                                              err))
      return FALSE;

  /* write SiDp headers (one for each instrument) */
  for (i = 0; i < ig->len; i++)
    ipatch_sli_writer_write_sidp (writer->handle, &sidp);
  if (!ipatch_file_buf_commit (writer->handle, err))
    return FALSE;

  return TRUE;
}

static gboolean
ipatch_sli_writer_write_sifi (IpatchFileHandle *handle, guint ignum, GError **err)
{
  guint32 header[2] = {IPATCH_SLI_FOURCC_SIFI, 0};

  /* Header is written without possible swapping for endianness
   * Fixup cklen later after writing whole file */
  ipatch_file_buf_write (handle, header, 8);
  ipatch_file_buf_write_u16 (handle, IPATCH_SLI_SPECHDR_VAL);
  ipatch_file_buf_write_u16 (handle, 0);
  ipatch_file_buf_write_u16 (handle, ignum);
  ipatch_file_buf_write_u16 (handle,
                             IPATCH_RIFF_HEADER_SIZE + IPATCH_SLI_SIFI_SIZE);
  return ipatch_file_buf_commit (handle, err);
}

static void
ipatch_sli_writer_write_siig (IpatchFileHandle *handle, IpatchSLISiIg *siig)
{
  /* id is written without possible swapping for endianness */
  ipatch_file_buf_write (handle, &(siig->ckid), 4);
  ipatch_file_buf_write_u32 (handle, siig->cklen);
  ipatch_file_buf_write_u16 (handle, siig->spechdr);
  ipatch_file_buf_write_u16 (handle, siig->unused1);
  ipatch_file_buf_write_u16 (handle, siig->inst_offs);
  ipatch_file_buf_write_u16 (handle, siig->instnum);
  ipatch_file_buf_write_u16 (handle, siig->zones_offs);
  ipatch_file_buf_write_u16 (handle, siig->allzones_num);
  ipatch_file_buf_write_u16 (handle, siig->smphdr_offs);
  ipatch_file_buf_write_u16 (handle, siig->maxzones_num);
  ipatch_file_buf_write_u16 (handle, siig->smpdata_offs);
  ipatch_file_buf_write_u16 (handle, siig->unused2);
}

static void
ipatch_sli_writer_write_inst_header (IpatchFileHandle *handle, IpatchSLIInstHeader *ihdr)
{
  ipatch_file_buf_write (handle, ihdr->name, IPATCH_SLI_NAME_SIZE);
  ipatch_file_buf_write_u32 (handle, ihdr->sound_id);
  ipatch_file_buf_write_u32 (handle, ihdr->unused1);
  ipatch_file_buf_write_u16 (handle, ihdr->category);
  ipatch_file_buf_write_u16 (handle, ihdr->unused2);
  ipatch_file_buf_write_u16 (handle, ihdr->zone_idx);
  ipatch_file_buf_write_u16 (handle, ihdr->zones_num);
}

static void
ipatch_sli_writer_write_zone_header (IpatchFileHandle *handle, IpatchSLIZone *zone, guint sample_idx)
{
  IpatchSF2GenItem *item;
  IpatchSF2GenAmount amount;
  guint32 offs;

  item = IPATCH_SF2_GEN_ITEM (zone);

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_NOTE_RANGE, &amount);
  ipatch_file_buf_write_u8 (handle, amount.range.low); /* keyrange_low */
  ipatch_file_buf_write_u8 (handle, amount.range.high); /* keyrange_high */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VELOCITY_RANGE, &amount);
  ipatch_file_buf_write_u8 (handle, amount.range.low); /* velrange_low */
  ipatch_file_buf_write_u8 (handle, amount.range.high); /* velrange_high */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_SAMPLE_COARSE_START, &amount);
  offs = amount.uword << 16;
  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_SAMPLE_START, &amount);
  offs += amount.uword << 1;
  ipatch_file_buf_write_u32 (handle, offs); /* start_offs1 */
  ipatch_file_buf_write_u32 (handle, offs); /* start_offs2 */

  ipatch_file_buf_write_u32 (handle, 0); /* unknown1 */
  ipatch_file_buf_write_u32 (handle, 0); /* unknown2 */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_COARSE_TUNE, &amount);
  ipatch_file_buf_write_s8 (handle, amount.sword); /* coarse_tune1 */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amount);
  ipatch_file_buf_write_s8 (handle, amount.sword); /* fine_tune1 */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_SAMPLE_MODES, &amount);
  if (amount.uword & IPATCH_SF2_GEN_SAMPLE_MODE_LOOP)
    zone->flags |= IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
  ipatch_file_buf_write_u8 (handle, zone->flags); /* sample_modes */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword); /* root_note */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_SCALE_TUNE, &amount))
    amount.uword = 0;
  ipatch_file_buf_write_u16 (handle, amount.uword); /* scale_tuning */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_COARSE_TUNE, &amount);
  ipatch_file_buf_write_s8 (handle, amount.sword); /* coarse_tune2 */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amount);
  ipatch_file_buf_write_s8 (handle, amount.sword); /* fine_tune2 */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_LFO_TO_PITCH, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* modLfoToPitch */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VIB_LFO_TO_PITCH, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* vibLfoToPitch */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_TO_PITCH, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* modEnvToPitch */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_FILTER_CUTOFF, &amount))
    amount.uword = 0;
  ipatch_file_buf_write_u16 (handle, amount.uword); /* initialFilterFc */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_FILTER_Q, &amount);
  ipatch_file_buf_write_u16 (handle, amount.uword); /* initialFilterQ */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_CUTOFF, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* modLfoToFilterFc */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_CUTOFF, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* modEnvToFilterFc */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_LFO_TO_VOLUME, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* modLfoToVolume */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_LFO_FREQ, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* freqModLfo */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VIB_LFO_FREQ, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* freqVibLfo */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_SUSTAIN, &amount);
  ipatch_file_buf_write_u16 (handle, amount.uword); /* sustainModEnv */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* keynumToModEnvHold */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* keynumToModEnvDecay */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VOL_ENV_SUSTAIN, &amount);
  ipatch_file_buf_write_u16 (handle, amount.uword); /* sustainVolEnv */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* keynumToVolEnvHold */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY, &amount);
  ipatch_file_buf_write_s16 (handle, amount.sword); /* keynumToVolEnvDecay */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_PAN, &amount);
  ipatch_file_buf_write_s8 (handle, amount.sword / 5); /* pan */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_LFO_DELAY, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* delayModLfo */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VIB_LFO_DELAY, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* delayVibLfo */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_ATTACK, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* attackModEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_HOLD, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* holdModEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_DECAY, &amount))
    amount.sword =0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* decayModEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_MOD_ENV_RELEASE, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* releaseModEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VOL_ENV_ATTACK, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* attackVolEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VOL_ENV_HOLD, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* holdVolEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VOL_ENV_DECAY, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* decayVolEnv */

  if (!ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_VOL_ENV_RELEASE, &amount))
    amount.sword = 0;
  ipatch_file_buf_write_s8 (handle, amount.sword / 100); /* releaseVolEnv */

  ipatch_sf2_gen_item_get_amount (item, IPATCH_SF2_GEN_ATTENUATION, &amount);
  ipatch_file_buf_write_u8 (handle, amount.uword / 10); /* initialAttenuation */

  ipatch_file_buf_write_u16 (handle, sample_idx); /* sample_idx */
  ipatch_file_buf_write_u16 (handle, 0); /* unused */
}

static void
ipatch_sli_writer_write_sample_header (IpatchFileHandle *handle,
                                       SampleHashValue *sample_info,
                                       IpatchSLISample *sample)
{
  char sname[IPATCH_SLI_NAME_SIZE];

  strncpy (sname, sample->name, IPATCH_SLI_NAME_SIZE);
  ipatch_file_buf_write (handle, sname, IPATCH_SLI_NAME_SIZE);
  ipatch_file_buf_write_u32 (handle, sample_info->offset);
  ipatch_file_buf_write_u32 (handle, sample_info->offset + sample_info->length);
  ipatch_file_buf_write_u32 (handle, sample->loop_start * 2);
  ipatch_file_buf_write_u32 (handle, sample->loop_end * 2);
  ipatch_file_buf_write_s8 (handle, sample->fine_tune);
  ipatch_file_buf_write_u8 (handle, sample->root_note);
  ipatch_file_buf_write_u8 (handle, sample_info->channels);
  ipatch_file_buf_write_u8 (handle, 16); /* bits per sample is verified above */
  ipatch_file_buf_write_u32 (handle, sample->rate);
}

static void
ipatch_sli_writer_write_sidp (IpatchFileHandle *handle, IpatchSLISiDp *sidp)
{
  /* id is written without possible swapping for endianness */
  ipatch_file_buf_write (handle, &(sidp->ckid), 4);
  ipatch_file_buf_write_u32 (handle, sidp->cklen);
  ipatch_file_buf_write_u16 (handle, sidp->spechdr);
  ipatch_file_buf_write_u16 (handle, sidp->unused);
}

static gboolean
ipatch_sli_writer_write_sample_data (IpatchFileHandle *handle, IpatchSLISample *sample, GError **err)
{
  IpatchSampleHandle shandle;
  gpointer buf;
  guint samsize, fmt_size, read_size, offs;
  int format;

  samsize = ipatch_sample_get_size (IPATCH_SAMPLE (sample->sample_data), NULL);
  format = ipatch_sample_get_format (IPATCH_SAMPLE (sample->sample_data));
  format &= IPATCH_SAMPLE_CHANNEL_MASK;
  format |= FORMAT_16BIT;
  fmt_size = ipatch_sample_format_size (format);
  /* ++ open sample handle */
  if (!ipatch_sample_data_open_native_sample (sample->sample_data,
                                              &shandle, 'r', format,
                                              IPATCH_SAMPLE_UNITY_CHANNEL_MAP,
                                              err))
    return FALSE;

  read_size = ipatch_sample_handle_get_max_frames (&shandle);
  for (offs = 0; offs < samsize; offs += read_size)
  {
    if (samsize - offs < read_size) /* check for last partial fragment */
      read_size = samsize - offs;

    /* read and transform (if necessary) audio data from sample store */
    if (!(buf = ipatch_sample_handle_read (&shandle, offs, read_size,
                                           NULL, err)) ||
        !ipatch_file_write (handle, buf, read_size * fmt_size, err))
    {
      ipatch_sample_handle_close (&shandle); /* -- close sample handle */
      return FALSE;
    }
  }
  ipatch_sample_handle_close (&shandle); /* -- close sample handle */

  /* write 64 "zero" samples (for each channel) following sample data */
  ipatch_file_buf_zero (handle,
                        64 * IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT(format));
  return ipatch_file_buf_commit (handle, err);
}
