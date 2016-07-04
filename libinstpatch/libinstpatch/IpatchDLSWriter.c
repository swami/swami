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
/**
 * SECTION: IpatchDLSWriter
 * @short_description: DLS instrument file writer
 * @see_also: #IpatchDLS
 * @stability: Stable
 *
 * Writes a DLS instrument object tree (#IpatchDLS) to a DLS file.
 */
#include <glib.h>
#include <string.h>

#include "IpatchDLSWriter.h"
#include "IpatchDLSFile.h"
#include "IpatchDLSFile_priv.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLS2Sample.h"
#include "IpatchGig.h"
#include "IpatchGigFile.h"
#include "IpatchGigFile_priv.h"
#include "IpatchGigRegion.h"
#include "IpatchGigInst.h"
#include "IpatchGigSample.h"
#include "IpatchItem.h"
#include "IpatchSampleData.h"
#include "IpatchSampleStoreFile.h"
#include "IpatchSample.h"
#include "ipatch_priv.h"
#include "i18n.h"

static void ipatch_dls_writer_finalize (GObject *object);
static gboolean ipatch_dls_write_level_0 (IpatchDLSWriter *writer,
					  GError **err);
static gboolean dls_write_info (IpatchDLSWriter *writer, GSList *info_list,
				GError **err);
static IpatchDLS2InfoBag *find_info_by_id (GSList *info_list, guint32 id);
static gboolean gig_write_file_info (IpatchDLSWriter *writer,
				     GSList *info_list, GError **err);
static gboolean gig_write_name_info (IpatchDLSWriter *writer,
				     GSList *info_list, GError **err);

static gboolean dls_write_inst_list (IpatchDLSWriter *writer, GError **err);
static gboolean dls_write_region_list (IpatchDLSWriter *writer,
				      IpatchDLS2Inst *inst, GError **err);
static gboolean gig_write_region_list (IpatchDLSWriter *writer,
				       IpatchGigInst *giginst,
				       GError **err);
static gboolean dls_write_art_list (IpatchDLSWriter *writer, GSList *conn_list,
				   GError **err);
static gboolean dls_write_region_header (IpatchDLSWriter *writer,
					 IpatchDLS2Region *region,
					 GError **err);
static gboolean gig_write_region_header (IpatchDLSWriter *writer,
					 IpatchGigRegion *region,
					 GError **err);

static gboolean dls_write_wave_link (IpatchDLSWriter *writer,
				     IpatchDLS2Region *region,
				     GError **err);
static gboolean gig_write_wave_link (IpatchDLSWriter *writer,
				     IpatchGigRegion *region,
				     GError **err);

static gboolean dls_write_sample_info (IpatchDLSWriter *writer,
				       IpatchDLS2SampleInfo *info,
				       GError **err);
static gboolean dls_write_sample_format (IpatchDLSWriter *writer,
					 IpatchDLS2Sample *sample,
					 GError **err);

static gboolean dls_reserve_pool_table (IpatchDLSWriter *writer, GError **err);
static gboolean dls_fixup_pool_table (IpatchDLSWriter *writer, GError **err);
static gboolean dls_write_wave_pool (IpatchDLSWriter *writer, GError **err);
static gboolean dls_write_dlid (IpatchDLSWriter *writer, guint8 *dlid,
				GError **err);

static gboolean gig_write_sub_regions (IpatchDLSWriter *writer,
				      IpatchGigRegion *region, GError **err);
static gboolean gig_write_dimension_names (IpatchDLSWriter *writer,
					  IpatchGigRegion *region,
					  GError **err);
static gboolean gig_write_sample_info (IpatchDLSWriter *writer,
				       IpatchDLS2SampleInfo *info, int rate,
				       GError **err);
static gboolean gig_write_dimension_info (IpatchDLSWriter *writer,
					  IpatchGigRegion *region,
					  GError **err);
static gboolean gig_write_group_names (IpatchDLSWriter *writer, GError **err);


G_DEFINE_TYPE (IpatchDLSWriter, ipatch_dls_writer, IPATCH_TYPE_RIFF);


static void
ipatch_dls_writer_class_init (IpatchDLSWriterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_dls_writer_finalize;
}

static void
ipatch_dls_writer_init (IpatchDLSWriter *writer)
{
  writer->sample_hash = g_hash_table_new (NULL, NULL);
}

static void
ipatch_dls_writer_finalize (GObject *object)
{
  IpatchDLSWriter *writer = IPATCH_DLS_WRITER (object);

  if (writer->orig_dls) g_object_unref (writer->orig_dls);
  if (writer->dls) g_object_unref (writer->dls);
  g_hash_table_destroy (writer->sample_hash);
  if (writer->sample_ofstbl) g_free (writer->sample_ofstbl);
  if (writer->sample_postbl) g_free (writer->sample_postbl);
  if (writer->store_list) g_object_unref (writer->store_list);

  if (G_OBJECT_CLASS (ipatch_dls_writer_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_dls_writer_parent_class)->finalize (object);
}

/**
 * ipatch_dls_writer_new:
 * @handle: DLS file handle to save to or %NULL to set later, taken over by
 *   writer object and will be closed on finalize.
 * @dls: DLS object to save or %NULL to set later
 *
 * Create a new DLS file writer.
 *
 * Returns: The new DLS writer
 */
IpatchDLSWriter *
ipatch_dls_writer_new (IpatchFileHandle *handle, IpatchDLS2 *dls)
{
  IpatchDLSWriter *writer;

  g_return_val_if_fail (!handle || IPATCH_IS_DLS_FILE (handle->file), NULL);
  g_return_val_if_fail (!dls || IPATCH_IS_DLS2 (dls), NULL);

  writer = g_object_new (IPATCH_TYPE_DLS_WRITER, NULL);
  if (handle) ipatch_dls_writer_set_file_handle (writer, handle);
  if (dls) ipatch_dls_writer_set_patch (writer, dls);

  return (writer);
}

/**
 * ipatch_dls_writer_set_patch:
 * @writer: DLS writer object
 * @dls: DLS patch to save
 *
 * Set the DLS patch object to save with a DLS writer.
 */
void
ipatch_dls_writer_set_patch (IpatchDLSWriter *writer, IpatchDLS2 *dls)
{
  g_return_if_fail (IPATCH_IS_DLS_WRITER (writer));
  g_return_if_fail (IPATCH_IS_DLS2 (dls));

  if (writer->orig_dls) g_object_unref (writer->orig_dls);
  writer->orig_dls = g_object_ref (dls);
}

/**
 * ipatch_dls_writer_set_file_handle:
 * @writer: DLS writer object
 * @handle: DLS file handle
 *
 * Set the DLS file handle of a DLS writer. A convenience function, since
 * ipatch_riff_set_file_handle() could also be used.
 */
void
ipatch_dls_writer_set_file_handle (IpatchDLSWriter *writer, IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_DLS_WRITER (writer));
  g_return_if_fail (handle && IPATCH_IS_DLS_FILE (handle->file));

  ipatch_riff_set_file_handle (IPATCH_RIFF (writer), handle);
}

/**
 * ipatch_dls_writer_save:
 * @writer: DLS writer object
 * @err: Location to store error info or %NULL
 *
 * Write a DLS or GigaSampler object to a file.
 *
 * Returns: %TRUE on success, %FALSE on error
 */
gboolean
ipatch_dls_writer_save (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff;
  IpatchItem *item;

  g_return_val_if_fail (IPATCH_IS_DLS_WRITER (writer), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);
  g_return_val_if_fail (writer->orig_dls != NULL, FALSE);

  if (writer->dls) g_object_unref (writer->dls); /* shouldn't be set, but.. */

  /* are we writing a GigaSampler file? */
  if (IPATCH_IS_GIG (writer->orig_dls)) writer->is_gig = TRUE;

  /* duplicate for save, so we can be multi-thread friendly :)
     ++ref new duplicate object */
  item = ipatch_item_duplicate (IPATCH_ITEM (writer->orig_dls));
  g_return_val_if_fail (item != NULL, FALSE);
  writer->dls = IPATCH_DLS2 (item);
  
  riff = IPATCH_RIFF (writer);

  /* <DLS > - Toplevel DLS RIFF chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_RIFF,
				IPATCH_DLS_FOURCC_DLS, err))
    return (FALSE);

  if (!ipatch_dls_write_level_0 (writer, err))
    goto err;

  if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
  /* </DLS > */

  return (TRUE);

 err:

  g_object_unref (writer->dls);
  writer->dls = NULL;

  return (FALSE);
}

/**
 * ipatch_dls_writer_create_stores:
 * @writer: SoundFont writer object
 *
 * Create sample stores and add them to applicable #IpatchSampleData objects and return object list.
 * This function can be called multiple times, additional calls will return the same list.
 *
 * Returns: (transfer full): List of sample stores which the caller owns a reference to or %NULL
 *
 * Since: 1.1.0
 */
IpatchList *
ipatch_dls_writer_create_stores (IpatchDLSWriter *writer)
{
  guint sample_index;
  IpatchSample *newstore;
  IpatchFile *save_file;
  IpatchDLS2Sample *sample;
  IpatchIter iter;
  IpatchList *list;
  int rate, format;
  guint size, pos;

  g_return_val_if_fail (writer->dls != NULL, NULL);

  // Return existing store list (if this function has been called before)
  if (writer->store_list)
    return (g_object_ref (writer->store_list));         // ++ ref for caller

  save_file = IPATCH_RIFF (writer)->handle->file;

  list = ipatch_list_new ();            // ++ ref list

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->dls), &iter,
			      IPATCH_TYPE_DLS2_SAMPLE);

  /* traverse samples */
  for (sample = ipatch_dls2_sample_first (&iter); sample;
       sample = ipatch_dls2_sample_next (&iter))
  {
    sample_index = GPOINTER_TO_UINT (g_hash_table_lookup (writer->sample_hash, sample));

    /* Hash_value should never be NULL, but.. */
    if (!sample_index) continue;

    pos = writer->sample_postbl[sample_index - 1];      // sample index is +1 to catch NULL

    g_object_get (sample,
                  "sample-format", &format,
                  "sample-size", &size,
                  "sample-rate", &rate,
                  NULL);

    newstore = ipatch_sample_store_file_new (save_file, pos);

    g_object_set (newstore,
                  "sample-format", format,
                  "sample-size", size,
                  "sample-rate", rate,
                  NULL);

    ipatch_sample_data_add (sample->sample_data, (IpatchSampleStore *)newstore);

    list->items = g_list_prepend (list->items, newstore);       // !! list takes over reference
  }

  writer->store_list = g_object_ref (list);     // ++ ref for writer object

  return (list);        // !! caller takes over reference
}

static gboolean
ipatch_dls_write_level_0 (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchIter iter;
  IpatchDLS2Sample *sample;
  guint index;

  /* write info list */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INFO, err))
    return (FALSE);

  if (!writer->is_gig)		/* DLS file? */
    {
      if (!dls_write_info (writer, writer->dls->info, err)) return (FALSE);
    }
  else				/* gig file */
    {
      if (!gig_write_file_info (writer, writer->dls->info, err))
	return (FALSE);
    }

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* save file version if its set */
  if (ipatch_item_get_flags (writer->dls) & IPATCH_DLS2_VERSION_SET)
    {
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_VERS, err))
	return (FALSE);
      if (!ipatch_file_write_u32 (riff->handle, writer->dls->ms_version, err))
	return (FALSE);
      if (!ipatch_file_write_u32 (riff->handle, writer->dls->ls_version, err))
	return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
    }

  /* collection header (instrument count) */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_COLH, err))
    return (FALSE);
  ipatch_container_init_iter (IPATCH_CONTAINER (writer->dls), &iter,
			      IPATCH_TYPE_DLS2_INST);
  if (!ipatch_file_write_u32 (riff->handle, ipatch_iter_count (&iter), err))
    return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* write DLSID if its set */
  if (writer->dls->dlid)
    if (!dls_write_dlid (writer, writer->dls->dlid, err)) return (FALSE);

  /* create hash of samples -> indexes */
  ipatch_container_init_iter (IPATCH_CONTAINER (writer->dls), &iter,
			      IPATCH_TYPE_DLS2_SAMPLE);
  sample = ipatch_dls2_sample_first (&iter);
  index = 1;		   /* index + 1 to catch NULL in hash table */
  while (sample)
    {
      g_hash_table_insert (writer->sample_hash, sample,
			   GUINT_TO_POINTER (index));
      sample = ipatch_dls2_sample_next (&iter);
      index++;
    }
  writer->sample_count = index - 1; /* count of samples */

  /* allocate sample offset and sample data position tables */
  writer->sample_ofstbl = g_malloc0 (writer->sample_count * 4);
  writer->sample_postbl = g_malloc0 (writer->sample_count * 4);

  /* write instrument list */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_LINS, err))
    return (FALSE);
  if (!dls_write_inst_list (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* GigaSampler sample group name list */
  if (writer->is_gig)
    if (!gig_write_group_names (writer, err))
      return (FALSE);

  /* reserve pool table (sample mappings) chunk */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_PTBL, err))
    return (FALSE);
  if (!dls_reserve_pool_table (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* write wave pool list (all samples) */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_WVPL, err))
    return (FALSE);
  if (!dls_write_wave_pool (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* fixup the pool table */
  if (!dls_fixup_pool_table (writer, err)) return (FALSE);

  /* FIXME: IPATCH_GIG_FOURCC_EINF - GigaSampler unknown */

  return (TRUE);
}

static gboolean
dls_write_info (IpatchDLSWriter *writer, GSList *info_list, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2InfoBag *bag;
  GSList *p;

  p = info_list;
  while (p)
    {
      bag = (IpatchDLS2InfoBag *)(p->data);

      if (!ipatch_riff_write_sub_chunk (riff, bag->fourcc, err))
	return (FALSE);
      if (!ipatch_file_write (riff->handle, bag->value,	/* write info str */
			      strlen (bag->value) + 1, err))
	return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

      p = g_slist_next (p);
    }

  return (TRUE);
}

static IpatchDLS2InfoBag *
find_info_by_id (GSList *info_list, guint32 id)
{
  IpatchDLS2InfoBag *bag;
  GSList *p;

  p = info_list;
  while (p)
    {
      bag = (IpatchDLS2InfoBag *)(p->data);
      if (bag->fourcc == id) break;
      p = g_slist_next (p);
    }

  if (p) return (bag);
  else return (NULL);
}

/* GigaSampler file info write function */
static gboolean
gig_write_file_info (IpatchDLSWriter *writer, GSList *info_list, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2InfoBag *bag;
  char *buf;
  int i, size;
  guint32 order[] = {
    IPATCH_DLS_FOURCC_IARL, IPATCH_DLS_FOURCC_IART, IPATCH_DLS_FOURCC_ICMS,
    IPATCH_DLS_FOURCC_ICMT, IPATCH_DLS_FOURCC_ICOP, IPATCH_DLS_FOURCC_ICRD,
    IPATCH_DLS_FOURCC_IENG, IPATCH_DLS_FOURCC_IGNR, IPATCH_DLS_FOURCC_IKEY,
    IPATCH_DLS_FOURCC_IMED, IPATCH_DLS_FOURCC_INAM, IPATCH_DLS_FOURCC_IPRD,
    IPATCH_DLS_FOURCC_ISBJ, IPATCH_DLS_FOURCC_ISFT, IPATCH_DLS_FOURCC_ISRC,
    IPATCH_DLS_FOURCC_ISRF, IPATCH_DLS_FOURCC_ITCH
    };

  buf = g_malloc (1024);	/* max size is comment field */

  for (i = 0; i < G_N_ELEMENTS (order); i++)
    {
      if (order[i] == IPATCH_DLS_FOURCC_IARL)
	size = IPATCH_GIG_IARL_INFO_SIZE;
      else if (order[i] == IPATCH_DLS_FOURCC_ICMT)
	size = IPATCH_GIG_ICMT_INFO_SIZE;
      else size = IPATCH_GIG_MOST_INFO_SIZE;

      /* blank it first, IARL filled with spaces */
      memset (buf, (order[i] != IPATCH_DLS_FOURCC_IARL) ? 0 : ' ', size);

      bag = find_info_by_id (info_list, order[i]);
      if (bag) strncpy (buf, bag->value, size - 1);

      if (!ipatch_riff_write_sub_chunk (riff, order[i], err)) goto err;
      if (!ipatch_file_write (riff->handle, buf, size, err)) goto err;
      if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
    }

  return (TRUE);

 err:

  g_free (buf);
  return (FALSE);
}

/* write GigaSampler name info for instruments or samples */
static gboolean
gig_write_name_info (IpatchDLSWriter *writer, GSList *info_list, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2InfoBag *bag;
  char buf[IPATCH_GIG_ITEM_INAM_SIZE];

  memset (buf, 0, sizeof (buf));

  bag = find_info_by_id (info_list, IPATCH_DLS_FOURCC_INAM);
  if (bag) strncpy (buf, bag->value, sizeof (buf) - 1);

  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_INAM, err))
    return (FALSE);
  if (!ipatch_file_write (riff->handle, buf, sizeof (buf), err))
    return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  return (TRUE);
}

static gboolean
dls_write_inst_list (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2Inst *inst;
  IpatchIter iter, region_iter;
  guint32 uint;
  gboolean retval;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->dls), &iter,
			      IPATCH_TYPE_DLS2_INST);
  inst = ipatch_dls2_inst_first (&iter);
  while (inst)			/* loop over instruments */
    {
      /* <INS > - Instrument chunk */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INS, err))
	return (FALSE);

      /* <INFO> - Info list */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INFO, err))
	return (FALSE);

      if (writer->is_gig)
	{
	  if (!gig_write_name_info (writer, inst->info, err)) return (FALSE);

	  /* <ISFT> - Write ISFT info value - FIXME (write libInstPatch?) */
	  if (!ipatch_riff_write_sub_chunk (riff,
					    IPATCH_DLS_FOURCC_ISFT, err))
	    return (FALSE);
	  if (!ipatch_file_write (riff->handle, IPATCH_GIG_INST_ISFT_VAL,
				  strlen (IPATCH_GIG_INST_ISFT_VAL), err))
	    return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	  /* </ISFT> */
	}
      else if (!dls_write_info (writer, inst->info, err)) return (FALSE);

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </INFO> */

      /* write DLSID if its set */
      if (inst->dlid)
	if (!dls_write_dlid (writer, inst->dlid, err)) return (FALSE);

      /* <INSH> - write instrument header chunk */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_INSH, err))
	return (FALSE);

      /* region count */
      if (!writer->is_gig)
	ipatch_container_init_iter ((IpatchContainer *)inst, &region_iter,
				    IPATCH_TYPE_DLS2_REGION);
      else
	ipatch_container_init_iter ((IpatchContainer *)inst, &region_iter,
				    IPATCH_TYPE_GIG_REGION);

      ipatch_file_buf_write_u32 (riff->handle, ipatch_iter_count (&region_iter));

      uint = inst->bank | ((ipatch_item_get_flags (inst) & IPATCH_DLS2_INST_PERCUSSION)
			   ? IPATCH_DLS_INSH_BANK_PERCUSSION : 0);
      ipatch_file_buf_write_u32 (riff->handle, uint);
      ipatch_file_buf_write_u32 (riff->handle, inst->program);

      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </INSH> */

      /* <LRGN> - region list */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_LRGN, err))
	return (FALSE);

      if (!writer->is_gig)
	retval = dls_write_region_list (writer, inst, err);
      else retval = gig_write_region_list (writer, IPATCH_GIG_INST (inst), err);

      if (!retval) return (FALSE);

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </LRGN> */

      /* <LAR2> - Global DLS2 articulator list */
      if (inst->conns)
	{
	  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_LAR2,
					     err)) return (FALSE);
	  if (!dls_write_art_list (writer, inst->conns, err)) return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	}
      /* </LAR2> */

      /* FIXME - Global DLS1 articulators? */

      /* if GigaSampler - write 3ewg in lart list (global region params) */
      if (writer->is_gig)
	{ /* <lart> */
	  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_LART,
					     err)) return (FALSE);
	  /* <3ewg> - GigaSampler 3ewg chunk */
	  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3EWG, err))
	    return (FALSE);

	  if (!ipatch_file_write (riff->handle,
				  &(IPATCH_GIG_INST (inst)->chunk_3ewg),
				  IPATCH_GIG_3EWG_SIZE, err))
	    return (FALSE);

	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	  /* </3ewg> */

	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	  /* </lart> */
	}

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </INS > */

      inst = ipatch_dls2_inst_next (&iter); /* next instrument */
    } /* while (p) - instrument loop */

  return (TRUE);
}

static gboolean
dls_write_region_list (IpatchDLSWriter *writer, IpatchDLS2Inst *inst,
		       GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2Region *region;
  IpatchIter iter;

  ipatch_container_init_iter ((IpatchContainer *)inst, &iter,
			      IPATCH_TYPE_DLS2_REGION);

  region = ipatch_dls2_region_first (&iter);
  while (region)		/* loop over regions */
    {
      /* FIXME: <RGN> DLS1 chunks? */

      /* <RGN2> - DLS2 region */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_RGN2, err))
	return (FALSE);

      /* FIXME: <CDL> - conditional chunk */

      if (region->info)
	{ /* <INFO> - Region info */
	  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INFO, err))
	    return (FALSE);
	  if (!dls_write_info (writer, region->info, err)) return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	} /* </INFO> */

      /* <RGNH> - Region header */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_RGNH, err))
	return (FALSE);
      if (!dls_write_region_header (writer, region, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </RGNH> */

      if (region->sample_info)
	{ /* <WSMP> - Global sample info override */
	  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WSMP, err))
	    return (FALSE);
	  if (!dls_write_sample_info (writer, region->sample_info, err))
	    return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	} /* </WSMP> */

      /* <WLNK> - Wave link */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WLNK, err))
	return (FALSE);
      if (!dls_write_wave_link (writer, region, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </WLNK> */

      /* FIXME: <LART> - DLS1 articulators? */

      if (region->conns)
	{ /* <LAR2> - DLS2 articulators */
	  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_LAR2, err))
	    return (FALSE);
	  if (!dls_write_art_list (writer, region->conns, err))
	    return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	} /* </LAR2> */

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </RGN2> */

      region = ipatch_dls2_region_next (&iter);
    }

  return (TRUE);
}

static gboolean
gig_write_region_list (IpatchDLSWriter *writer, IpatchGigInst *giginst,
		       GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2Inst *inst = IPATCH_DLS2_INST (giginst);
  IpatchGigRegion *region;
  IpatchGigSubRegion *subregion;
  IpatchIter iter;

  ipatch_container_init_iter ((IpatchContainer *)inst, &iter,
				IPATCH_TYPE_GIG_REGION);

  region = ipatch_gig_region_first (&iter);
  while (region)		/* loop over regions */
    {
      /* <RGN > - GigaSampler region */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_RGN, err))
	return (FALSE);

      if (region->info)
	{ /* <INFO> - Region info */
	  if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INFO, err))
	    return (FALSE);
	  if (!dls_write_info (writer, region->info, err)) return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	} /* </INFO> */

      /* <RGNH> - Region header */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_RGNH, err))
	return (FALSE);
      if (!gig_write_region_header (writer, region, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </RGNH> */

      if (region->sub_region_count > 0)
	subregion = region->sub_regions[0];
      else subregion = NULL;

      /* <WSMP> - This is somewhat of a dummy WSMP chunk. */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WSMP, err))
	return (FALSE);

      if (!dls_write_sample_info (writer, subregion
				  ? subregion->sample_info : NULL, err))
	return (FALSE);

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </WSMP> */

      /* <WLNK> - Wave link */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WLNK, err))
	return (FALSE);
      if (!gig_write_wave_link (writer, region, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </WLNK> */

      /* <3LNK> - GigaSampler dimension info */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3LNK, err))
	return (FALSE);
      if (!gig_write_dimension_info (writer, region, err))
	return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3LNK> */

      /* <3PRG> - GigaSampler regions */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_GIG_FOURCC_3PRG, err))
	return (FALSE);
      if (!gig_write_sub_regions (writer, region, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3PRG> */

      /* <3DNL> - GigaSampler dimension names */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_GIG_FOURCC_3DNL, err))
	return (FALSE);
      if (!gig_write_dimension_names (writer, region, err))
	return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3DNL> */

      /* FIXME: <3DDP> - what is it? (we preserve it for now) */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3DDP, err))
	return (FALSE);
      if (!ipatch_file_write (riff->handle, &region->chunk_3ddp,
			      IPATCH_GIG_3DDP_SIZE, err))
	  return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3DDP> */

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </RGN > */

      region = ipatch_gig_region_next (&iter);
    }

  return (TRUE);
}

static gboolean
dls_write_art_list (IpatchDLSWriter *writer, GSList *conn_list, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2Conn *conn;
  GSList *p;

  /* <3EWG> - Gig region global params handled elsewhere */

  /* FIXME: <CDL> - Conditional chunk */
  /* FIXME: <ART1> - DLS1 articulators? */

  if (!conn_list) return (TRUE); /* no connections? */

  /* <art2> */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_ART2, err))
    return (FALSE);

  /* write articulator header size */
  ipatch_file_buf_write_u32 (riff->handle, IPATCH_DLS_ART_HEADER_SIZE);

  /* FIXME: Preserve header expansion? */

  /* write connection count */
  ipatch_file_buf_write_u32 (riff->handle, g_slist_length (conn_list));

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  p = conn_list;
  while (p)			/* write connection blocks */
    {
      conn = (IpatchDLS2Conn *)(p->data);
      ipatch_file_buf_write_u16 (riff->handle, conn->src);
      ipatch_file_buf_write_u16 (riff->handle, conn->ctrlsrc);
      ipatch_file_buf_write_u16 (riff->handle, conn->dest);
      ipatch_file_buf_write_u16 (riff->handle, conn->trans);
      ipatch_file_buf_write_s32 (riff->handle, conn->scale);

      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
      p = g_slist_next (p);
    }

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </art2> */

  return (TRUE);
}

static gboolean
dls_write_region_header (IpatchDLSWriter *writer, IpatchDLS2Region *region,
			 GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  guint16 options = 0;

  ipatch_file_buf_write_u16 (riff->handle, region->note_range_low);
  ipatch_file_buf_write_u16 (riff->handle, region->note_range_high);
  ipatch_file_buf_write_u16 (riff->handle, region->velocity_range_low);
  ipatch_file_buf_write_u16 (riff->handle, region->velocity_range_high);

  if (ipatch_item_get_flags (region) & IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE)
    options |= IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE;
  ipatch_file_buf_write_u16 (riff->handle, options);

  ipatch_file_buf_write_u16 (riff->handle, region->key_group);

  if (region->layer_group != 0) /* optional layer field? */
    ipatch_file_buf_write_u16 (riff->handle, region->layer_group);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

static gboolean
gig_write_region_header (IpatchDLSWriter *writer, IpatchGigRegion *region,
			 GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  guint16 options = 0;

  ipatch_file_buf_write_u16 (riff->handle, region->note_range_low);
  ipatch_file_buf_write_u16 (riff->handle, region->note_range_high);
  ipatch_file_buf_write_u16 (riff->handle, region->velocity_range_low);
  ipatch_file_buf_write_u16 (riff->handle, region->velocity_range_high);

  if (ipatch_item_get_flags (region) & IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE)
    options |= IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE;
  ipatch_file_buf_write_u16 (riff->handle, options);

  ipatch_file_buf_write_u16 (riff->handle, region->key_group);

  if (region->layer_group != 0) /* optional layer field? */
    ipatch_file_buf_write_u16 (riff->handle, region->layer_group);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

static gboolean
dls_write_wave_link (IpatchDLSWriter *writer, IpatchDLS2Region *region,
		     GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  guint16 options = 0;
  guint flags;
  guint sample_index = 0;

  flags = ipatch_item_get_flags (region);
  if (flags & IPATCH_DLS2_REGION_PHASE_MASTER)
    options |= IPATCH_DLS_WLNK_PHASE_MASTER;
  if (flags & IPATCH_DLS2_REGION_MULTI_CHANNEL)
    options |= IPATCH_DLS_WLNK_MULTI_CHANNEL;

  ipatch_file_buf_write_u16 (riff->handle, options);
  ipatch_file_buf_write_u16 (riff->handle, region->phase_group);
  ipatch_file_buf_write_u32 (riff->handle, region->channel);

  /* get index of sample (index + 1 actually) */
  sample_index = GPOINTER_TO_UINT (g_hash_table_lookup (writer->sample_hash,
							region->sample));
  g_return_val_if_fail (sample_index != 0, FALSE);

  /* write sample index (subtract 1 since we store it +1 to catch NULL) */
  ipatch_file_buf_write_u32 (riff->handle, sample_index - 1);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

static gboolean
gig_write_wave_link (IpatchDLSWriter *writer, IpatchGigRegion *region,
		     GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchGigSubRegion *subregion;
  guint16 options = 0;
  guint flags;
  guint sample_index = 0;

  flags = ipatch_item_get_flags (region);
  if (flags & IPATCH_GIG_REGION_PHASE_MASTER)
    options |= IPATCH_DLS_WLNK_PHASE_MASTER;
  if (flags & IPATCH_GIG_REGION_MULTI_CHANNEL)
    options |= IPATCH_DLS_WLNK_MULTI_CHANNEL;

  ipatch_file_buf_write_u16 (riff->handle, options);
  ipatch_file_buf_write_u16 (riff->handle, region->phase_group);
  ipatch_file_buf_write_u32 (riff->handle, region->channel);

  if (region->sub_region_count > 0)
    {
      subregion = region->sub_regions[0];

      sample_index = GPOINTER_TO_UINT
	(g_hash_table_lookup (writer->sample_hash, subregion->sample));
    }

  g_return_val_if_fail (sample_index != 0, FALSE);

  /* write sample index (subtract 1 since we store it +1 to catch NULL) */
  ipatch_file_buf_write_u32 (riff->handle, sample_index - 1);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* writes sample info, @info may be NULL in which case defaults are used */
static gboolean
dls_write_sample_info (IpatchDLSWriter *writer, IpatchDLS2SampleInfo *info,
		       GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2SampleInfo definfo = IPATCH_DLS2_SAMPLE_INFO_INIT;
  guint loop_type, saveloop_type;
  guint32 options = 0;

  /* use default info if not supplied */
  if (!info) info = &definfo;

  /* write structure size */
  ipatch_file_buf_write_u32 (riff->handle, IPATCH_DLS_WSMP_HEADER_SIZE);

  ipatch_file_buf_write_u16 (riff->handle, info->root_note);
  ipatch_file_buf_write_u16 (riff->handle, info->fine_tune);
  ipatch_file_buf_write_u32 (riff->handle, info->gain);

  if (info->options & IPATCH_DLS2_SAMPLE_NO_TRUNCATION)
    options |= IPATCH_DLS_WSMP_NO_TRUNCATION;
  if (info->options & IPATCH_DLS2_SAMPLE_NO_COMPRESSION)
    options |= IPATCH_DLS_WSMP_NO_COMPRESSION;
  ipatch_file_buf_write_u32 (riff->handle, options);

  /* FIXME: Preserve header expansion data? */

  loop_type = info->options & IPATCH_DLS2_SAMPLE_LOOP_MASK;
  ipatch_file_buf_write_u32 (riff->handle, (loop_type != IPATCH_SAMPLE_LOOP_NONE)
			     ? 1 : 0); /* loop count */

  if (loop_type != IPATCH_SAMPLE_LOOP_NONE)
    {
      /* write loop structure size */
      ipatch_file_buf_write_u32 (riff->handle, IPATCH_DLS_WSMP_LOOP_SIZE);

      if (loop_type == IPATCH_SAMPLE_LOOP_RELEASE)
	saveloop_type = IPATCH_DLS_WSMP_LOOP_RELEASE;
      else saveloop_type = IPATCH_DLS_WSMP_LOOP_FORWARD; /* default */
      ipatch_file_buf_write_u32 (riff->handle, saveloop_type);

      ipatch_file_buf_write_u32 (riff->handle, info->loop_start);
      ipatch_file_buf_write_u32 (riff->handle, info->loop_end - info->loop_start);
    }

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

static gboolean
dls_write_sample_format (IpatchDLSWriter *writer, IpatchDLS2Sample *sample,
			 GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  int format;
  int frame_size;
  int channels;

  g_return_val_if_fail (sample->sample_data != NULL, FALSE);

  /* get format from primary store */
  format = ipatch_sample_get_format (IPATCH_SAMPLE (sample->sample_data));
  frame_size = ipatch_sample_format_size (format);

  channels = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format);
  g_return_val_if_fail (channels == 1 || channels == 2, FALSE);

  /* for right now we just support PCM
     FIXME - What about floating point?? */
  ipatch_file_buf_write_u16 (riff->handle, IPATCH_RIFF_WAVE_FMT_PCM);

  ipatch_file_buf_write_u16 (riff->handle, channels); /* write channels */
  ipatch_file_buf_write_u32 (riff->handle, sample->rate);

  /* write dwAvgBytesPerSec and wBlockAlign fields */
  ipatch_file_buf_write_u32 (riff->handle, frame_size * sample->rate);
  ipatch_file_buf_write_u16 (riff->handle, frame_size);

  /* bit width of audio */
  ipatch_file_buf_write_u16 (riff->handle, ipatch_sample_format_width (format) * 8);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* write the pool table header and reserve entries for the total number of
   samples, the entries are fixed up later after the wave pool has been
   written */
static gboolean
dls_reserve_pool_table (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  guint count;

  /* write pool table header size */
  if (!ipatch_file_write_u32 (riff->handle, IPATCH_DLS_PTBL_HEADER_SIZE, err))
    return (FALSE);

  /* FIXME: Preserve header expansion? */

  count = writer->sample_count;
  if (!ipatch_file_write_u32 (riff->handle, count, err)) /* write sample cue count */
    return (FALSE);

  /* get position of pool table cues for later fixup */
  writer->ptbl_pos = ipatch_file_get_position (riff->handle);

  /* reserve the pool cues (one for each sample) */
  if (!ipatch_file_seek (riff->handle, writer->sample_count * IPATCH_DLS_POOLCUE_SIZE,
			 G_SEEK_CUR, err))
    return (FALSE);

  return (TRUE);
}

static gboolean
dls_fixup_pool_table (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  guint32 retpos;

  retpos = ipatch_file_get_position (riff->handle);

  /* seek back to pool cue table */
  if (!ipatch_file_seek (riff->handle, writer->ptbl_pos, G_SEEK_SET, err))
    return (FALSE);

  /* do endian swap on cue offsets if needed */
  if (IPATCH_RIFF_NEED_SWAP (riff))
    {
      guint32 *cuep, *stop;

      cuep = writer->sample_ofstbl;
      stop = cuep + writer->sample_count;
      for (; cuep < stop; cuep++)
	*cuep = GUINT32_SWAP_LE_BE (*cuep);
    }

  /* write the table */
  if (!ipatch_file_write (riff->handle, writer->sample_ofstbl,
			  writer->sample_count * 4, err)) return (FALSE);

  /* seek back to original position */
  if (!ipatch_file_seek (riff->handle, retpos, G_SEEK_SET, err)) return (FALSE);

  return (TRUE);
}

static gboolean
dls_write_wave_pool (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchDLS2Sample *sample;
  IpatchSampleHandle sample_handle;
  IpatchIter iter;
  gpointer buf;
  guint samsize, read_size, ofs;
  guint index;
  guint32 start;
  int dest_fmt, fmt_size;

  /* start position of wave pool chunk */
  start = ipatch_file_get_position (riff->handle);

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->dls), &iter,
			      IPATCH_TYPE_DLS2_SAMPLE);
  sample = ipatch_dls2_sample_first (&iter);
  for (index = 0; sample; index++, sample = ipatch_dls2_sample_next (&iter))
    {
      /* store offset to WAVE list chunk for later pool table fixup */
      writer->sample_ofstbl[index] = ipatch_file_get_position (riff->handle) - start;

      /* <WAVE> - Wave list chunk */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_WAVE, err))
	goto err;

      /* write DLSID if its set */
      if (sample->dlid)
	if (!dls_write_dlid (writer, sample->dlid, err)) return (FALSE);

      /* <FMT> - Sample format chunk */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_FMT, err))
	goto err;
      if (!dls_write_sample_format (writer, sample, err)) goto err;
      if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
      /* </FMT> */

      /* <INFO> - Sample text info */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_DLS_FOURCC_INFO, err))
	goto err;

      if (!writer->is_gig)
	{
	  if (!dls_write_info (writer, sample->info, err)) goto err;
	}
      else if (!gig_write_name_info (writer, sample->info, err)) goto err;

      if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
      /* </INFO> */

      if (!writer->is_gig && sample->sample_info)
	{
	  /* <WSMP> - Wave sample info chunk */
	  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WSMP, err))
	    goto err;
	  if (!dls_write_sample_info (writer, sample->sample_info, err))
	    goto err;
	  if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
	  /* </WSMP> */
	}

      /* <DATA> - Sample data */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_DATA, err))
	goto err;

      /* store position to sample data for possible ipatch_dls_writer_create_stores() call */
      writer->sample_postbl[index] = ipatch_file_get_position (riff->handle);

      samsize = ipatch_sample_get_size (IPATCH_SAMPLE (sample->sample_data), NULL);

      dest_fmt = ipatch_sample_get_format (IPATCH_SAMPLE (sample->sample_data));
      dest_fmt &= IPATCH_SAMPLE_WIDTH_MASK | IPATCH_SAMPLE_CHANNEL_MASK;
      dest_fmt |= IPATCH_SAMPLE_LENDIAN;

      /* write 8 or 16 bit data
	 FIXME - Support other bit widths? */
      if (IPATCH_SAMPLE_FORMAT_GET_WIDTH (dest_fmt) == IPATCH_SAMPLE_8BIT)
	dest_fmt |= IPATCH_SAMPLE_UNSIGNED;
      else if (IPATCH_SAMPLE_FORMAT_GET_WIDTH (dest_fmt) >= IPATCH_SAMPLE_16BIT)
	{
	  dest_fmt |= IPATCH_SAMPLE_SIGNED;
	  dest_fmt &= ~IPATCH_SAMPLE_WIDTH_MASK;
	  dest_fmt |= IPATCH_SAMPLE_16BIT;
	}

      /* frame size of dest format */
      fmt_size = ipatch_sample_format_size (dest_fmt);

      /* ++ Open sample data handle (set transform manually) */
      if (!ipatch_sample_handle_open (IPATCH_SAMPLE (sample->sample_data), &sample_handle, 'r',
                                      dest_fmt, IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
        goto err;

      read_size = ipatch_sample_handle_get_max_frames (&sample_handle);

      ofs = 0;
      while (ofs < samsize)
	{
	  if (samsize - ofs < read_size) /* check for last partial fragment */
	    read_size = samsize - ofs;

	  /* read and transform (if necessary) audio data from sample store */
	  if (!(buf = ipatch_sample_handle_read (&sample_handle, ofs, read_size, NULL, err)))
	    {
              ipatch_sample_handle_close (&sample_handle);      /* -- close sample handle */
	      goto err;
	    }

	  /* write sample data to DLS file */
	  if (!ipatch_file_write (riff->handle, buf, read_size * fmt_size, err))
	    {
              ipatch_sample_handle_close (&sample_handle);      /* -- close sample handle */
	      goto err;
	    }
	  ofs += read_size;
	}

      ipatch_sample_handle_close (&sample_handle);      /* -- close sample handle */

      if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
      /* </DATA> */

      if (writer->is_gig)
	{ /* <SMPL> - GigaSampler sample info chunk */
	  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_SMPL, err))
	    goto err;
	  if (!gig_write_sample_info (writer, sample->sample_info,
				      sample->rate, err))
	    goto err;
	  if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
	  /* </SMPL> */

	  /* <3GIX> - GigaSampler sample group number */
	  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3GIX, err))
	    goto err;
	  if (!ipatch_file_write_u32 (riff->handle,
				      IPATCH_GIG_SAMPLE (sample)->group_number,
				      err))
	    goto err;
	  if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
	  /* </3GIX> */
	}

      if (!ipatch_riff_close_chunk (riff, -1, err)) goto err;
      /* </WAVE> */
    }

  return (TRUE);

 err:
  return (FALSE);
}

static gboolean
dls_write_dlid (IpatchDLSWriter *writer, guint8 *dlid, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);

  /* FIXME - Generate a new DLSID if needed */

  /* <DLID> - DLSID chunk */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_DLID, err))
    return (FALSE);

  if (!ipatch_file_write (riff->handle, dlid, 16, err)) return (FALSE);

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </DLID> */

  return (TRUE);
}

static gboolean
gig_write_sub_regions (IpatchDLSWriter *writer, IpatchGigRegion *region,
		       GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchGigSubRegion *sub_region;
  int i;

  for (i=0; i < region->sub_region_count; i++)
    {
      /* <3EWL> - GigaSampler sub region list chunk */
      if (!ipatch_riff_write_list_chunk (riff, IPATCH_GIG_FOURCC_3EWL, err))
	return (FALSE);

      sub_region = region->sub_regions[i];

      /* <WSMP> - GigaSampler sample info chunk */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_DLS_FOURCC_WSMP, err))
	return (FALSE);
      if (!dls_write_sample_info (writer, sub_region->sample_info, err))
	return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </WSMP> */

      /* <3EWA> - GigaSampler effects chunk */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3EWA, err))
	return (FALSE);

      ipatch_gig_store_effects (riff->handle, &sub_region->effects);

      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3EWA> */

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3EWL> */
    }

  return (TRUE);
}

static gboolean
gig_write_dimension_names (IpatchDLSWriter *writer, IpatchGigRegion *region,
			   GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchGigDimension *dim;
  int i;

  for (i = 0; i < region->dimension_count; i++)
    {
      dim = region->dimensions[i];

      if (dim->name && *dim->name)
	{
	  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_SUB,
					IPATCH_FOURCC('n','a','m','0'+i), err))
	    return (FALSE);
	  if (!ipatch_file_write (riff->handle, dim->name,
				  strlen (dim->name) + 1, err))
	    return (FALSE);
	  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
	}
    }

  return (TRUE);
}

/* for now we just use the standard DLS sample info */
static gboolean
gig_write_sample_info (IpatchDLSWriter *writer, IpatchDLS2SampleInfo *info,
		       int rate, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);

  ipatch_file_buf_write_u32 (riff->handle, 0); /* manufacturer */
  ipatch_file_buf_write_u32 (riff->handle, 0); /* product */

  /* period of 1 sample in nanoseconds (1 / rate * 1,000,000,000) */
  ipatch_file_buf_write_u32 (riff->handle, 1000000000 / rate);

  ipatch_file_buf_write_u32 (riff->handle, info->root_note);

  /* SMPL chunk stores finetune as an unsigned 32bit fraction of a semitone,
     so 0x80000000 is 1/2 of a MIDI note. DLS on the other hand uses a
     16 bit signed relative pitch value (methinks this is
     rel_pitch = cents / 100 * 32768, but not sure!! FIXME!!) */
  ipatch_file_buf_write_u32 (riff->handle, 0);

  ipatch_file_buf_write_u32 (riff->handle, 0); /* SMPTE format */
  ipatch_file_buf_write_u32 (riff->handle, 0); /* SMPTE offset */

  /* loop count */
  ipatch_file_buf_write_u32 (riff->handle, info->options == IPATCH_SAMPLE_LOOP_NONE
			     ? 0 : 1);

  ipatch_file_buf_write_u32 (riff->handle, 0); /* extra data size */


  /* loop fields are always written, even when no loop */


  ipatch_file_buf_write_u32 (riff->handle, 0); /* loop ID */

  /* FIXME - Is there a release WSMP loop type or other types? */
  ipatch_file_buf_write_u32 (riff->handle, 0); /* loop type - Normal */

  ipatch_file_buf_write_u32 (riff->handle, info->loop_start); /* loop start */
  ipatch_file_buf_write_u32 (riff->handle, info->loop_end); /* loop end */

  ipatch_file_buf_write_u32 (riff->handle, 0); /* loop sample fraction */
  ipatch_file_buf_write_u32 (riff->handle, 0); /* times to loop (0=inf) */


  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

static gboolean
gig_write_dimension_info (IpatchDLSWriter *writer, IpatchGigRegion *region,
			  GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchGigDimension *dimension;
  int count;
  guint sample_index;
  int i;

  count = region->sub_region_count;
  ipatch_file_buf_write_u32 (riff->handle, count); /* count of sub regions */

  for (i = 0; i < region->dimension_count; i++)
    {
      dimension = region->dimensions[i];

      /* type of dimension */
      ipatch_file_buf_write_u8 (riff->handle, dimension->type);

      /* split bit count */
      ipatch_file_buf_write_u8 (riff->handle, dimension->split_count);

      /* FIXME - 6 bytes of unknown data */
      ipatch_file_buf_zero (riff->handle, 6);
    }

  /* zero out remaining unused dimensions */
  i = (5 - region->dimension_count) * 8;
  if (i) ipatch_file_buf_zero (riff->handle, i);

  /* write sub region sample indexes */
  for (i = 0; i < region->sub_region_count; i++)
    {
      sample_index = GPOINTER_TO_UINT
	(g_hash_table_lookup (writer->sample_hash,
			      region->sub_regions[i]->sample));
      g_return_val_if_fail (sample_index != 0, FALSE);
      ipatch_file_buf_write_u32 (riff->handle, sample_index - 1);
    }

  /* fill remaining sample cue indexes with 0xFFFFFFFF */
  i = (32 - region->sub_region_count) * 4;
  if (i) ipatch_file_buf_memset (riff->handle, 0xFF, i);

  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* write GigaSampler 3gri chunk (sample group names) */
static gboolean
gig_write_group_names (IpatchDLSWriter *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchGig *gig = IPATCH_GIG (writer->dls);
  char name[IPATCH_GIG_3GNM_SIZE];
  GSList *p;

  /* <3gri> - GigaSampler 3gri list chunk */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_GIG_FOURCC_3GRI, err))
    return (FALSE);

  /* <3gnl> - GigaSampler 3dnl list chunk */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_GIG_FOURCC_3GNL, err))
    return (FALSE);
  
  for (p = gig->group_names; p; p = p->next)
    {
      /* <3gnm> - GigaSampler sample info chunk */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_GIG_FOURCC_3GNM, err))
	return (FALSE);

      /* write the sample group name */
      strncpy (name, (char *)(p->data), IPATCH_GIG_3GNM_SIZE);
      if (!ipatch_file_write (riff->handle, name, IPATCH_GIG_3GNM_SIZE, err))
	return (FALSE);

      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
      /* </3gnm> */
    }

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </3gnl> */

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </3gri> */

  return (TRUE);
}
