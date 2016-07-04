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
 * SECTION: IpatchSF2Writer
 * @short_description: SoundFont writer object
 * @see_also: 
 * @stability: Stable
 *
 * Object for writing a tree of SoundFont objects (#IpatchSF2) to a SoundFont
 * file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IpatchSF2Writer.h"
#include "IpatchSF2File_priv.h"
#include "IpatchBase.h"
#include "IpatchFile.h"
#include "IpatchSampleStore.h"
#include "IpatchSampleStoreFile.h"
#include "IpatchSampleStoreRom.h"
#include "IpatchSampleStoreSwap.h"
#include "IpatchSampleStoreSplit24.h"
#include "IpatchSF2Preset.h"
#include "IpatchSF2Inst.h"
#include "IpatchSF2Sample.h"
#include "IpatchSF2PZone.h"
#include "IpatchSF2IZone.h"
#include "IpatchSF2Zone.h"
#include "IpatchUnit.h"
#include "i18n.h"
#include "ipatch_priv.h"
#include "misc.h"
#include "version.h"

/* NOTICE: A duplicate SoundFont object is used for saving. This
 * solves all multi-thread issues and allows one to continue editing
 * even while a SoundFont is being saved. It also means that the
 * duplicate SoundFont object can be accessed directly without
 * locking. Sample data objects are not duplicated though, so they
 * still need to be dealt with properly.
 */

enum
{
  PROP_0,
  PROP_MIGRATE_SAMPLES
};

/* Hash value used for sample_hash */
typedef struct
{
  guint index;          /* Sample index */
  guint position;       /* Position in file */
  guint position24;     /* 24 bit byte file position (or 0 if 16 bit sample) */
} SampleHashValue;

/* SoundFont 16 bit sample format */
#define FORMAT_16BIT    IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_MONO \
  | IPATCH_SAMPLE_SIGNED | IPATCH_SAMPLE_LENDIAN

/* SoundFont 24 bit sample format */
#define FORMAT_24BIT    IPATCH_SAMPLE_24BIT | IPATCH_SAMPLE_MONO \
  | IPATCH_SAMPLE_SIGNED | IPATCH_SAMPLE_LENDIAN

static void ipatch_sf2_writer_set_property (GObject *object, guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_sf2_writer_get_property (GObject *object, guint property_id,
					    GValue *value, GParamSpec *pspec);
static SampleHashValue *ipatch_sf2_writer_sample_hash_value_new (void);
static void ipatch_sf2_writer_sample_hash_value_destroy (gpointer value);
static void ipatch_sf2_writer_finalize (GObject *object);
static gboolean ipatch_sf2_write_level_0 (IpatchSF2Writer *writer,
					  GError **err);
static gboolean sfont_write_info (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_strchunk (IpatchSF2Writer *writer, guint32 id,
				      const char *val, GError **err);
static gboolean sfont_write_samples (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_samples24 (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_phdrs (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_pbags (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_pmods (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_pgens (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_ihdrs (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_ibags (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_imods (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_igens (IpatchSF2Writer *writer, GError **err);
static gboolean sfont_write_shdrs (IpatchSF2Writer *writer, GError **err);


G_DEFINE_TYPE (IpatchSF2Writer, ipatch_sf2_writer, IPATCH_TYPE_RIFF)


static void
ipatch_sf2_writer_class_init (IpatchSF2WriterClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = ipatch_sf2_writer_set_property;
  obj_class->get_property = ipatch_sf2_writer_get_property;
  obj_class->finalize = ipatch_sf2_writer_finalize;

  /**
   * IpatchSF2Writer:migrate-samples:
   *
   * Was supposed to migrate sample data to the new file, was not implemented properly though.
   * Does nothing now.
   *
   * Deprecated: 1.1.0: Use ipatch_sf2_writer_create_stores() instead.
   **/
  g_object_class_install_property (obj_class, PROP_MIGRATE_SAMPLES,
		g_param_spec_boolean ("migrate-samples", _("Migrate Samples"),
				      _("Migrate samples to new file"),
				      FALSE, G_PARAM_READWRITE));
}

static void
ipatch_sf2_writer_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_MIGRATE_SAMPLES:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sf2_writer_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_MIGRATE_SAMPLES:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sf2_writer_init (IpatchSF2Writer *writer)
{
  writer->inst_hash = g_hash_table_new (NULL, NULL);
  writer->sample_hash = g_hash_table_new_full (NULL, NULL, NULL,
                                               ipatch_sf2_writer_sample_hash_value_destroy);
}

static SampleHashValue *
ipatch_sf2_writer_sample_hash_value_new (void)
{
  return (g_slice_new0 (SampleHashValue));
}

static void
ipatch_sf2_writer_sample_hash_value_destroy (gpointer value)
{
  g_slice_free (SampleHashValue, value);
}

static void
ipatch_sf2_writer_finalize (GObject *object)
{
  IpatchSF2Writer *writer = IPATCH_SF2_WRITER (object);

  if (writer->orig_sf) g_object_unref (writer->orig_sf);
  if (writer->sf) g_object_unref (writer->sf);
  g_hash_table_destroy (writer->inst_hash);
  g_hash_table_destroy (writer->sample_hash);
  if (writer->store_list) g_object_unref (writer->store_list);

  if (G_OBJECT_CLASS (ipatch_sf2_writer_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sf2_writer_parent_class)->finalize (object);
}

/**
 * ipatch_sf2_writer_new:
 * @handle: SoundFont file handle to save to or %NULL to set later
 * @sfont: SoundFont object to save or %NULL to set later
 *
 * Create a new SoundFont 2 file writer.
 *
 * Returns: The new SoundFont writer
 */
IpatchSF2Writer *
ipatch_sf2_writer_new (IpatchFileHandle *handle, IpatchSF2 *sfont)
{
  IpatchSF2Writer *writer;

  g_return_val_if_fail (!sfont || IPATCH_IS_SF2 (sfont), NULL);
  g_return_val_if_fail (!handle || IPATCH_IS_SF2_FILE (handle->file), NULL);

  writer = g_object_new (IPATCH_TYPE_SF2_WRITER, NULL);
  if (sfont) ipatch_sf2_writer_set_patch (writer, sfont);
  if (handle) ipatch_sf2_writer_set_file_handle (writer, handle);

  return (writer);
}

/**
 * ipatch_sf2_writer_set_patch:
 * @writer: SoundFont writer object
 * @sfont: SoundFont patch to save
 *
 * Set the SoundFont patch object to save with a SoundFont writer.
 */
void
ipatch_sf2_writer_set_patch (IpatchSF2Writer *writer, IpatchSF2 *sfont)
{
  g_return_if_fail (IPATCH_IS_SF2_WRITER (writer));
  g_return_if_fail (IPATCH_IS_SF2 (sfont));

  if (writer->orig_sf) g_object_unref (writer->orig_sf);
  writer->orig_sf = g_object_ref (sfont);
}

/**
 * ipatch_sf2_writer_set_file:
 * @writer: SoundFont writer object
 * @handle: SoundFont file handle
 *
 * Set the SoundFont file handle of a SoundFont writer. A convenience
 * function, since ipatch_riff_set_file_handle() could also be used, albeit
 * without stricter type casting.
 */
void
ipatch_sf2_writer_set_file_handle (IpatchSF2Writer *writer,
                                   IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_SF2_WRITER (writer));
  g_return_if_fail (handle && IPATCH_IS_SF2_FILE (handle->file));

  ipatch_riff_set_file_handle (IPATCH_RIFF (writer), handle);
}

/**
 * ipatch_sf2_writer_save:
 * @writer: SoundFont writer object
 * @err: Location to store error info or %NULL
 *
 * Write a SoundFont object to a file.
 *
 * Returns: %TRUE on success, %FALSE on error
 */
gboolean
ipatch_sf2_writer_save (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff;
  gboolean b;

  g_return_val_if_fail (IPATCH_IS_SF2_WRITER (writer), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);
  g_return_val_if_fail (writer->orig_sf != NULL, FALSE);

  /* shouldn't be set, but.. */
  if (writer->sf) g_object_unref (writer->sf);
  
  /* set SoundFont version according to whether 24 bit samples are enabled */
  b = (ipatch_item_get_flags (IPATCH_ITEM (writer->orig_sf))
       & IPATCH_SF2_SAMPLES_24BIT) != 0;

  g_object_set (writer->orig_sf, "version", b ? "2.04" : "2.01", NULL);

  /* duplicate for save, so we can be multi-thread friendly :)
     ++ ref new duplicate object */
  writer->sf = IPATCH_SF2 (ipatch_item_duplicate (IPATCH_ITEM (writer->orig_sf)));

  riff = IPATCH_RIFF (writer);

  /* write toplevel SoundFont RIFF chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_RIFF,
				IPATCH_SFONT_FOURCC_SFBK, err))
    return (FALSE);

  if (!ipatch_sf2_write_level_0 (writer, err))
    goto err;

  /* close the RIFF chunk */
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  g_object_set (writer->orig_sf,
		"changed", FALSE, /* file and object are in sync */
		"saved", TRUE,	/* has now been saved */
		NULL);

  return (TRUE);

 err:

  g_object_unref (writer->sf); /* -- unref duplicate SoundFont */
  writer->sf = NULL;

  return (FALSE);
}

/**
 * ipatch_sf2_writer_create_stores:
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
ipatch_sf2_writer_create_stores (IpatchSF2Writer *writer)
{
  SampleHashValue *hash_value;
  IpatchSample *newstore;
  IpatchFile *save_file;
  IpatchSF2Sample *sample;
  IpatchIter iter;
  gboolean smpl24;
  IpatchList *list;
  int rate, format;
  guint size;

  g_return_val_if_fail (writer->sf != NULL, NULL);

  // Return existing store list (if this function has been called before)
  if (writer->store_list)
    return (g_object_ref (writer->store_list));         // ++ ref for caller

  save_file = IPATCH_RIFF (writer)->handle->file;
  smpl24 = (ipatch_item_get_flags (writer->sf) & IPATCH_SF2_SAMPLES_24BIT) != 0;

  list = ipatch_list_new ();            // ++ ref list

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_SAMPLE);

  /* traverse samples */
  for (sample = ipatch_sf2_sample_first (&iter); sample;
       sample = ipatch_sf2_sample_next (&iter))
  {
    hash_value = g_hash_table_lookup (writer->sample_hash, sample);

    /* Skip ROM samples, hash_value should never be NULL, but.. */
    if (!hash_value || hash_value->position == 0) continue;

    g_object_get (sample,
                  "sample-format", &format,
                  "sample-size", &size,
                  "sample-rate", &rate,
                  NULL);

    // Create 16 bit sample store if SoundFont does not contain 24 bit or original sample was 16 bit or less
    if (!smpl24 || IPATCH_SAMPLE_FORMAT_GET_WIDTH (format) <= IPATCH_SAMPLE_16BIT)
    {           /* ++ ref newstore */
      newstore = ipatch_sample_store_file_new (save_file, hash_value->position);
      format = FORMAT_16BIT;
    }
    else        /* ++ ref newstore */
    {
      newstore = ipatch_sample_store_split24_new (save_file, hash_value->position,
                                                  hash_value->position24);
      format = FORMAT_24BIT;
    }

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

/**
 * ipatch_sf2_write_phdr: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @phdr: Preset header structure to store
 *
 * Buffer writes a preset header into @handle from a @phdr structure.
 */
void
ipatch_sf2_write_phdr (IpatchFileHandle *handle, const IpatchSF2Phdr *phdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (phdr != NULL);

  ipatch_file_buf_write (handle, phdr->name, IPATCH_SFONT_NAME_SIZE);
  ipatch_file_buf_write_u16 (handle, phdr->program);
  ipatch_file_buf_write_u16 (handle, phdr->bank);
  ipatch_file_buf_write_u16 (handle, phdr->bag_index);
  ipatch_file_buf_write_u32 (handle, phdr->library);
  ipatch_file_buf_write_u32 (handle, phdr->genre);
  ipatch_file_buf_write_u32 (handle, phdr->morphology);
}

/**
 * ipatch_sf2_write_ihdr: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @ihdr: Instrument header structure to store
 *
 * Writes an instrument header into @handle from a @ihdr structure.
 */
void
ipatch_sf2_write_ihdr (IpatchFileHandle *handle, const IpatchSF2Ihdr *ihdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (ihdr != NULL);

  ipatch_file_buf_write (handle, ihdr->name, IPATCH_SFONT_NAME_SIZE);
  ipatch_file_buf_write_u16 (handle, ihdr->bag_index);
}

/**
 * ipatch_sf2_write_shdr: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @shdr: Sample header structure to store
 *
 * Writes a sample header into @handle from a @shdr structure.
 */
void
ipatch_sf2_write_shdr (IpatchFileHandle *handle, const IpatchSF2Shdr *shdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (shdr != NULL);

  ipatch_file_buf_write (handle, shdr->name, IPATCH_SFONT_NAME_SIZE);
  ipatch_file_buf_write_u32 (handle, shdr->start);
  ipatch_file_buf_write_u32 (handle, shdr->end);
  ipatch_file_buf_write_u32 (handle, shdr->loop_start);
  ipatch_file_buf_write_u32 (handle, shdr->loop_end);
  ipatch_file_buf_write_u32 (handle, shdr->rate);
  ipatch_file_buf_write_u8 (handle, shdr->root_note);
  ipatch_file_buf_write_u8 (handle, shdr->fine_tune);
  ipatch_file_buf_write_u16 (handle, shdr->link_index);
  ipatch_file_buf_write_u16 (handle, shdr->type);
}

/**
 * ipatch_sf2_write_bag: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @bag: Bag structure to store
 *
 * Writes a preset or instrument bag into @handle from a @bag structure.
 */
void
ipatch_sf2_write_bag (IpatchFileHandle *handle, const IpatchSF2Bag *bag)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (bag != NULL);

  ipatch_file_buf_write_u16 (handle, bag->gen_index);
  ipatch_file_buf_write_u16 (handle, bag->mod_index);
}

/**
 * ipatch_sf2_write_mod: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @mod: Modulator structure to store
 *
 * Writes a modulator into @handle from a @mod structure.
 */
void
ipatch_sf2_write_mod (IpatchFileHandle *handle, const IpatchSF2Mod *mod)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (mod != NULL);

  ipatch_file_buf_write_u16 (handle, mod->src);
  ipatch_file_buf_write_u16 (handle, mod->dest);
  ipatch_file_buf_write_u16 (handle, mod->amount);
  ipatch_file_buf_write_u16 (handle, mod->amtsrc);
  ipatch_file_buf_write_u16 (handle, mod->trans);
}

/**
 * ipatch_sf2_write_gen: (skip)
 * @handle: File handle to buffer writes to, commit after calling this function
 * @genid: ID of generator to store
 * @amount: Generator amount to store
 *
 * Writes a generator into @handle from a @genid and @amount
 * structure.
 */
void
ipatch_sf2_write_gen (IpatchFileHandle *handle, int genid,
		      const IpatchSF2GenAmount *amount)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (amount != NULL);

  ipatch_file_buf_write_u16 (handle, genid);

  /* check if genid is valid (preset or inst) and is a range unit */
  if (genid != IPATCH_SF2_GEN_INSTRUMENT_ID
      && genid != IPATCH_SF2_GEN_SAMPLE_ID
      && ipatch_sf2_gen_is_valid (genid, FALSE)
      && ipatch_sf2_gen_info[genid].unit == IPATCH_UNIT_TYPE_RANGE)
    {				/* store the range */
      ipatch_file_buf_write_u8 (handle, amount->range.low);
      ipatch_file_buf_write_u8 (handle, amount->range.high);
    }
  else ipatch_file_buf_write_s16 (handle, amount->sword);
}

static gboolean
ipatch_sf2_write_level_0 (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Inst *inst;
  IpatchIter iter;
  guint index;

  /* write info list */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_LIST,
				IPATCH_SFONT_FOURCC_INFO, err))
    return (FALSE);
  if (!sfont_write_info (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);


  /* <sdta> - Sample data list chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_LIST,
				IPATCH_SFONT_FOURCC_SDTA, err))
    return (FALSE);

  /* if 24 bit samples are enabled write smpl and sm24 chunks */
  if (ipatch_item_get_flags (writer->sf) & IPATCH_SF2_SAMPLES_24BIT)
    {
      if (!sfont_write_samples24 (writer, err)) return (FALSE);
    }
  else if (!sfont_write_samples (writer, err)) return (FALSE);

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </sdta> */


  /* generate instrument pointer => index hash, used by preset generators */
  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_INST);
  inst = ipatch_sf2_inst_first (&iter);
  index = 1;
  while (inst)
    { /* add instrument and index to instrument hash */
      g_hash_table_insert (writer->inst_hash, inst,
			   GUINT_TO_POINTER (index));
      index++;
      inst = ipatch_sf2_inst_next (&iter);
    }


  /* <pdta> - SoundFont parameter "Hydra" list chunk */
  if (!ipatch_riff_write_list_chunk (riff, IPATCH_SFONT_FOURCC_PDTA, err))
    return (FALSE);


  /* <phdr> - Preset headers */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_PHDR, err))
    return (FALSE);
  if (!sfont_write_phdrs (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <pbag> - Preset bags */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_PBAG, err))
    return (FALSE);
  if (!sfont_write_pbags (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <pmod> - Preset modulators */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_PMOD, err))
    return (FALSE);
  if (!sfont_write_pmods (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <pgen> - Preset generators */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_PGEN, err))
    return (FALSE);
  if (!sfont_write_pgens (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <ihdr> - Instrument headers */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_INST, err))
    return (FALSE);
  if (!sfont_write_ihdrs (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <ibag> - Instrument bags */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_IBAG, err))
    return (FALSE);
  if (!sfont_write_ibags (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <imod> - Instrument modulators */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_IMOD, err))
    return (FALSE);
  if (!sfont_write_imods (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <igen> - Instrument generators */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_IGEN, err))
    return (FALSE);
  if (!sfont_write_igens (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* <shdr> - Sample headers */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_SHDR, err))
    return (FALSE);
  if (!sfont_write_shdrs (writer, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);


  /* </pdta> */
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  return (TRUE);
}

/* save SoundFont info in the recommended order */
static gboolean
sfont_write_info (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Info *info_array;
  char *val, *software;
  int i;

  /* save SoundFont version */
  if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_IFIL, err))
    return (FALSE);
  ipatch_file_buf_write_u16 (riff->handle, writer->sf->ver_major);
  ipatch_file_buf_write_u16 (riff->handle, writer->sf->ver_minor);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  /* save SoundFont synthesis engine */
  if (!(val = ipatch_sf2_get_info (writer->sf, IPATCH_SF2_ENGINE)))
    val = g_strdup (IPATCH_SF2_DEFAULT_ENGINE);
  if (!sfont_write_strchunk (writer, IPATCH_SFONT_FOURCC_ISNG, val, err))
    {
      g_free (val);
      return (FALSE);
    }
  g_free (val);

  /* save SoundFont name */
  if (!(val = ipatch_sf2_get_info (writer->sf, IPATCH_SF2_NAME)))
    val = g_strdup (IPATCH_BASE_DEFAULT_NAME);
  if (!sfont_write_strchunk (writer, IPATCH_SFONT_FOURCC_INAM, val, err))
    {
      g_free (val);
      return (FALSE);
    }
  g_free (val);

  /* SoundFont has ROM name set? */
  if ((val = ipatch_sf2_get_info (writer->sf, IPATCH_SF2_ROM_NAME)))
    { /* save ROM name */
      if (!sfont_write_strchunk (writer, IPATCH_SFONT_FOURCC_IROM, val, err))
	{
	  g_free (val);
	  return (FALSE);
	}
      g_free (val);

      /* save the ROM version too */
      if (!ipatch_riff_write_sub_chunk (riff, IPATCH_SFONT_FOURCC_IVER, err))
	return (FALSE);
      ipatch_file_buf_write_u16 (riff->handle, writer->sf->romver_major);
      ipatch_file_buf_write_u16 (riff->handle, writer->sf->romver_minor);
      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
      if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
    }

  /* write the rest of the info (except ones already written) */
  info_array = ipatch_sf2_get_info_array (writer->sf);
  for (i = 0; info_array[i].id; i++)
    {
      if (info_array[i].id != IPATCH_SF2_ENGINE
	  && info_array[i].id != IPATCH_SF2_NAME
	  && info_array[i].id != IPATCH_SF2_ROM_NAME
	  && info_array[i].id != IPATCH_SF2_SOFTWARE)
	if (!sfont_write_strchunk (writer, info_array[i].id,
				   info_array[i].val, err))
	  {
	    ipatch_sf2_free_info_array (info_array);
	    return (FALSE);
	  }
    }
  ipatch_sf2_free_info_array (info_array);

  /* compose the libInstPatch software string using application name if any */
  if (ipatch_application_name)
    software = g_strconcat (ipatch_application_name,
			    " (libInstPatch " IPATCH_VERSION ")", NULL);
  else software = g_strdup ("libInstPatch " IPATCH_VERSION);

  /* construct software created:modified string */
  val = ipatch_sf2_get_info (writer->sf, IPATCH_SF2_SOFTWARE);
  if (val)
    {
      char *tmp;

      tmp = strchr (val, ':'); /* find colon created:modified separator */
      if (tmp)
	{
	  tmp[1] = '\0';	/* terminate after : */
	  tmp = g_strconcat (val, software, NULL);
	  g_free (val);
	  val = tmp;
	}
      else			/* no colon separator? Discard.. */
	{
	  g_free (val);
	  val = g_strdup (software);
	}
    }
  else val = g_strdup (software);
  g_free (software);

  /* write the software string */
  if (!sfont_write_strchunk (writer, IPATCH_SFONT_FOURCC_ISFT, val, err))
    {
      g_free (val);
      return (FALSE);
    }
  g_free (val);

  return (TRUE);
}

/* write a even size null terminated string contained in a sub chunk */
static gboolean
sfont_write_strchunk (IpatchSF2Writer *writer, guint32 id, const char *val,
		      GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  char pad = '\0';
  int len;

  if (!ipatch_riff_write_sub_chunk (riff, id, err)) return (FALSE);

  len = strlen (val) + 1;
  if (!ipatch_file_write (riff->handle, val, len, err)) return (FALSE);
  if (len & 1)			/* pad to an even number of bytes */
    if (!ipatch_file_write (riff->handle, &pad, 1, err)) return (FALSE);

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);

  return (TRUE);
}

/* save sample data (16 bit mode) */
static gboolean
sfont_write_samples (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Sample *sample;
  IpatchSampleHandle handle;
  IpatchSF2File *file = IPATCH_SF2_FILE (riff->handle->file);
  SampleHashValue *sample_hash_value;
  IpatchRiffChunk *chunk;
  IpatchIter iter;
  gpointer buf;
  guint8 zerobuf[46 * 2]; /* 46 zero values to write at end of each sample */
  guint samsize, size, ofs;
  int index = 0;

  memset (&zerobuf, 0, sizeof (zerobuf));

  /* <smpl> - Sample data sub chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_SUB,
				IPATCH_SFONT_FOURCC_SMPL, err))
    return (FALSE);

  /* set the sample position in the IpatchSF2File */
  ipatch_sf2_file_set_sample_pos (file, ipatch_riff_get_position (riff));

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_SAMPLE);

  /* traverse samples */
  for (sample = ipatch_sf2_sample_first (&iter); sample;
       sample = ipatch_sf2_sample_next (&iter))
    {
      /* add sample info to sample hash */
      sample_hash_value = ipatch_sf2_writer_sample_hash_value_new ();
      sample_hash_value->index = index++;
      g_hash_table_insert (writer->sample_hash, sample, sample_hash_value);

      /* ignore ROM samples */
      if (ipatch_item_get_flags (sample) & IPATCH_SF2_SAMPLE_FLAG_ROM) continue;

      /* get sample position in sample chunk and store to sample hash value */
      chunk = ipatch_riff_get_chunk (riff, -1);
      sample_hash_value->position = file->sample_pos + chunk->position;

      /* ++ open sample handle */
      if (!ipatch_sample_data_open_native_sample (sample->sample_data, &handle,
                                                  'r', FORMAT_16BIT,
                                                  IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
        return (FALSE);

      size = ipatch_sample_handle_get_max_frames (&handle);
      samsize = ipatch_sample_get_size ((IpatchSample *)(sample->sample_data), NULL);

      for (ofs = 0; ofs < samsize; ofs += size)	/* loop while data to store */
	{
	  if (samsize - ofs < size) /* check for last partial fragment */
	    size = samsize - ofs;

	  /* read and transform (if necessary) audio data from sample store */
	  if (!(buf = ipatch_sample_handle_read (&handle, ofs, size, NULL, err)))
	    {
              ipatch_sample_handle_close (&handle);     /* -- close sample handle */
	      return (FALSE);
	    }

	  /* write 16 bit mono sample data to SoundFont file */
	  if (!ipatch_file_write (riff->handle, buf, size * 2, err))
	    {
              ipatch_sample_handle_close (&handle);     /* -- close sample handle */
	      return (FALSE);
	    }
	}

      ipatch_sample_handle_close (&handle);     /* -- close sample handle */

      /* 46 "zero" samples following sample as per SoundFont spec */
      if (!ipatch_file_write (riff->handle, &zerobuf, 46 * 2, err))
	return (FALSE);
    }

  if (!ipatch_riff_close_chunk (riff, -1, err)) return (FALSE);
  /* </smpl> */

  return (TRUE);
}

/* save sample data (24 bit mode) */
static gboolean
sfont_write_samples24 (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Sample *sample;
  IpatchSampleHandle handle;
  IpatchSF2File *file = IPATCH_SF2_FILE (riff->handle->file);
  SampleHashValue *sample_hash_value;
  IpatchIter iter;
  gpointer buf;
  guint8 *lsbuf = NULL;			/* init to NULL for error: return */
  guint lsbuf_size = 0;
  guint8 zerobuf[46 * 2]; /* 46 zero values to write at end of each sample */
  guint samsize, size, start, ofs, total_size, totalofs = 0;
  guint index = 0;
  int i;

  memset (&zerobuf, 0, sizeof (zerobuf));

  /* <smpl> - Sample data sub chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_SUB,
				IPATCH_SFONT_FOURCC_SMPL, err))
    goto error;

  /* set the sample position in the IpatchSF2File */
  ipatch_sf2_file_set_sample_pos (file, ipatch_riff_get_position (riff));

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_SAMPLE);

  /* calc total size of smpl chunk, so we can write sm24 chunk simultaneously */
  sample = ipatch_sf2_sample_first (&iter);

  for (total_size = 0; sample; sample = ipatch_sf2_sample_next (&iter))
    { /* ignore ROM samples */
      if (ipatch_item_get_flags (sample) & IPATCH_SF2_SAMPLE_FLAG_ROM) continue;

      /* size of audio in samples + 46 silent samples */
      total_size += ipatch_sample_get_size ((IpatchSample *)sample, NULL) + 46;
    }

  /* seek to end of smpl chunk (sample data written later) */
  if (!ipatch_file_seek (riff->handle, total_size * 2, G_SEEK_CUR, err))
    return (FALSE);

  if (!ipatch_riff_close_chunk (riff, -1, err)) goto error;
  /* </smpl> */

  /* <sm24> - LS bytes of 24 bit sample data sub chunk */
  if (!ipatch_riff_write_chunk (riff, IPATCH_RIFF_CHUNK_SUB,
				IPATCH_SFONT_FOURCC_SM24, err))
    return (FALSE);

  /* set the sample 24 position in the IpatchSF2File */
  ipatch_sf2_file_set_sample24_pos (file, ipatch_riff_get_position (riff));

  /* allocate extra buffer to store LS bytes of 24 bit samples */
  lsbuf = g_malloc (IPATCH_SAMPLE_COPY_BUFFER_SIZE);

  /* traverse samples */
  for (sample = ipatch_sf2_sample_first (&iter); sample;
       sample = ipatch_sf2_sample_next (&iter))
    {
      /* add sample info to sample hash */
      sample_hash_value = ipatch_sf2_writer_sample_hash_value_new ();
      sample_hash_value->index = index++;
      g_hash_table_insert (writer->sample_hash, sample, sample_hash_value);

      /* ignore ROM samples */
      if (ipatch_item_get_flags (sample) & IPATCH_SF2_SAMPLE_FLAG_ROM) continue;

      /* ++ open sample handle */
      if (!ipatch_sample_data_open_native_sample (sample->sample_data, &handle,
                                                  'r', FORMAT_24BIT,
                                                  IPATCH_SAMPLE_UNITY_CHANNEL_MAP, err))
        goto error;

      size = ipatch_sample_handle_get_max_frames (&handle);
      samsize = ipatch_sample_get_size ((IpatchSample *)(sample->sample_data), NULL);

      /* Allocate/reallocate 24 bit LSB buffer (1 byte per 24 bit sample) */
      if (size > lsbuf_size)
      {
        lsbuf = g_realloc (lsbuf, size);
        lsbuf_size = size;
      }

      /* start offset in samples of this sample data */
      start = totalofs;
      sample_hash_value->position = file->sample_pos + start * 2;
      sample_hash_value->position24 = file->sample24_pos + start;

      /* loop while data to store */
      for (ofs = 0; ofs < samsize; ofs += size, totalofs += size)
        {
      	  if (samsize - ofs < size) /* check for last partial fragment */
	    size = samsize - ofs;

	  /* read and transform (if necessary) audio data from sample store */
	  if (!(buf = ipatch_sample_handle_read (&handle, ofs, size, NULL, err)))
            goto error_close_handle;

	  /* copy the LS bytes of the 24 bit samples */
	  for (i = 0; i < size; i++)
	    ((guint8 *)lsbuf)[i] = ((guint8 *)buf)[i * 4];

	  /* compact the 16 bit portion of the 24 bit samples */
	  for (i = 0; i < size; i++)
	    {
	      ((guint8 *)buf)[i * 2] = ((guint8 *)buf)[i * 4 + 1];
	      ((guint8 *)buf)[i * 2 + 1] = ((guint8 *)buf)[i * 4 + 2];	      
	    }

	  /* seek to location in smpl chunk to store 16 bit data */
	  if (!ipatch_file_seek (riff->handle, file->sample_pos + totalofs * 2,
				 G_SEEK_SET, err))
            goto error_close_handle;

	  /* write 16 bit portions of 24 bit samples to file */
	  if (!ipatch_file_write (riff->handle, buf, size * 2, err))
            goto error_close_handle;

	  /* seek to location in sm24 chunk to store LS bytes of 24 bit samples */
	  if (!ipatch_file_seek (riff->handle, file->sample24_pos + totalofs,
				 G_SEEK_SET, err))
            goto error_close_handle;

	  /* write least significant 8 bits of 24 bit samples to file */
	  if (!ipatch_file_write (riff->handle, lsbuf, size, err))
            goto error_close_handle;
	}

      ipatch_sample_handle_close (&handle);     /* -- close sample handle */

      /* seek to location in smpl chunk to store 16 bit zero samples */
      if (!ipatch_file_seek (riff->handle, file->sample_pos + totalofs * 2,
			     G_SEEK_SET, err))
        goto error;

      /* 46 "zero" samples following sample as per SoundFont spec */
      if (!ipatch_file_write (riff->handle, &zerobuf, 46 * 2, err))
        goto error;

      /* seek to location in sm24 chunk to store LS bytes of zero samples */
      if (!ipatch_file_seek (riff->handle, file->sample24_pos + totalofs,
			     G_SEEK_SET, err))
        goto error;

      /* 46 "zero" samples following sample as per SoundFont spec */
      if (!ipatch_file_write (riff->handle, &zerobuf, 46, err))
        goto error;

      totalofs += 46;
    }

  g_free (lsbuf);		/* free lsbuf */

  /* must set to NULL in case of error: return */
  lsbuf = NULL;

  /* seek to end of sm24 chunk */
  if (!ipatch_file_seek (riff->handle, file->sample24_pos + total_size,
			 G_SEEK_SET, err))
    goto error;

  if (!ipatch_riff_close_chunk (riff, -1, err)) goto error;
  /* </sm24> */

  return (TRUE);

error_close_handle:
  ipatch_sample_handle_close (&handle);     /* -- close sample handle */

error:
  g_free (lsbuf);			/* free lsbuf */

  return (FALSE);
}

/* save preset headers */
static gboolean
sfont_write_phdrs (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Preset *preset;
  IpatchSF2Phdr phdr;
  IpatchIter iter, zone_iter;
  guint16 pbagndx = 0;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_PRESET);
  preset = ipatch_sf2_preset_first (&iter);
  while (preset)		/* loop over all presets */
    {
      strncpy (phdr.name, preset->name, IPATCH_SFONT_NAME_SIZE);
      phdr.program = preset->program;
      phdr.bank = preset->bank;
      phdr.bag_index = pbagndx;
      phdr.library = preset->library;
      phdr.genre = preset->genre;
      phdr.morphology = preset->morphology;

      ipatch_sf2_write_phdr (riff->handle, &phdr);
      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

      /* get count of preset zones */
      ipatch_container_init_iter ((IpatchContainer *)preset, &zone_iter,
				  IPATCH_TYPE_SF2_PZONE);
      pbagndx += ipatch_iter_count (&zone_iter);

      /* if any global generators or modulators then add 1 for global zone */
      if (preset->genarray.flags || preset->mods) pbagndx++;

      preset = ipatch_sf2_preset_next (&iter); /* next preset */
    }

  /* create terminal record */
  memset (&phdr, 0, sizeof (phdr));
  strcpy (phdr.name, "EOP");
  phdr.bag_index = pbagndx;

  ipatch_sf2_write_phdr (riff->handle, &phdr);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* save preset bags */
static gboolean
sfont_write_pbags (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Preset *preset;
  IpatchSF2Zone *zone;
  IpatchSF2Bag bag;
  IpatchIter iter, zone_iter;
  guint16 genndx = 0, modndx = 0;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_PRESET);
  preset = ipatch_sf2_preset_first (&iter);
  while (preset)		/* traverse through presets */
    {
      ipatch_container_init_iter ((IpatchContainer *)preset, &zone_iter,
				  IPATCH_TYPE_SF2_PZONE);

      /* process global zone if any global modulators or generators */
      if (preset->genarray.flags || preset->mods) zone = NULL;
      else zone = ipatch_sf2_zone_first (&zone_iter);

      do
	{
	  bag.gen_index = genndx;
	  bag.mod_index = modndx;
	  ipatch_sf2_write_bag (riff->handle, &bag);
	  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

	  if (zone)
	    {
	      genndx += ipatch_sf2_gen_array_count_set (&zone->genarray);
	      if (zone->item) genndx++; /* increment for INSTRUMENT_ID */

	      modndx += g_slist_length (zone->mods);
	    }
	  else	/* after global zone */
	    {
	      genndx += ipatch_sf2_gen_array_count_set (&preset->genarray);
	      modndx += g_slist_length (preset->mods);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter);
	}
      while (zone);		/* traverse preset's zones */

      preset = ipatch_sf2_preset_next (&iter);
    }

  /* terminal record */
  bag.gen_index = genndx;
  bag.mod_index = modndx;
  ipatch_sf2_write_bag (riff->handle, &bag);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* save preset modulators */
static gboolean
sfont_write_pmods (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Preset *preset;
  IpatchSF2Zone *zone;
  IpatchSF2Mod *mod;
  IpatchIter iter, zone_iter;
  GSList *p;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_PRESET);
  preset = ipatch_sf2_preset_first (&iter);
  while (preset)		/* traverse through all presets */
    {
      zone = NULL;
      p = preset->mods;	/* first is the global modulators */

      ipatch_container_init_iter ((IpatchContainer *)preset, &zone_iter,
				  IPATCH_TYPE_SF2_PZONE);
      do
	{
	  while (p)		/* save zone's modulators */
	    {
	      mod = (IpatchSF2Mod *)(p->data);
	      ipatch_sf2_write_mod (riff->handle, mod);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	      p = g_slist_next (p);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter);

	  if (zone) p = zone->mods;
	}
      while (zone);		/* traverse this preset's zones */

      preset = ipatch_sf2_preset_next (&iter);
    }

  /* terminal record */
  ipatch_file_buf_zero (riff->handle, IPATCH_SFONT_MOD_SIZE);
  if (!ipatch_file_buf_commit (riff->handle, err))
    return (FALSE);

  return (TRUE);
}

/* save preset generators */
static gboolean
sfont_write_pgens (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Preset *preset;
  IpatchSF2Zone *zone;
  IpatchSF2GenArray *genarray;
  IpatchSF2GenAmount amount;
  IpatchIter iter, zone_iter;
  guint inst_index;
  guint64 flags;
  int i;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_PRESET);
  preset = ipatch_sf2_preset_first (&iter);
  while (preset)		/* traverse through all presets */
    { /* global zone */
      genarray = &preset->genarray;
      zone = NULL;

      ipatch_container_init_iter ((IpatchContainer *)preset, &zone_iter,
				  IPATCH_TYPE_SF2_PZONE);
      do
	{
	  if (IPATCH_SF2_GEN_ARRAY_TEST_FLAG /* note range set? */
	      (genarray, IPATCH_SF2_GEN_NOTE_RANGE))
	    {
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_NOTE_RANGE,
				    &genarray->values [IPATCH_SF2_GEN_NOTE_RANGE]);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  if (IPATCH_SF2_GEN_ARRAY_TEST_FLAG /* velocity range set? */
	      (genarray, IPATCH_SF2_GEN_VELOCITY_RANGE))
	    {
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_VELOCITY_RANGE,
				    &genarray->values [IPATCH_SF2_GEN_VELOCITY_RANGE]);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  /* clear the note range and velocity since already saved */
	  flags = genarray->flags
	    & ~(IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_NOTE_RANGE)
		| IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_VELOCITY_RANGE));

	  /* set the rest of the generators */
	  for (i = 0; flags != 0; i++, flags >>= 1)
	    {
	      if (flags & 0x1)	/* generator set? */
		{
		  ipatch_sf2_write_gen (riff->handle, i, &genarray->values[i]);
		  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
		}
	    }

	  /* save instrument ID if any */
	  if (zone && zone->item)
	    {
	      inst_index = GPOINTER_TO_UINT
		(g_hash_table_lookup (writer->inst_hash, zone->item));
	      g_return_val_if_fail (inst_index != 0, FALSE);
	      inst_index--;	/* index + 1 (to catch NULL), so decrement */

	      amount.uword = inst_index;
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_INSTRUMENT_ID,
				    &amount);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter); /* next zone */

	  if (zone) genarray = &zone->genarray;
	}
      while (zone);		/* traverse preset's zones */

      preset = ipatch_sf2_preset_next (&iter); /* next preset */
    }

  /* terminal record */
  ipatch_file_buf_zero (riff->handle, IPATCH_SFONT_GEN_SIZE);
  if (!ipatch_file_buf_commit (riff->handle, err))
    return (FALSE);

  return (TRUE);
}

/* save instrument headers */
static gboolean
sfont_write_ihdrs (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Inst *inst;
  IpatchSF2Ihdr ihdr;
  IpatchIter iter, zone_iter;
  guint16 ibagndx = 0;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_INST);
  inst = ipatch_sf2_inst_first (&iter);
  while (inst)			/* loop over all instruments */
    {
      strncpy (ihdr.name, inst->name, IPATCH_SFONT_NAME_SIZE);
      ihdr.bag_index = ibagndx;
      ipatch_sf2_write_ihdr (riff->handle, &ihdr);
      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

      ipatch_container_init_iter ((IpatchContainer *)inst, &zone_iter,
				  IPATCH_TYPE_SF2_IZONE);
      ibagndx += ipatch_iter_count (&zone_iter);

      /* if any global generators or modulators then add 1 for global zone */
      if (inst->genarray.flags || inst->mods) ibagndx++;

      inst = ipatch_sf2_inst_next (&iter);
    }

  /* terminal record */
  memset (&ihdr, 0, sizeof (ihdr));
  strcpy (ihdr.name, "EOI");
  ihdr.bag_index = ibagndx;
  ipatch_sf2_write_ihdr (riff->handle, &ihdr);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* save instrument bags */
static gboolean
sfont_write_ibags (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Inst *inst;
  IpatchSF2Zone *zone;
  IpatchSF2Bag bag;
  IpatchIter iter, zone_iter;
  guint16 genndx = 0, modndx = 0;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_INST);
  inst = ipatch_sf2_inst_first (&iter);
  while (inst)			/* traverse through instruments */
    {
      ipatch_container_init_iter ((IpatchContainer *)inst, &zone_iter,
				  IPATCH_TYPE_SF2_IZONE);

      /* process global zone if any global modulators or generators */
      if (inst->genarray.flags || inst->mods) zone = NULL;
      else zone = ipatch_sf2_zone_first (&zone_iter);

      do
	{
	  bag.gen_index = genndx;
	  bag.mod_index = modndx;
	  ipatch_sf2_write_bag (riff->handle, &bag);
	  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

	  if (zone)
	    {
	      genndx += ipatch_sf2_gen_array_count_set (&zone->genarray);
	      if (zone->item) genndx++; /* increment for SAMPLE_ID */
    
	      modndx += g_slist_length (zone->mods);
	    }
	  else	/* after global zone */
	    {
	      genndx += ipatch_sf2_gen_array_count_set (&inst->genarray);
	      modndx += g_slist_length (inst->mods);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter);
	}
      while (zone);		/* traverse instrument's zones */

      inst = ipatch_sf2_inst_next (&iter);
    }

  /* terminal record */
  bag.gen_index = genndx;
  bag.mod_index = modndx;
  ipatch_sf2_write_bag (riff->handle, &bag);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}

/* save instrument modulators */
static gboolean
sfont_write_imods (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Inst *inst;
  IpatchSF2Zone *zone;
  IpatchSF2Mod *mod;
  IpatchIter iter, zone_iter;
  GSList *p;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_INST);
  inst = ipatch_sf2_inst_first (&iter);
  while (inst)			/* traverse through all instruments */
    {
      zone = NULL;
      p = inst->mods;	/* first is the global modulators */

      ipatch_container_init_iter ((IpatchContainer *)inst, &zone_iter,
				  IPATCH_TYPE_SF2_IZONE);
      do
	{
	  while (p)		/* save modulators */
	    {
	      mod = (IpatchSF2Mod *)(p->data);
	      ipatch_sf2_write_mod (riff->handle, mod);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	      p = g_slist_next (p);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter);

	  if (zone) p = zone->mods;
	}
      while (zone);		/* traverse this instrument's zones */

      inst = ipatch_sf2_inst_next (&iter);
    }

  /* terminal record */
  ipatch_file_buf_zero (riff->handle, IPATCH_SFONT_MOD_SIZE);
  if (!ipatch_file_buf_commit (riff->handle, err))
    return (FALSE);

  return (TRUE);
}

/* save instrument generators */
static gboolean
sfont_write_igens (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2Inst *inst;
  IpatchSF2Zone *zone;
  IpatchSF2GenArray *genarray;
  IpatchSF2GenAmount amount;
  IpatchIter iter, zone_iter;
  SampleHashValue *sample_hash_value;
  guint64 flags;
  int i;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_INST);
  inst = ipatch_sf2_inst_first (&iter);
  while (inst)			/* traverse through all instruments */
    {
      /* global zone */
      genarray = &inst->genarray;
      zone = NULL;

      ipatch_container_init_iter ((IpatchContainer *)inst, &zone_iter,
				  IPATCH_TYPE_SF2_IZONE);
      do
	{
	  if (IPATCH_SF2_GEN_ARRAY_TEST_FLAG /* note range set? */
	      (genarray, IPATCH_SF2_GEN_NOTE_RANGE))
	    {
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_NOTE_RANGE,
				    &genarray->values [IPATCH_SF2_GEN_NOTE_RANGE]);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  if (IPATCH_SF2_GEN_ARRAY_TEST_FLAG /* velocity range set? */
	      (genarray, IPATCH_SF2_GEN_VELOCITY_RANGE))
	    {
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_VELOCITY_RANGE,
				    &genarray->values [IPATCH_SF2_GEN_VELOCITY_RANGE]);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  /* clear the note range and velocity since already saved */
	  flags = genarray->flags
	    & ~(IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_NOTE_RANGE)
		| IPATCH_SF2_GENID_SET (IPATCH_SF2_GEN_VELOCITY_RANGE));

	  /* set the rest of the generators */
	  for (i = 0; flags != 0; i++, flags >>= 1)
	    {
	      if (flags & 0x1)	/* generator set? */
		{
		  ipatch_sf2_write_gen (riff->handle, i, &genarray->values[i]);
		  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
		}
	    }

	  /* save sample ID if any */
	  if (zone && zone->item)
	    {
	      sample_hash_value = g_hash_table_lookup (writer->sample_hash, zone->item);
	      g_return_val_if_fail (sample_hash_value != NULL, FALSE);

	      amount.uword = sample_hash_value->index;
	      ipatch_sf2_write_gen (riff->handle, IPATCH_SF2_GEN_SAMPLE_ID, &amount);
	      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
	    }

	  if (!zone) zone = ipatch_sf2_zone_first (&zone_iter);	/* after global zone */
	  else zone = ipatch_sf2_zone_next (&zone_iter); /* next zone */

	  if (zone) genarray = &zone->genarray;
	}
      while (zone);		/* traverse instrument's zones */

      inst = ipatch_sf2_inst_next (&iter); /* next instrument */
    }

  /* terminal record */
  ipatch_file_buf_zero (riff->handle, IPATCH_SFONT_GEN_SIZE);
  if (!ipatch_file_buf_commit (riff->handle, err))
    return (FALSE);

  return (TRUE);
}

/* save sample headers */
static gboolean
sfont_write_shdrs (IpatchSF2Writer *writer, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (writer);
  IpatchSF2File *sf2file = IPATCH_SF2_FILE (riff->handle->file);
  SampleHashValue *sample_hash_value;
  IpatchSF2Sample *sample, *linked;
  IpatchIter iter;
  IpatchSampleStore *store;
  IpatchSF2Shdr shdr;
  guint start;
  int location;
  int untitled = 0;

  ipatch_container_init_iter (IPATCH_CONTAINER (writer->sf), &iter,
			      IPATCH_TYPE_SF2_SAMPLE);

  /* traverse all samples */
  for (sample = ipatch_sf2_sample_first (&iter); sample;
       sample = ipatch_sf2_sample_next (&iter))
    {
      if (!(ipatch_item_get_flags (sample) & IPATCH_SF2_SAMPLE_FLAG_ROM))
	{
          sample_hash_value = g_hash_table_lookup (writer->sample_hash, sample);
          g_return_val_if_fail (sample_hash_value != NULL, FALSE);
	  start = sample_hash_value->position;
	  start -= sf2file->sample_pos;	/* use offset from start of samples */
          shdr.type = 0;
	}
      else			/* ROM sample */
	{
          store = ipatch_sample_data_get_native_sample (sample->sample_data);
	  g_return_val_if_fail (store != NULL, FALSE);
	  g_object_get (store, "location", &location, NULL);
          start = location;
          shdr.type = IPATCH_SF2_FILE_SAMPLE_TYPE_ROM;
	}

      memset (shdr.name, 0, IPATCH_SFONT_NAME_SIZE);
      if (sample->name)
	strncpy (shdr.name, sample->name, IPATCH_SFONT_NAME_SIZE - 1);
      else
	sprintf (shdr.name, _("untitled-%d"), ++untitled); /* i18n: Should be less than 16 chars! */

      start /= 2;		/* convert start from bytes to samples */

      shdr.start = start;
      shdr.end = ipatch_sample_get_size ((IpatchSample *)sample, NULL) + start;
      shdr.loop_start = sample->loop_start + start;
      shdr.loop_end = sample->loop_end + start;
      shdr.rate = sample->rate;
      shdr.root_note = sample->root_note;
      shdr.fine_tune = sample->fine_tune;

      switch (sample->channel)
      {
        case IPATCH_SF2_SAMPLE_CHANNEL_LEFT:
          shdr.type |= IPATCH_SF2_FILE_SAMPLE_TYPE_LEFT;
          break;
        case IPATCH_SF2_SAMPLE_CHANNEL_RIGHT:
          shdr.type |= IPATCH_SF2_FILE_SAMPLE_TYPE_RIGHT;
          break;
        default:
          shdr.type |= IPATCH_SF2_FILE_SAMPLE_TYPE_MONO;
          break;
      }

      shdr.link_index = 0;

      linked = ipatch_sf2_sample_peek_linked (sample);

      if (linked)
	{
	  sample_hash_value = g_hash_table_lookup (writer->sample_hash, linked);
	  g_return_val_if_fail (sample_hash_value != NULL, FALSE);
	  shdr.link_index = sample_hash_value->index;
	}

      ipatch_sf2_write_shdr (riff->handle, &shdr);
      if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);
    }

  /* terminal record */
  memset (&shdr, 0, IPATCH_SFONT_SHDR_SIZE);
  strcpy (shdr.name, "EOS");
  ipatch_sf2_write_shdr (riff->handle, &shdr);
  if (!ipatch_file_buf_commit (riff->handle, err)) return (FALSE);

  return (TRUE);
}
