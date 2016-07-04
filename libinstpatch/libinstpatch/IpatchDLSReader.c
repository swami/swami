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
 * SECTION: IpatchDLSReader
 * @short_description: DLS version 2 file reader
 * @see_also: #IpatchDLS
 * @stability: Stable
 *
 * Parses a DLS file into an object tree (#IpatchDLS).
 */
#include <glib.h>
#include <string.h>

#include "IpatchDLSReader.h"
#include "IpatchDLSFile.h"
#include "IpatchDLSFile_priv.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLS2Sample.h"
#include "IpatchGig.h"
#include "IpatchGigEffects.h"
#include "IpatchGigFile.h"
#include "IpatchGigFile_priv.h"
#include "IpatchGigInst.h"
#include "IpatchGigRegion.h"
#include "IpatchGigSample.h"
#include "IpatchSampleStoreFile.h"
#include "IpatchSample.h"
#include "ipatch_priv.h"
#include "i18n.h"

#define DEBUG_DLS  g_warning
#define DEBUG_DLS_UNKNOWN_CHUNK(parser, level)

/* #define DEBUG_DLS_UNKNOWN_CHUNK(parser, level) \
  g_warning (ipatch_riff_message_detail (parser, -1, "Unknown chunk")) */


/* a slightly sane cap on the max size of an INFO string
   (1MB comment anyone?) */
#define IPATCH_DLS_MAX_INFO_SIZE   (1024 * 1024)

/* a size to use for buffered reads or variable length structures
   - allocated on stack */
#define VARCHUNK_BUF_SIZE 1024

#define DLS_ERROR_MSG "DLS Reader error: %s"

#define SET_SIZE_ERROR(parser, level, err) \
  g_set_error (err, IPATCH_RIFF_ERROR, IPATCH_RIFF_ERROR_SIZE_MISMATCH,\
	       _(DLS_ERROR_MSG), \
	       ipatch_riff_message_detail(parser, -1, "Unexpected chunk size"))

#define SET_DATA_ERROR(parser, level, err) \
  g_set_error (err, IPATCH_RIFF_ERROR, IPATCH_RIFF_ERROR_INVALID_DATA,\
	       _(DLS_ERROR_MSG), \
	       ipatch_riff_message_detail (parser, -1, "Invalid data"))

static void ipatch_dls_reader_class_init (IpatchDLSReaderClass *klass);
static void ipatch_dls_reader_init (IpatchDLSReader *reader);
static void ipatch_dls_reader_finalize (GObject *object);

static gboolean ipatch_dls_reader_start (IpatchDLSReader *reader, GError **err);
static gboolean ipatch_dls_reader_fixup (IpatchDLSReader *reader, GError **err);
static void ipatch_dls_nullify_fixups (IpatchDLSReader *reader);
static gboolean assert_loading_gig (IpatchDLSReader *reader, GError **err);

static gboolean ipatch_dls_reader_load_level_0 (IpatchDLSReader *reader,
					 GError **err);
static gboolean ipatch_dls_reader_load_inst_list (IpatchDLSReader *reader,
					   GError **err);
static gboolean ipatch_dls_reader_load_region_list (IpatchDLSReader *reader,
				      IpatchDLS2Inst *inst, GError **err);
static gboolean ipatch_gig_reader_load_region_list (IpatchDLSReader *reader,
					     IpatchGigInst *giginst,
					     GError **err);
static gboolean ipatch_dls_reader_load_art_list (IpatchDLSReader *reader,
					  GSList **conn_list, GError **err);
static gboolean ipatch_dls_reader_load_wave_pool (IpatchDLSReader *reader,
					   GError **err);
static gboolean ipatch_gig_reader_load_sub_regions (IpatchDLSReader *reader,
					     IpatchGigRegion *region,
					     GError **err);
static gboolean ipatch_gig_reader_load_inst_lart (IpatchDLSReader *reader,
					   IpatchGigInst *inst, GError **err);

static gboolean ipatch_dls_load_info (IpatchRiff *riff, IpatchDLS2Info **info,
			       GError **err);
static gboolean ipatch_dls_load_region_header (IpatchRiff *riff,
					IpatchDLS2Region *region, GError **err);
static gboolean ipatch_gig_load_region_header (IpatchRiff *riff,
				        IpatchGigRegion *region, GError **err);
static gboolean ipatch_dls_load_wave_link (IpatchRiff *riff, IpatchDLS2Region *region,
				    GError **err);
static gboolean ipatch_dls_load_sample_info (IpatchRiff *riff,
                                      IpatchDLS2SampleInfo *info,
				      GError **err);
static gboolean ipatch_dls_load_connection (IpatchRiff *riff,
				     GSList **conn_list,
				     GError **err);
static gboolean ipatch_dls_load_sample_format (IpatchRiff *riff,
					IpatchDLS2Sample *sample,
					int *bitwidth, int *channels,
					GError **err);
static guint32 *ipatch_dls_load_pool_table (IpatchRiff *riff, guint *size, GError **err);
static gboolean ipatch_dls_load_dlid (IpatchRiff *riff, guint8 *dlid, GError **err);

static gboolean ipatch_gig_load_sample_info (IpatchRiff *riff,
				      IpatchDLS2SampleInfo *info,
				      GError **err);
static gboolean ipatch_gig_load_dimension_info (IpatchRiff *riff,
					 IpatchGigRegion *region,
					 GError **err);
static gboolean ipatch_gig_load_dimension_names (IpatchRiff *riff,
					  IpatchGigRegion *region,
					  GError **err);
static gboolean ipatch_gig_load_group_names (IpatchRiff *riff,
				      GSList **name_list, GError **err);

/* global variables */

static gpointer parent_class = NULL;


/**
 * ipatch_dls_reader_error_quark: (skip)
 */
GQuark
ipatch_dls_reader_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("DLSReader-error-quark");

  return (q);
}

GType
ipatch_dls_reader_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info = {
      sizeof (IpatchDLSReaderClass), NULL, NULL,
      (GClassInitFunc)ipatch_dls_reader_class_init, NULL, NULL,
      sizeof (IpatchDLSReader), 0,
      (GInstanceInitFunc)ipatch_dls_reader_init,
    };

    obj_type = g_type_register_static (IPATCH_TYPE_RIFF,
				       "IpatchDLSReader", &obj_info, 0);
  }

  return (obj_type);
}

static void
ipatch_dls_reader_class_init (IpatchDLSReaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = ipatch_dls_reader_finalize;
}

static void
ipatch_dls_reader_init (IpatchDLSReader *reader)
{
  reader->dls = NULL;
  reader->needs_fixup = TRUE;
  reader->is_gig = FALSE;
  reader->wave_hash = g_hash_table_new (NULL, NULL);
}

static void
ipatch_dls_reader_finalize (GObject *object)
{
  IpatchDLSReader *reader = IPATCH_DLS_READER (object);

  /* fixup before destroying so bad things don't happen */
  if (reader->needs_fixup)
    ipatch_dls_nullify_fixups (reader);

  /* unref the dls object */
  if (reader->dls)
    {
      g_object_unref (reader->dls);
      reader->dls = NULL;
    }

  g_hash_table_destroy (reader->wave_hash);
  reader->wave_hash = NULL;

  g_free (reader->pool_table);
  reader->pool_table = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * ipatch_dls_reader_new:
 * @handle: DLS file handle to parse or %NULL to set later
 *
 * Create a new DLS file reader
 *
 * Returns: The new DLS reader
 */
IpatchDLSReader *
ipatch_dls_reader_new (IpatchFileHandle *handle)
{
  IpatchDLSReader *reader;

  g_return_val_if_fail (!handle || IPATCH_IS_DLS_FILE (handle->file), NULL);

  reader = g_object_new (IPATCH_TYPE_DLS_READER, NULL);
  if (handle) ipatch_riff_set_file_handle (IPATCH_RIFF (reader), handle);

  return (reader);
}

/**
 * ipatch_dls_reader_load:
 * @reader: DLS reader object
 * @err: Location to store error info or %NULL
 *
 * Load a DLS file.
 *
 * Returns: (transfer full): New DLS object with refcount of 1.
 */
IpatchDLS2 *
ipatch_dls_reader_load (IpatchDLSReader *reader, GError **err)
{
  IpatchRiff *riff;
  GError *local_err = NULL;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), NULL);
  riff = IPATCH_RIFF (reader);
  g_return_val_if_fail (IPATCH_IS_FILE_HANDLE (riff->handle), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

 restart: /* to restart parsing if a DLS file turns out to be GigaSampler */

  if (!ipatch_dls_reader_start (reader, err)) return (NULL);
  if (!ipatch_dls_reader_load_level_0 (reader, &local_err))
    {
      /* what was thought to be a DLS file turned out to be GigaSampler? */
      if (local_err && local_err->domain == IPATCH_DLS_READER_ERROR
	  && local_err->code == IPATCH_DLS_READER_ERROR_GIG)
	{
	  g_clear_error (&local_err);

	  /* seek back to beginning of file */
	  if (!ipatch_file_seek (riff->handle, 0, G_SEEK_SET, err))
	    return (FALSE);

	  reader->is_gig = TRUE;
	  g_object_unref (reader->dls);
	  reader->dls = NULL;
	  goto restart;		/* restart in GigaSampler mode */
	}

      if (local_err) g_propagate_error (err, local_err);
      return (NULL);
    }

  if (!ipatch_dls_reader_fixup (reader, err)) return (NULL);

  /* ++ ref for caller, finalize() will remove reader's ref */
  return (g_object_ref (reader->dls));
}

/**
 * ipatch_dls_reader_start:
 * @reader: DLS/Gig reader
 * @err: Location to store error info or %NULL
 *
 * Starts parsing a DLS/Gig file. This function only needs to be
 * called if using an IpatchDLSReader without ipatch_dls_load()
 * (custom readers).  The file object of the @reader must be set
 * before calling this function.  Loads the first "DLS" RIFF chunk to
 * verify we are loading a DLS file and sets other internal variables.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
ipatch_dls_reader_start (IpatchDLSReader *reader, GError **err)
{
  IpatchRiff *riff;
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  riff = IPATCH_RIFF (reader);
  g_return_val_if_fail (IPATCH_IS_FILE_HANDLE (riff->handle), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* are we parsing a GigaSampler file? */
  if (IPATCH_IS_GIG_FILE (riff->handle->file)) reader->is_gig = TRUE;

  /* start parsing */
  if (!(chunk = ipatch_riff_start_read (riff, err)))
    return (FALSE);

  if (chunk->id != IPATCH_DLS_FOURCC_DLS)
    {
      g_set_error (err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_UNEXPECTED_ID,
		   _("Not a DLS file (RIFF id = '%4s')"), chunk->idstr);
      return (FALSE);
    }

  /* ++ ref new object */
  if (reader->is_gig) reader->dls = IPATCH_DLS2 (ipatch_gig_new ());
  else reader->dls = ipatch_dls2_new ();

  ipatch_dls2_set_file (reader->dls, IPATCH_DLS_FILE (riff->handle->file));

  return (TRUE);
}

/**
 * ipatch_dls_reader_fixup:
 * @reader: DLS/Gig reader
 * @err: Location to store error info or %NULL
 *
 * Fixup sample pointers in DLS/GigaSampler regions of the DLS object in
 * @reader. The sample pointers should be sample pool indexes previously
 * stored by ipatch_dls_load_wave_link() or ipatch_gig_load_dimension_info().
 * The pool table must also have been previously loaded for this to make
 * any sense.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
ipatch_dls_reader_fixup (IpatchDLSReader *reader, GError **err)
{
  GHashTable *fixup_hash;
  IpatchDLS2Sample *sample;
  IpatchDLS2Inst *inst;
  IpatchDLS2Region *region;
  IpatchGigRegion *gig_region;
  IpatchGigSubRegion *sub_region;
  IpatchIter inst_iter, region_iter;
  int i;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (!reader->needs_fixup) return (TRUE); /* already fixed up? */

  fixup_hash = g_hash_table_new (NULL, NULL);

  /* create pool table index -> sample hash */
  i = 0;
  while (i < reader->pool_table_size)
    {
      sample = g_hash_table_lookup (reader->wave_hash,
				    GINT_TO_POINTER (reader->pool_table[i]));
      if (sample)
	g_hash_table_insert (fixup_hash, GINT_TO_POINTER (i), sample);
      else g_warning ("Invalid wave pool entry (index=%d)", i);
      i++;
    }

  if (!reader->is_gig)		/* regular DLS file? (not GigaSampler) */
    {
      /* fixup DLS region sample indexes */
      ipatch_container_init_iter ((IpatchContainer *)(reader->dls), &inst_iter,
				  IPATCH_TYPE_DLS2_INST);
      inst = ipatch_dls2_inst_first (&inst_iter);
      while (inst)		/* loop over instruments */
	{
	  ipatch_container_init_iter ((IpatchContainer *)inst, &region_iter,
				      IPATCH_TYPE_DLS2_REGION);
	  region = ipatch_dls2_region_first (&region_iter);
	  while (region)	/* loop over instrument regions */
	    {
	      /* region->sample is pool index (ipatch_dls_load_wave_link) */
	      sample = g_hash_table_lookup (fixup_hash, region->sample);
	      if (!sample)		/* fixup failed? */
		{
		  char *name;
		  g_object_get (inst, "name", &name, NULL);
		  g_warning ("Failed to fixup sample for inst '%s' (index=%d)",
			     name ? name : "<unnamed>",
			     GPOINTER_TO_UINT (region->sample));
		  g_free (name);
		  region->sample = NULL;
		  ipatch_container_remove (IPATCH_CONTAINER (inst),
					   IPATCH_ITEM (region));
		}
	      else
		{
		  region->sample = NULL;
		  ipatch_dls2_region_set_sample (region, sample);
		}
	      region = ipatch_dls2_region_next (&region_iter);
	    } /* while (region) region loop */
	  inst = ipatch_dls2_inst_next (&inst_iter);
	} /* while (inst) instrument loop */
    }
  else /* reader->is_gig - fixup GigaSampler sub region sample indexes */
    {
      ipatch_container_init_iter ((IpatchContainer *)(reader->dls), &inst_iter,
				  IPATCH_TYPE_GIG_INST);
      inst = ipatch_dls2_inst_first (&inst_iter);
      while (inst)		/* loop over instruments */
	{
	  ipatch_container_init_iter ((IpatchContainer *)inst, &region_iter,
				      IPATCH_TYPE_GIG_REGION);
	  gig_region = ipatch_gig_region_first (&region_iter);
	  while (gig_region)	/* loop over instrument regions */
	    {
	      for (i = 0; i < gig_region->sub_region_count; i++)
		{
		  sub_region = gig_region->sub_regions[i];

		  /* fixup sample index (see ipatch_gig_load_dimension_info) */
		  sample = g_hash_table_lookup (fixup_hash,
						sub_region->sample);
		  if (!sample)		/* fixup failed? */
		    {
		      char *name;
		      g_object_get (inst, "name", &name, NULL);
		      g_warning ("Failed to fixup sample for inst"
				 " '%s' (index=%d)", name ? name : "<unnamed>",
				 GPOINTER_TO_UINT (sub_region->sample));
		      g_free (name);
		      sub_region->sample = NULL;
		    }
		  else
		    {
		      sub_region->sample = NULL;
		      ipatch_gig_sub_region_set_sample (sub_region,
						IPATCH_GIG_SAMPLE (sample));
		    }
		} /* for each sub region */
	      gig_region = ipatch_gig_region_next (&region_iter);
	    } /* while (gig_region) region loop */
	  inst = ipatch_dls2_inst_next (&inst_iter);
	} /* while (inst) instrument loop */
    } /* else - GigaSampler file */

  g_hash_table_destroy (fixup_hash); /* destroy fixup hash */
  reader->needs_fixup = FALSE;

  return (TRUE);
}

/* sample index fixups have to be cleared on error, so problems don't occur
   when DLS object is finalized */
static void
ipatch_dls_nullify_fixups (IpatchDLSReader *reader)
{
  IpatchDLS2Inst *inst;
  IpatchDLS2Region *region;
  IpatchIter inst_iter, region_iter;
  int i;

  ipatch_container_init_iter ((IpatchContainer *)(reader->dls), &inst_iter,
			      reader->is_gig ? IPATCH_TYPE_GIG_INST
			      : IPATCH_TYPE_DLS2_INST);
  inst = ipatch_dls2_inst_first (&inst_iter);
  while (inst)			/* loop over instruments */
    {
      ipatch_container_init_iter ((IpatchContainer *)inst, &region_iter,
				  reader->is_gig ? IPATCH_TYPE_GIG_REGION
				  : IPATCH_TYPE_DLS2_REGION);
      region = ipatch_dls2_region_first (&region_iter);
      while (region)		/* loop over instrument regions */
	{
	  region->sample = NULL;

	  if (reader->is_gig) /* NULLify GigaSampler sample indexes */
	    {
	      IpatchGigRegion *gig_region = IPATCH_GIG_REGION (region);

	      for (i = 0; i < gig_region->sub_region_count; i++)
		gig_region->sub_regions[i]->sample = NULL;
	    }

	  region = ipatch_dls2_region_next (&region_iter);
	}
      inst = ipatch_dls2_inst_next (&inst_iter);
    }
}

/* called before GigaSampler related stuff to ensure that we are already in
   GigaSampler mode, if not then an error is returned which signals the
   ipatch_dls_load() routine to restart in GigaSampler mode */
static gboolean
assert_loading_gig (IpatchDLSReader *reader, GError **err)
{
  if (reader->is_gig) return (TRUE);

  g_set_error (err, IPATCH_DLS_READER_ERROR, IPATCH_DLS_READER_ERROR_GIG,
	       "GigaSampler file detected, restart loading in gig mode");
  return (FALSE);
}

/**
 * ipatch_dls_reader_load_level_0:
 * @reader: DLS/Gig reader
 * @err: Location to store error info or %NULL
 *
 * Load the top level DLS chunk of a DLS or GigaSampler file (essentially
 * the entire file except the toplevel chunk itself).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_reader_load_level_0 (IpatchDLSReader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  GError *local_err = NULL;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST)
	{
	  switch (chunk->id)
	    {
	    case IPATCH_DLS_FOURCC_LINS: /* instrument list */
	      if (!ipatch_dls_reader_load_inst_list (reader, err))
		return (FALSE);
	      break;
	    case IPATCH_DLS_FOURCC_WVPL: /* wave pool list (sample data) */
	    case IPATCH_DLS_FOURCC_DWPL: /* WTF? - Seen in some DLS1 files */
	      if (!ipatch_dls_reader_load_wave_pool (reader, err))
		return (FALSE);
	      break;
	    case IPATCH_DLS_FOURCC_INFO: /* toplevel INFO */
	      if (!ipatch_dls_load_info (riff, &reader->dls->info, err))
		return (FALSE);
	      break;
	    case IPATCH_GIG_FOURCC_3GRI:
	      if (!assert_loading_gig (reader, err)) return (FALSE);
	      if (!ipatch_gig_load_group_names (riff,
				&(IPATCH_GIG (reader->dls)->group_names), err))
		return (FALSE);
	      break;
	    default:
	      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
	      break;
	    }
	}
      else			/* a sub chunk */
	{
	  switch (chunk->id)
	    {
	    case IPATCH_DLS_FOURCC_CDL: /* toplevel conditional chunk */
	      DEBUG_DLS ("Toplevel DLS CDL chunk!\n");
	      break;
	    case IPATCH_DLS_FOURCC_VERS: /* file version chunk */
	      if (chunk->size != IPATCH_DLS_VERS_SIZE)
		{
		  SET_SIZE_ERROR (riff, -1, err);
		  return (FALSE);
		}

	      if (!ipatch_file_buf_load (riff->handle, IPATCH_DLS_VERS_SIZE, err))
		return (FALSE);

	      reader->dls->ms_version = ipatch_file_buf_read_u32 (riff->handle);
	      reader->dls->ls_version = ipatch_file_buf_read_u32 (riff->handle);

	      ipatch_item_set_flags (IPATCH_ITEM (reader->dls),
				     IPATCH_DLS2_VERSION_SET);
	      break;
	    case IPATCH_DLS_FOURCC_DLID: /* globally unique identifier */
	      if (!reader->dls->dlid) reader->dls->dlid = g_malloc (16);
	      if (!ipatch_dls_load_dlid (riff, reader->dls->dlid, err))
		return (FALSE);
	      break;
	    case IPATCH_DLS_FOURCC_COLH: /* collection header (inst count) */
	      /* we don't care since instruments are dynamically loaded */
	      break;
	    case IPATCH_DLS_FOURCC_PTBL: /* pool table (sample mappings) */
	      reader->pool_table = ipatch_dls_load_pool_table
		(riff, &reader->pool_table_size, &local_err);

	      if (local_err)
		{
		  g_propagate_error (err, local_err);
		  return (FALSE);
		}
	      break;
	    case IPATCH_GIG_FOURCC_EINF: /* FIXME - unknown */
	      break;
	    default:
	      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
	      break;
	    }
	}
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_dls_reader_load_inst_list:
 * @reader: DLS/Gig reader
 * @err: Location to store error info or %NULL
 *
 * Loads DLS or GigaSampler instrument list from the current position in
 * the file assigned to @reader.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_reader_load_inst_list (IpatchDLSReader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  IpatchIter iter;
  guint32 uint;
  gboolean retval;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* initialize iterator to instrument list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->dls), &iter,
			      reader->is_gig ? IPATCH_TYPE_GIG_INST
			      : IPATCH_TYPE_DLS2_INST);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST && /* an INS list? */
	  chunk->id == IPATCH_DLS_FOURCC_INS)
	{
	  IpatchDLS2Inst *inst;

	  /* ++ ref new instrument and append it in DLS2/Gig object */
	  if (reader->is_gig) inst = IPATCH_DLS2_INST (ipatch_gig_inst_new ());
	  else inst = ipatch_dls2_inst_new ();
	  ipatch_container_insert_iter ((IpatchContainer *)(reader->dls),
					(IpatchItem *)inst, &iter);
	  g_object_unref (inst); /* -- unref new instrument (parented) */

	  while ((chunk = ipatch_riff_read_chunk (riff, err)))
	    {
	      if (chunk->type == IPATCH_RIFF_CHUNK_LIST)
		{
		  switch (chunk->id) /* list chunk? */
		    {
		    case IPATCH_DLS_FOURCC_LRGN: /* region list */
		      if (!reader->is_gig)
			retval = ipatch_dls_reader_load_region_list (reader,
								     inst, err);
		      else
			retval = ipatch_gig_reader_load_region_list (reader,
						  IPATCH_GIG_INST (inst), err);
		      if (!retval) return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_LART: /* DLS1 articulator list */
		      if (reader->is_gig)
			{	/* load GigaSampler 3ewg chunk */
			  if (!ipatch_gig_reader_load_inst_lart (reader,
						IPATCH_GIG_INST (inst), err))
			    return (FALSE);

			  break;
			}
		      /* fall through */
		    case IPATCH_DLS_FOURCC_LAR2: /* DLS2 articulator list */
		      if (!ipatch_dls_reader_load_art_list (reader,
							    &inst->conns, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_INFO: /* instrument INFO */
		      if (!ipatch_dls_load_info (riff, &inst->info, err))
			return (FALSE);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      else		/* sub chunk */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_INSH: /* instrument header */
		      if (chunk->size != IPATCH_DLS_INSH_SIZE)
			{
			  SET_SIZE_ERROR (riff, -1, err);
			  return (FALSE);
			}

		      if (!ipatch_file_buf_load (riff->handle, chunk->size, err))
			return (FALSE);

		      /* we ignore the region count */
		      ipatch_file_buf_skip (riff->handle, 4);

		      uint = ipatch_file_buf_read_u32 (riff->handle);
		      inst->bank = uint & IPATCH_DLS_INSH_BANK_MASK;

		      if (uint & IPATCH_DLS_INSH_BANK_PERCUSSION)
			ipatch_item_set_flags (inst, IPATCH_DLS2_INST_PERCUSSION);

		      inst->program = ipatch_file_buf_read_u32 (riff->handle);
		      break;
		    case IPATCH_DLS_FOURCC_DLID: /* globally unique ID */
		      if (!inst->dlid) inst->dlid = g_malloc (16);
		      if (!ipatch_dls_load_dlid (riff, inst->dlid, err))
			return (FALSE);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      
	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	    }

	  inst->conns = g_slist_reverse (inst->conns);

	  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);
	}

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_dls_reader_load_region_list:
 * @reader: DLS reader
 * @inst: DLS instrument to fill with loaded regions
 * @err: Location to store error info or %NULL
 *
 * Loads DLS region list into an @inst object from the current
 * position in the file assigned to @reader.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_reader_load_region_list (IpatchDLSReader *reader,
				    IpatchDLS2Inst *inst, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchIter iter;
  IpatchRiffChunk *chunk;
  IpatchDLS2Region *region;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (IPATCH_IS_DLS2_INST (inst), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* initialize iterator to DLS2 region list */
  ipatch_container_init_iter (IPATCH_CONTAINER (inst), &iter,
			      IPATCH_TYPE_DLS2_REGION);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST
	  && (chunk->id == IPATCH_DLS_FOURCC_RGN
	      || chunk->id == IPATCH_DLS_FOURCC_RGN2))
	{
	  region = ipatch_dls2_region_new ();
	  ipatch_container_insert_iter ((IpatchContainer *)inst,
					(IpatchItem *)region, &iter);
	  g_object_unref (region); /* -- unref new region (parented) */

	  while ((chunk = ipatch_riff_read_chunk (riff, err)))
	    {
	      if (chunk->type == IPATCH_RIFF_CHUNK_LIST) /* list chunk? */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_LART:
		    case IPATCH_DLS_FOURCC_LAR2:
		      if (!ipatch_dls_reader_load_art_list (reader,
			  &region->conns, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_INFO:
		      if (!ipatch_dls_load_info (riff, &region->info, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3PRG: /* gig sub region list */
		      assert_loading_gig (reader, err);
		      return (FALSE);
		    case IPATCH_GIG_FOURCC_3DNL: /* dimension names */
		      assert_loading_gig (reader, err);
		      return (FALSE);
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      else		/* sub chunk */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_RGNH:
		      if (!ipatch_dls_load_region_header (riff, region, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_WLNK:
		      if (!ipatch_dls_load_wave_link (riff, region, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_WSMP:
		      if (!region->sample_info) /* for sanity */
			region->sample_info = ipatch_dls2_sample_info_new ();

		      if (!ipatch_dls_load_sample_info (riff,
						  region->sample_info, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_CDL:
		      DEBUG_DLS ("Region CDL chunk!\n");
		      break;
		    case IPATCH_GIG_FOURCC_3LNK: /* Gig dimension info */
		      assert_loading_gig (reader, err);
		      return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3DDP: /* FIXME - what is it? */
		      assert_loading_gig (reader, err);
		      return (FALSE);
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}

	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	    }
	  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);
	}
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_gig_reader_load_region_list:
 * @reader: DLS/Gig reader
 * @giginst: Gig instrument to fill with loaded regions
 * @err: Location to store error info or %NULL
 *
 * Loads GigaSampler region list into an @inst object from the current
 * position in the file assigned to @reader.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_gig_reader_load_region_list (IpatchDLSReader *reader,
				    IpatchGigInst *giginst, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchIter iter;
  IpatchRiffChunk *chunk;
  IpatchDLS2Inst *inst;
  IpatchGigRegion *region;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_INST (giginst), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  inst = IPATCH_DLS2_INST (giginst);

  /* initialize iterator to Gig region list */
  ipatch_container_init_iter (IPATCH_CONTAINER (inst), &iter,
			      IPATCH_TYPE_GIG_REGION);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST
	  && (chunk->id == IPATCH_DLS_FOURCC_RGN
	      || chunk->id == IPATCH_DLS_FOURCC_RGN2))
	{
	  region = ipatch_gig_region_new ();
	  ipatch_container_insert_iter ((IpatchContainer *)inst,
					(IpatchItem *)region, &iter);
	  g_object_unref (region); /* -- unref new region (parented) */

	  while ((chunk = ipatch_riff_read_chunk (riff, err)))
	    {
	      if (chunk->type == IPATCH_RIFF_CHUNK_LIST) /* list chunk? */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_INFO:
		      if (!ipatch_dls_load_info (riff, &region->info, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3PRG: /* gig sub region list */
		      if (!ipatch_gig_reader_load_sub_regions (reader,
							       region, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3DNL: /* dimension names */
		      if (!ipatch_gig_load_dimension_names (riff,
							    region, err))
			return (FALSE);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      else		/* sub chunk */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_RGNH:
		      if (!ipatch_gig_load_region_header (riff, region, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_WLNK:
		      /* ignore WLNK chunks with GigaSampler files */
		      break;
		    case IPATCH_DLS_FOURCC_WSMP:
		      /* ignore useless sample info for GigaSampler files */
		      break;
		    case IPATCH_DLS_FOURCC_CDL:
		      DEBUG_DLS ("Region CDL chunk!\n");
		      break;
		    case IPATCH_GIG_FOURCC_3LNK: /* dimension info */
		      if (!ipatch_gig_load_dimension_info (riff,
							   region, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3DDP: /* FIXME - what is it? */
		      if (chunk->size == IPATCH_GIG_3DDP_SIZE)
			if (!ipatch_file_read (riff->handle,
				&(IPATCH_GIG_REGION (region)->chunk_3ddp),
					       IPATCH_GIG_3DDP_SIZE, err))
			  return (FALSE);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}

	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	    }
	  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);
	}
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_dls_reader_load_art_list:
 * @reader: DLS/Gig reader
 * @conn_list: (out) (element-type IpatchDLS2Conn): Pointer to a list to populate 
 * @err: Location to store error info or %NULL
 *
 * Loads DLS or GigaSampler articulator list from the current position in
 * the file assigned to @reader.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_reader_load_art_list (IpatchDLSReader *reader, GSList **conn_list,
				 GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (conn_list != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB)
	{
	  switch (chunk->id)
	    {
	    case IPATCH_DLS_FOURCC_ART1:
	    case IPATCH_DLS_FOURCC_ART2:
	      if (!ipatch_dls_load_connection (riff, conn_list, err))
		return (FALSE);
	      break;
	    case IPATCH_DLS_FOURCC_CDL:
	      DEBUG_DLS ("Articulator CDL chunk!\n");
	      break;
	    default:
	      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
	      break;
	    }
	}

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_dls_reader_load_wave_pool:
 * @reader: DLS/Gig reader
 * @err: Location to store error info or %NULL
 *
 * Loads DLS or GigaSampler wave pool ("wvpl" chunk) from the current position
 * in the file assigned to @reader. Populates the @reader wave pool hash with
 * sample offsets for later fixup.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_reader_load_wave_pool (IpatchDLSReader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  IpatchDLS2Sample *sample;
  IpatchSampleData *sampledata;
  IpatchSample *store;
  IpatchIter iter;
  guint data_ofs, wave_ofs, data_size;
  int bitwidth;
  int channels;
  int format;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* initialize iterator to sample list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->dls), &iter,
			      IPATCH_TYPE_DLS2_SAMPLE);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST
	  && chunk->id == IPATCH_DLS_FOURCC_WAVE)
	{
	  /* offset to wave LIST header in wave pool chunk */
	  wave_ofs = chunk[-1].position
	    - (IPATCH_RIFF_LIST_HEADER_SIZE + IPATCH_RIFF_FOURCC_SIZE);

	  if (!reader->is_gig)
	    sample = ipatch_dls2_sample_new (); /* ++ ref and add new sample */
	  else			/* ++ ref new gig sample */
	    sample = IPATCH_DLS2_SAMPLE (ipatch_gig_sample_new ());

	  ipatch_container_insert_iter ((IpatchContainer *)(reader->dls),
					(IpatchItem *)sample, &iter);
	  g_object_unref (sample); /* -- unref new sample */

	  data_size = 0;
	  data_ofs = 0;
	  bitwidth = 0;
	  channels = 0;

	  while ((chunk = ipatch_riff_read_chunk (riff, err)))
	    {
	      if (chunk->type == IPATCH_RIFF_CHUNK_SUB)	/* sub chunk? */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_FMT:
		      if (!ipatch_dls_load_sample_format
			  (riff, sample, &bitwidth, &channels, err))
			return (FALSE);
		      break;
		    case IPATCH_DLS_FOURCC_DATA:
		      /* position in file to sample data */
		      data_ofs = ipatch_riff_get_position (riff);
		      data_size = chunk->size;
		      break;
		    case IPATCH_DLS_FOURCC_WSMP:
		      if (!sample->sample_info) /* for sanity */
			sample->sample_info = ipatch_dls2_sample_info_new ();
		      if (!ipatch_dls_load_sample_info (riff,
						sample->sample_info, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_SMPL: /* GigaSampler sample info */
		      /* Have seen in non gig files, just ignore it then */
		      if (!reader->is_gig) break;
		      if (!sample->sample_info) /* for sanity */
			sample->sample_info = ipatch_dls2_sample_info_new ();
		      if (!ipatch_gig_load_sample_info (riff,
						sample->sample_info, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3GIX:
		      if (!assert_loading_gig (reader, err)) return (FALSE);
		      if (chunk->size == IPATCH_GIG_3GIX_SIZE)
			{ /* Sample group # - FIXME - Is it really 32 bits? */
			  if (!ipatch_file_read_u32 (riff->handle,
				&(IPATCH_GIG_SAMPLE (sample)->group_number),
						     err))
			    return (FALSE);
			}
		      break;
		    case IPATCH_DLS_FOURCC_DLID:
		      if (!sample->dlid) sample->dlid = g_malloc (16);
		      if (!ipatch_dls_load_dlid (riff, sample->dlid, err))
			return (FALSE);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      else if (chunk->id == IPATCH_DLS_FOURCC_INFO) /* info list? */
		{
		  if (!ipatch_dls_load_info (riff, &sample->info, err))
		    return (FALSE);
		}
	      else DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);

	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	    } /* while() - wave LIST chunk */

	  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);

	  /* format and data chunks processed? */
	  if (bitwidth && channels && data_size && data_ofs)
	    {
	      /* convert data size to samples */
	      data_size = data_size / (bitwidth / 8) / channels;

	      /* add wave LIST chunk offset to wave hash (fixup later) */
	      g_hash_table_insert (reader->wave_hash,
				   GUINT_TO_POINTER (wave_ofs), sample);

	      format = (bitwidth == 8 ? IPATCH_SAMPLE_8BIT
			: IPATCH_SAMPLE_16BIT)
		| (channels == 2 ? IPATCH_SAMPLE_STEREO : IPATCH_SAMPLE_MONO)
		| (bitwidth == 8 ? IPATCH_SAMPLE_UNSIGNED : IPATCH_SAMPLE_SIGNED)
		| IPATCH_SAMPLE_LENDIAN;

              /* ++ ref new store */
	      store = ipatch_sample_store_file_new (riff->handle->file, data_ofs);
              g_object_set (store,
                            "sample-size", data_size,
                            "sample-format", format,
                            "sample-rate", sample->rate,
                            NULL);
	      sampledata = ipatch_sample_data_new ();   /* ++ ref sample data */
              ipatch_sample_data_add (sampledata, IPATCH_SAMPLE_STORE (store));
	      ipatch_dls2_sample_set_data (sample, sampledata);
              g_object_unref (store);	        /* -- unref store */
              g_object_unref (sampledata);      /* -- unref sampledata */
	    }
	  else
	    {		      /* !!! don't use sample after removed */
	      g_warning (_("Invalid sample"));
	      ipatch_item_remove ((IpatchItem *)sample);
	    }
	} /* if wave LIST chunk */

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    } /* while() - wave pool */

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_gig_reader_load_sub_regions:
 * @reader: Gig reader
 * @region: Gig region to load GigaSampler sub regions into
 * @err: Location to store error info or %NULL
 *
 * Loads GigaSampler sub regions ("3prg" chunk) from the current position
 * in the file assigned to @reader.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_gig_reader_load_sub_regions (IpatchDLSReader *reader,
				    IpatchGigRegion *region, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  IpatchGigSubRegion *sub_region;
  int sub_region_index = 0;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* loop in "3prg" chunk */
  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_LIST
	  && chunk->id == IPATCH_GIG_FOURCC_3EWL)
	{			/* loop in "3ewl" chunk */
	  if (sub_region_index >= region->sub_region_count)
	    {		      /* shouldn't happen, but just in case */
	      g_warning ("GigaSampler sub region count mismatch");
	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	      break;
	    }

	  sub_region = region->sub_regions[sub_region_index];

	  while ((chunk = ipatch_riff_read_chunk (riff, err)))
	    {
	      if (chunk->type == IPATCH_RIFF_CHUNK_SUB)	/* sub chunk? */
		{
		  switch (chunk->id)
		    {
		    case IPATCH_DLS_FOURCC_WSMP:
		      /* ipatch_gig_sub_region_set_sample_info_override would
			 probably be cleaner, but sub_region->sample might not
			 be valid (index that is fixed up later) */
		      if (!sub_region->sample_info)
			{
			  sub_region->sample_info
			    = ipatch_dls2_sample_info_new ();
			  ipatch_item_set_flags (sub_region,
			    IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE);
			}

		      if (!ipatch_dls_load_sample_info (riff,
					sub_region->sample_info, err))
			return (FALSE);
		      break;
		    case IPATCH_GIG_FOURCC_3EWA: /* GigaSampler effects */
		      if (chunk->size != IPATCH_GIG_3EWA_SIZE)
			{
			  SET_SIZE_ERROR (riff, -1, err);
			  return (FALSE);
			}

		      /* load effects chunk into buffer */
		      if (!ipatch_file_buf_load (riff->handle,
						 IPATCH_GIG_3EWA_SIZE, err))
			return (FALSE);

		      ipatch_gig_parse_effects (riff->handle,
						&sub_region->effects);
		      break;
		    default:
		      DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);
		      break;
		    }
		}
	      else DEBUG_DLS_UNKNOWN_CHUNK (riff, -1);

	      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
	    } /* while() - "3ewl" LIST chunk */

	  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);

	  sub_region_index++; /* advance to next sub region */
	} /* if "3ewl" LIST chunk */

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    } /* while() - "3prg" chunk */

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_gig_reader_load_inst_lart:
 * @reader: Gig reader
 * @err: Location to store error info or %NULL
 *
 * Loads a GigaSampler global parameter chunk '3ewg' from an instrument 'lart'
 * list.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_gig_reader_load_inst_lart (IpatchDLSReader *reader,
				  IpatchGigInst *inst, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_DLS_READER (reader), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_INST (inst), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* no chunks? - return */
  if (!(chunk = ipatch_riff_read_chunk (riff, err)))
    return (ipatch_riff_get_error (riff, NULL));

  /* not a '3ewg' chunk? - return */
  if (chunk->type != IPATCH_RIFF_CHUNK_SUB
      || chunk->id != IPATCH_GIG_FOURCC_3EWG
      || chunk->size != IPATCH_GIG_3EWG_SIZE)
    {
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
      return (TRUE);
    }

  /* read the 3ewg chunk data */
  if (!ipatch_file_read (riff->handle, &inst->chunk_3ewg,
			 IPATCH_GIG_3EWG_SIZE, err))
    return (FALSE);

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/**
 * ipatch_dls_load_info:
 * @riff: RIFF parser
 * @info: Pointer to DLS info list
 * @err: Location to store error info or %NULL
 *
 * Loads DLS or GigaSampler info from the current position in the file
 * assigned to @riff.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_info (IpatchRiff *riff, IpatchDLS2Info **info,
		      GError **err)
{
  IpatchRiffChunk *chunk;
  guint32 size;
  char *str;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB && chunk->size > 0)
	{
	  size = chunk->size;
	  if (size > IPATCH_DLS_MAX_INFO_SIZE)
	    size = IPATCH_DLS_MAX_INFO_SIZE;

	  str = g_malloc (size);
	  if (!ipatch_file_read (riff->handle, str, size, err))
	    {
	      g_free (str);
	      return (FALSE);
	    }
	  str[size - 1] = '\0';	/* force terminate in case it isn't */

	  ipatch_dls2_info_set (info, chunk->id, str);
	  g_free (str);
	}

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_dls_load_region_header:
 * @riff: RIFF parser
 * @region: DLS region to store header info in
 * @err: Location to store error info or %NULL
 *
 * Loads DLS instrument region header ("rgnh" chunk)
 * from the current position in the file assigned to @riff. The "rgnh" chunk
 * header should have already been loaded.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_region_header (IpatchRiff *riff,
			       IpatchDLS2Region *region, GError **err)
{
  IpatchRiffChunk *chunk;
  guint16 options;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != IPATCH_DLS_RGNH_SIZE
      && chunk->size != IPATCH_DLS_RGNH_LAYER_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, chunk->size, err)) return (FALSE);

  region->note_range_low = ipatch_file_buf_read_u16 (riff->handle);
  region->note_range_high = ipatch_file_buf_read_u16 (riff->handle);
  region->velocity_range_low = ipatch_file_buf_read_u16 (riff->handle);
  region->velocity_range_high = ipatch_file_buf_read_u16 (riff->handle);

  /* ISOK? Undefined flags are discarded! */
  options = ipatch_file_buf_read_u16 (riff->handle);
  if (options & IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE)
    ipatch_item_set_flags (region, IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE);

  region->key_group = ipatch_file_buf_read_u16 (riff->handle);

  if (chunk->size == IPATCH_DLS_RGNH_LAYER_SIZE) /* optional layer field? */
    region->layer_group = ipatch_file_buf_read_u16 (riff->handle);

  return (TRUE);
}

/**
 * ipatch_gig_load_region_header:
 * @riff: RIFF parser
 * @region: Gig region to store header info in
 * @err: Location to store error info or %NULL
 *
 * Loads GigaSampler instrument region header ("rgnh" chunk)
 * from the current position in the file assigned to @riff. The "rgnh" chunk
 * header should have already been loaded.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_gig_load_region_header (IpatchRiff *riff,
			       IpatchGigRegion *region, GError **err)
{
  IpatchRiffChunk *chunk;
  guint16 options;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != IPATCH_DLS_RGNH_SIZE
      && chunk->size != IPATCH_DLS_RGNH_LAYER_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, chunk->size, err)) return (FALSE);

  region->note_range_low = ipatch_file_buf_read_u16 (riff->handle);
  region->note_range_high = ipatch_file_buf_read_u16 (riff->handle);
  region->velocity_range_low = ipatch_file_buf_read_u16 (riff->handle);
  region->velocity_range_high = ipatch_file_buf_read_u16 (riff->handle);

  /* ISOK? Undefined flags are discarded! */
  options = ipatch_file_buf_read_u16 (riff->handle);
  if (options & IPATCH_DLS_RGNH_OPTION_SELF_NON_EXCLUSIVE)
    ipatch_item_set_flags (region, IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE);

  region->key_group = ipatch_file_buf_read_u16 (riff->handle);

  if (chunk->size == IPATCH_DLS_RGNH_LAYER_SIZE) /* optional layer field? */
    region->layer_group = ipatch_file_buf_read_u16 (riff->handle);

  return (TRUE);
}

/**
 * ipatch_dls_load_wave_link:
 * @riff: RIFF parser
 * @region: DLS region to load info into
 * @err: Location to store error info or %NULL
 *
 * Loads DLS wave link chunk ("wlnk" chunk) from the current position in the
 * file assigned to @riff. The "wlnk" chunk header should have already been
 * loaded.
 *
 * NOTE: Sample pool index is stored in @region sample pointer. This index
 * should be fixed up or cleared before the region is freed otherwise bad
 * things will happen.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_wave_link (IpatchRiff *riff, IpatchDLS2Region *region,
			   GError **err)
{
  IpatchRiffChunk *chunk;
  guint16 options;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != IPATCH_DLS_WLNK_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, chunk->size, err)) return (FALSE);

  /* ISOK? Undefined flags are discarded! */
  options = ipatch_file_buf_read_u16 (riff->handle);
  if (options & IPATCH_DLS_WLNK_PHASE_MASTER)
    ipatch_item_set_flags (region, IPATCH_DLS2_REGION_PHASE_MASTER);
  if (options & IPATCH_DLS_WLNK_MULTI_CHANNEL)
    ipatch_item_set_flags (region, IPATCH_DLS2_REGION_MULTI_CHANNEL);

  region->phase_group = ipatch_file_buf_read_u16 (riff->handle);
  region->channel = ipatch_file_buf_read_u32 (riff->handle);

  /* store sample pool index in sample pointer (for later fixup) */
  region->sample = GINT_TO_POINTER (ipatch_file_buf_read_u32 (riff->handle));

  return (TRUE);
}

/**
 * ipatch_dls_load_sample_info:
 * @riff: RIFF parser
 * @info: (out): Sample info structure to load info into
 * @err: Location to store error info or %NULL
 *
 * Loads DLS or GigaSampler sample info ("wsmp" chunk) from the
 * current position in the file assigned to @riff. The "wsmp" chunk header
 * should already have been loaded.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_sample_info (IpatchRiff *riff,
			     IpatchDLS2SampleInfo *info, GError **err)
{
  IpatchRiffChunk *chunk;
  guint32 options, struct_size, loop_count, loop_type;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size > VARCHUNK_BUF_SIZE
      || chunk->size < IPATCH_DLS_WSMP_HEADER_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, chunk->size, err)) return (FALSE);

  struct_size = ipatch_file_buf_read_u32 (riff->handle);
  if (struct_size < IPATCH_DLS_WSMP_HEADER_SIZE || struct_size & 1)
    {				/* structure size sanity check */
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  info->root_note = (guint8)ipatch_file_buf_read_u16 (riff->handle);
  info->fine_tune = ipatch_file_buf_read_u16 (riff->handle);
  info->gain = ipatch_file_buf_read_u32 (riff->handle);

  /* ISOK? Undefined flags are discarded! */
  options = ipatch_file_buf_read_u32 (riff->handle);
  if (options & IPATCH_DLS_WSMP_NO_TRUNCATION)
    info->options |= IPATCH_DLS2_SAMPLE_NO_TRUNCATION;
  if (options & IPATCH_DLS_WSMP_NO_COMPRESSION)
    info->options |= IPATCH_DLS2_SAMPLE_NO_COMPRESSION;

  /* skip header expansion data (if any) */
  ipatch_file_buf_skip (riff->handle, struct_size - IPATCH_DLS_WSMP_HEADER_SIZE);

  loop_count = ipatch_file_buf_read_u32 (riff->handle);

  /* we only support 1 loop, but work even if > 1 (spec says undefined) */
  if (loop_count > 0 && chunk->size >= struct_size
      + IPATCH_DLS_WSMP_LOOP_SIZE)
    {
      ipatch_file_buf_skip (riff->handle, 4); /* skip loop structure size */

      loop_type = ipatch_file_buf_read_u32 (riff->handle);
      if (loop_type == IPATCH_DLS_WSMP_LOOP_RELEASE)
	info->options |= IPATCH_SAMPLE_LOOP_RELEASE;
      else info->options |= IPATCH_SAMPLE_LOOP_STANDARD; /* default */

      info->loop_start = ipatch_file_buf_read_u32 (riff->handle);
      info->loop_end =
	info->loop_start + ipatch_file_buf_read_u32 (riff->handle);
    }

  return (TRUE);
}

/**
 * ipatch_dls_load_connection:
 * @riff: RIFF parser
 * @conn_list: (out) (element-type IpatchDLS2Conn): Pointer to a list to populate
 *   with loaded #IpatchDLS2Conn structures.
 * @err: Location to store error info or %NULL
 *
 * Load a DLS articulator chunk ("art1" or "art2") containing connection
 * blocks which are loded into @conn_list. The articulation chunk header
 * should already have been loaded.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_connection (IpatchRiff *riff, GSList **conn_list,
			    GError **err)
{
  IpatchDLS2Conn *conn;
  IpatchRiffChunk *chunk;
  int header_size, count, i;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (conn_list != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size < IPATCH_DLS_ART_HEADER_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, IPATCH_DLS_ART_HEADER_SIZE, err))
    return (FALSE);

  header_size = ipatch_file_buf_read_u32 (riff->handle);
  if (header_size < IPATCH_DLS_ART_HEADER_SIZE || header_size & 1)
    {			       /* make sure art header size is sane */
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  /* check for header expansion */
  if (header_size > IPATCH_DLS_ART_HEADER_SIZE)
    {				/* skip expansion data */
      if (!ipatch_file_seek (riff->handle, header_size - IPATCH_DLS_ART_HEADER_SIZE,
			     G_SEEK_CUR, err))
	return (FALSE);

      /* load connection count (last field before connection blocks) */
      if (!ipatch_file_buf_load (riff->handle, 4, err)) return (FALSE);
    }

  count = ipatch_file_buf_read_u32 (riff->handle);

  if (chunk->size != header_size + count * IPATCH_DLS_CONN_SIZE)
    {			/* make sure connection block count is sane */
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!count) return (TRUE);

  if (!ipatch_file_buf_load (riff->handle, chunk->size - header_size, err))
    return (FALSE);

  for (i = count; i > 0; i--)	/* parse connection blocks */
    {
      conn = ipatch_dls2_conn_new ();
      *conn_list = g_slist_prepend (*conn_list, conn);

      conn->src = ipatch_file_buf_read_u16 (riff->handle);
      conn->ctrlsrc = ipatch_file_buf_read_u16 (riff->handle);
      conn->dest = ipatch_file_buf_read_u16 (riff->handle);
      conn->trans = ipatch_file_buf_read_u16 (riff->handle);
      conn->scale = ipatch_file_buf_read_s32 (riff->handle);
    }

  return (TRUE);
}

/**
 * ipatch_dls_load_sample_format:
 * @riff: RIFF parser
 * @sample: DLS sample to load data into
 * @bitwidth: (out) (optional): Pointer to an integer to fill with the
 *   sample's bit width or %NULL
 * @channels: (out) (optional): Pointer to an integer to fill with the
 *   number of channels or %NULL
 * @err: Location to store error info or %NULL
 *
 * Parses DLS sample format info ("fmt " chunk) from the current
 * position in the file assigned to the @riff (chunk header should already
 * be loaded).
 *
 * Returns: %TRUE on success, %FALSE on error (in which case @err may be set).
 */
static gboolean
ipatch_dls_load_sample_format (IpatchRiff *riff,
			       IpatchDLS2Sample *sample,
			       int *bitwidth, int *channels,
			       GError **err)
{
  IpatchRiffChunk *chunk;
  guint16 i16;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_DLS2_SAMPLE (sample), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* since it seems there are various length FMT chunks, we only assert that
     it is at least the size of the default PCM format chunk.
     sizes seen: (18 bytes..) */
  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size < IPATCH_DLS_WAVE_FMT_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, IPATCH_DLS_WAVE_FMT_SIZE, err))
    return (FALSE);

  i16 = ipatch_file_buf_read_u16 (riff->handle);
  if (i16 != IPATCH_RIFF_WAVE_FMT_PCM) /* assert PCM wave data */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  i16 = ipatch_file_buf_read_u16 (riff->handle);
  if (i16 != 1 && i16 != 2)	/* assert 1 or 2 channel data */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (channels) *channels = i16;

  sample->rate = ipatch_file_buf_read_u32 (riff->handle);

  /* skip useless dwAvgBytesPerSec and wBlockAlign fields */
  ipatch_file_buf_skip (riff->handle, 6);

  /* load bit width */
  i16 = ipatch_file_buf_read_u16 (riff->handle);
  if (i16 != 8 && i16 != 16)   /* FIXME - Support higher bit widths */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (bitwidth) *bitwidth = i16;

  return (TRUE);
}

/**
 * ipatch_dls_load_pool_table:
 * @riff: RIFF parser
 * @size: (out): Pointer to an unsigned integer to store number of entries in
 *   returned pool table array
 * @err: Location to store error info or %NULL
 *
 * Load a sample pool table ("ptbl" chunk) of a DLS or GigaSampler
 * file from the current position in the file assigned to @riff (chunk
 * header should already be loaded).
 *
 * Returns: (array length=size): A newly allocated array of 32bit integers
 * for each entry in the pool table, or %NULL if empty pool table or on
 * error (in which case @err may be set). Free the table when finished with it.
 */
static guint32 *
ipatch_dls_load_pool_table (IpatchRiff *riff, guint *size,
			    GError **err)
{
  IpatchRiffChunk *chunk;
  guint32 *cuep, *tmpcuep;
  guint header_size, count;

  if (size) *size = 0;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), NULL);
  g_return_val_if_fail (size != NULL, NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size < IPATCH_DLS_PTBL_HEADER_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (NULL);
    }

  if (!ipatch_file_buf_load (riff->handle, IPATCH_DLS_PTBL_HEADER_SIZE, err))
    return (NULL);

  header_size = ipatch_file_buf_read_u32 (riff->handle);

  if (header_size < IPATCH_DLS_PTBL_HEADER_SIZE || header_size & 1)
    {				/* make sure art header size is sane */
      SET_DATA_ERROR (riff, -1, err);
      return (NULL);
    }

  /* check for header expansion */
  if (header_size > IPATCH_DLS_PTBL_HEADER_SIZE)
    {				/* skip expansion data */
      if (!ipatch_file_seek (riff->handle, header_size - IPATCH_DLS_PTBL_HEADER_SIZE,
			     G_SEEK_CUR, err))
	return (NULL);

      /* load cue count (last field before cue offsets) */
      if (!ipatch_file_buf_load (riff->handle, 4, err))
	return (NULL);
    }

  count = ipatch_file_buf_read_u32 (riff->handle);

  if (chunk->size != header_size + count * IPATCH_DLS_POOLCUE_SIZE)
    {			/* make sure pool cue count is sane */
      SET_SIZE_ERROR (riff, -1, err);
      return (NULL);
    }

  if (!count) return (NULL);	/* no pool table? */

  cuep = g_malloc (chunk->size - header_size);

  /* load pool cue offsets */
  if (!ipatch_file_read (riff->handle, cuep, chunk->size - header_size, err))
    {
      g_free (cuep);
      return (NULL);
    }

  *size = count;

  /* do endian swap on cue offsets if needed */
  if (IPATCH_RIFF_NEED_SWAP (riff))
    {
      tmpcuep = cuep;
      while (count-- > 0)
	{
	  *tmpcuep = GUINT32_SWAP_LE_BE (*tmpcuep);
	  tmpcuep++;
	}
    }

  return (cuep);
}

/**
 * ipatch_dls_load_dlid:
 * @riff: Riff parser
 * @dlid: (out): Location to store 16 byte DLSID
 * @err: Location to store error info or %NULL
 * 
 * Load a DLSID from the current position in a riff object.  This is a
 * 16 byte unique ID used for tracking data changes.  The "dlid" header
 * should have already been loaded.
 * 
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
ipatch_dls_load_dlid (IpatchRiff *riff, guint8 *dlid, GError **err)
{
  IpatchRiffChunk *chunk;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (dlid != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != 16)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_read (riff->handle, dlid, 16, err)) return (FALSE);

  return (TRUE);
}

/**
 * ipatch_gig_load_sample_info:
 * @riff: RIFF parser
 * @info: DLS sample info to load data into
 * @err: Location to store error info or %NULL
 *
 * Load Gig sample info ("smpl" chunk) from current position in file assigned
 * to @riff (chunk header should already be loaded).
 *
 * Returns: %TRUE on success, %FALSE on error (in which case @err may be set).
 */
static gboolean
ipatch_gig_load_sample_info (IpatchRiff *riff,
			     IpatchDLS2SampleInfo *info, GError **err)
{
  IpatchRiffChunk *chunk;
  guint32 loop_count;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != IPATCH_GIG_SMPL_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, IPATCH_GIG_SMPL_SIZE, err))
    return (FALSE);

  ipatch_file_buf_read_u32 (riff->handle); /* manufacturer */
  ipatch_file_buf_read_u32 (riff->handle); /* product */
  ipatch_file_buf_read_u32 (riff->handle); /* sample period in nanoseconds */

  info->root_note = (guint8)ipatch_file_buf_read_u32 (riff->handle);

  /* FIXME - Is this an unsigned 32 bit fraction of a semitone? */
  info->fine_tune = (guint16)ipatch_file_buf_read_u32 (riff->handle);

  ipatch_file_buf_read_u32 (riff->handle); /* SMPTE format */
  ipatch_file_buf_read_u32 (riff->handle); /* SMPTE offset */

  loop_count = ipatch_file_buf_read_u32 (riff->handle);

  ipatch_file_buf_read_u32 (riff->handle); /* manufBytes */
  ipatch_file_buf_read_u32 (riff->handle); /* loop ID */

  if (loop_count > 0)		/* we only use 1 loop if it exists */
    {
      ipatch_file_buf_read_u32 (riff->handle); /* loop type - FIXME! */

      info->options |= IPATCH_SAMPLE_LOOP_STANDARD;

      info->loop_start = ipatch_file_buf_read_u32 (riff->handle);
      info->loop_end = ipatch_file_buf_read_u32 (riff->handle);
    }

  return (TRUE);
}

/**
 * ipatch_gig_load_dimension_info:
 * @riff: RIFF parser
 * @region: Region to load data into
 * @err: Location to store an error or %NULL
 *
 * Load Gigasampler dimension info ("3lnk" chunk), from the current position
 * in the file assigned to @riff (chunk header should already be loaded).
 *
 * NOTE: Sample pool table indexes are stored in the sample pointer of
 * sub regions. These indexes should be fixed up or cleared before the
 * @region is freed, otherwise bad things will happen.
 *
 * Returns: %TRUE on success, %FALSE on error (in which case @err may be set).
 */
static gboolean
ipatch_gig_load_dimension_info (IpatchRiff *riff,
				IpatchGigRegion *region, GError **err)
{
  IpatchRiffChunk *chunk;
  int count, temp_count, split_count;
  guint8 type, c;
  int i;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size != IPATCH_GIG_3LNK_SIZE)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  if (!ipatch_file_buf_load (riff->handle, IPATCH_GIG_3LNK_SIZE, err))
    return (FALSE);

  count = ipatch_file_buf_read_u32 (riff->handle); /* count of sub regions */
  if (count < 1 || count > 32)	/* should be between 1 and 32 sub regions */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  /* calculate number of used split bits */
  temp_count = count;
  split_count = 0;
  while (!(temp_count & 1))
    {
      temp_count >>= 1;
      split_count++;
    }

  if (temp_count != 1)		/* make sure count is a power of 2 */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  while (split_count > 0)
    {
      type = ipatch_file_buf_read_u8 (riff->handle); /* type of dimension */

      if (type > IPATCH_GIG_DIMENSION_TYPE_MAX)
	DEBUG_DLS ("Unknown GigaSampler dimension type '0x%x'", type);

      c = ipatch_file_buf_read_u8 (riff->handle); /* split bit count */
      ipatch_file_buf_skip (riff->handle, 6); /* FIXME - skip ignored stuff */

      ipatch_gig_region_new_dimension (region, type, c);
      split_count -= c;
    }

  if (split_count != 0)		/* a split bit count is messed up? */
    {
      SET_DATA_ERROR (riff, -1, err);
      return (FALSE);
    }

  /* "seek" to sample cue list */
  ipatch_file_buf_seek (riff->handle, 44, G_SEEK_SET);

  /* store sample indexes to sub region sample pointer fields (fixup later) */
  for (i = 0; i < region->sub_region_count; i++)
    region->sub_regions[i]->sample
      = GUINT_TO_POINTER (ipatch_file_buf_read_u32 (riff->handle));

  return (TRUE);
}

/**
 * ipatch_gig_load_dimension_names:
 * @riff: RIFF parser
 * @region: Gig region to load names into
 * @err: Location to store error info or %NULL
 *
 * Loads Gigasampler dimension names from the current position in the file
 * assigned to @riff. The "3dnl" chunk header should already have been
 * loaded.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
static gboolean
ipatch_gig_load_dimension_names (IpatchRiff *riff,
				 IpatchGigRegion *region, GError **err)
{
  IpatchRiffChunk *chunk;
  char name[256];		/* just use a static buffer for name */
  int i, size;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (IPATCH_IS_GIG_REGION (region), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size == 0) return (TRUE); /* no dimension names */

  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB
	  && strncmp (chunk->idstr, "nam", 3) == 0 && chunk->size > 0)
	{ /* 4th char of FOURCC is dimension index */
	  i = chunk->idstr[3] - '0';
	  if (i >= 0 && i < region->dimension_count)
	    {
	      size = MIN (chunk->size, 255);
	      if (!ipatch_file_read (riff->handle, name, size, err))
		return (FALSE);

	      name[size] = '\0';

	      if (name[0] != '\0')
		g_object_set (region->dimensions[i], "name", name, NULL);
	    }
	}

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    }

  return (ipatch_riff_get_error (riff, NULL));
}

/**
 * ipatch_gig_load_group_names:
 * @riff: Riff parser
 * @name_list: (out) (element-type char*): List to add names to
 * @err: Location to store error info or %NULL
 * 
 * Load a '3gri' sample group name chunk into a GSList of strings.  The
 * 3gri chunk header should have already been loaded.
 * 
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
ipatch_gig_load_group_names (IpatchRiff *riff, GSList **name_list,
			     GError **err)
{
  IpatchRiffChunk *chunk;
  char buf[65], *name;
  int size;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_RIFF (riff), FALSE);
  g_return_val_if_fail (name_list != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* no chunks? - return */
  if (!(chunk = ipatch_riff_read_chunk (riff, err)))
    return (ipatch_riff_get_error (riff, NULL));

  /* not a '3gnl' chunk? - return */
  if (chunk->type != IPATCH_RIFF_CHUNK_LIST
      || chunk->id != IPATCH_GIG_FOURCC_3GNL)
    {
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
      return (TRUE);
    }

  /* loop over 3gnm chunks */
  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB
	  && chunk->id == IPATCH_GIG_FOURCC_3GNM)
	{
	  size = MIN (chunk->size, sizeof (buf) - 1);
	  if (!ipatch_file_read (riff->handle, &buf, size, err)) goto err;
	  buf[sizeof(buf) - 1] = '\0';

	  size = strlen (buf);
	  if (size > 0)
	    {
	      name = g_memdup (buf, size + 1);
	      *name_list = g_slist_append (*name_list, name);
	    }
	}

      if (!ipatch_riff_end_chunk (riff, err)) goto err;
    }

  /* make sure no errors occured */
  if (!ipatch_riff_get_error (riff, NULL)) return (FALSE);

  if (!ipatch_riff_end_chunk (riff, err)) goto err;

  return (TRUE);

 err:

  /* free any existing names */
  p = *name_list;
  while (p)
    {
      g_free (p->data);
      p = g_slist_delete_link (p, p);
    }

  *name_list = NULL;

  return (FALSE);
}
