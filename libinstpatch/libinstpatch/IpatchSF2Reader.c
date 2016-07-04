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
 * SECTION: IpatchSF2Reader
 * @short_description: SoundFont file reader
 * @see_also: 
 * @stability: Stable
 *
 * Reads a SoundFont file and loads it into a object tree (#IpatchSF2).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "IpatchSF2Reader.h"
#include "IpatchSF2File.h"
#include "IpatchSF2File_priv.h"
#include "IpatchSF2ModItem.h"
#include "IpatchGig.h"
#include "IpatchGigEffects.h"
#include "IpatchGigFile.h"
#include "IpatchSampleStore.h"
#include "IpatchSampleStoreRam.h"
#include "IpatchSampleStoreRom.h"
#include "IpatchSampleStoreSplit24.h"
#include "IpatchGigInst.h"
#include "IpatchGigRegion.h"
#include "IpatchSampleStoreFile.h"
#include "IpatchUnit.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* ----- ** WARNING ** -----
   We don't care about locking in here, because we are the exclusive
   owner of the loading SoundFont. Many fields are accessed directly,
   etc. This is not an example of proper use of libInstPatch..  Don't
   try this at home.. Blah blah. */

static void ipatch_sf2_reader_class_init (IpatchSF2ReaderClass *klass);
static void ipatch_sf2_reader_init (IpatchSF2Reader *reader);
static void ipatch_sf2_reader_finalize (GObject *object);

static void ipatch_sf2_load_phdr (IpatchFileHandle *handle, IpatchSF2Phdr *phdr);
static void ipatch_sf2_load_ihdr (IpatchFileHandle *handle, IpatchSF2Ihdr *ihdr);
static void ipatch_sf2_load_shdr (IpatchFileHandle *handle, IpatchSF2Shdr *shdr);
static void ipatch_sf2_load_mod (IpatchFileHandle *handle, IpatchSF2Mod *mod);
static void ipatch_sf2_load_gen (IpatchFileHandle *handle, int *genid,
                                 IpatchSF2GenAmount *amount);

static gboolean ipatch_sf2_load_level_0 (IpatchSF2Reader *reader,
					 GError **err);

static gboolean sfload_infos (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_phdrs (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_pbags (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_pmods (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_pgens (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_ihdrs (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_ibags (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_imods (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_igens (IpatchSF2Reader *reader, GError **err);
static gboolean sfload_shdrs (IpatchSF2Reader *reader, GError **err);

#define SFONT_ERROR_MSG "SoundFont reader error: %s"

#define SET_SIZE_ERROR(riff, level, err) \
  g_set_error (err, IPATCH_RIFF_ERROR, IPATCH_RIFF_ERROR_SIZE_MISMATCH,\
	       _(SFONT_ERROR_MSG), \
	       ipatch_riff_message_detail(riff, -1, "Unexpected chunk size"))

#define SET_DATA_ERROR(riff, level, err) \
  g_set_error (err, IPATCH_RIFF_ERROR, IPATCH_RIFF_ERROR_INVALID_DATA,\
	       _(SFONT_ERROR_MSG), \
	       ipatch_riff_message_detail (riff, -1, "Invalid data"))

G_DEFINE_TYPE (IpatchSF2Reader, ipatch_sf2_reader, IPATCH_TYPE_RIFF)

static void
ipatch_sf2_reader_class_init (IpatchSF2ReaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_sf2_reader_finalize;
}

static void
ipatch_sf2_reader_init (IpatchSF2Reader *reader)
{
  reader->sf = NULL;
}

static void
ipatch_sf2_reader_finalize (GObject *object)
{
  IpatchSF2Reader *reader = IPATCH_SF2_READER (object);

  if (reader->sf)
    {
      g_object_unref (reader->sf); /* -- unref SoundFont */
      reader->sf = NULL;
    }

  g_free (reader->pbag_table);
  reader->pbag_table = NULL;
  g_free (reader->ibag_table);
  reader->ibag_table = NULL;
  g_free (reader->inst_table);
  reader->inst_table = NULL;
  g_free (reader->sample_table);
  reader->sample_table = NULL;

  if (G_OBJECT_CLASS (ipatch_sf2_reader_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sf2_reader_parent_class)->finalize (object);
}

/**
 * ipatch_sf2_reader_new:
 * @handle: SoundFont 2 file handle to parse or %NULL to set later
 *
 * Create a new SoundFont file reader
 *
 * Returns: The new SoundFont file reader
 */
IpatchSF2Reader *
ipatch_sf2_reader_new (IpatchFileHandle *handle)
{
  IpatchSF2Reader *reader;

  g_return_val_if_fail (!handle || IPATCH_IS_SF2_FILE (handle->file), NULL);

  reader = g_object_new (IPATCH_TYPE_SF2_READER, NULL);
  if (handle) ipatch_sf2_reader_set_file_handle (reader, handle);

  return (reader);
}

/**
 * ipatch_sf2_reader_set_file_handle:
 * @reader: SoundFont reader object
 * @handle: SoundFont 2 file handle
 *
 * Set the SoundFont file handle of a SoundFont reader. A convenience
 * function, since ipatch_riff_set_file_handle() could also be used, albeit
 * without stricter type casting.
 */
void
ipatch_sf2_reader_set_file_handle (IpatchSF2Reader *reader, IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_SF2_READER (reader));
  g_return_if_fail (handle && IPATCH_IS_SF2_FILE (handle->file));

  ipatch_riff_set_file_handle (IPATCH_RIFF (reader), handle);
}

/**
 * ipatch_sf2_reader_load:
 * @reader: SF2 reader object
 * @err: Location to store error info or %NULL
 *
 * Load an SF2 file.
 *
 * Returns: (transfer full): New SF2 object with refcount of 1.
 */
IpatchSF2 *
ipatch_sf2_reader_load (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff;
  IpatchRiffChunk *chunk;
  GError *local_err = NULL;
  int size;
 
  g_return_val_if_fail (IPATCH_IS_SF2_READER (reader), NULL);
  riff = IPATCH_RIFF (reader);
  g_return_val_if_fail (riff->handle && IPATCH_IS_SF2_FILE (riff->handle->file), NULL);

  /* start parsing */
  if (!(chunk = ipatch_riff_start_read (riff, err)))
    return (NULL);

  if (chunk->id != IPATCH_SFONT_FOURCC_SFBK)
    {
      g_set_error (err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_UNEXPECTED_ID,
		   _("Not a SoundFont file (RIFF id = '%4s')"), chunk->idstr);
      return (NULL);
    }
 
  /* verify total size of file with RIFF chunk size */
  size = ipatch_file_get_size (riff->handle->file, &local_err);

  if (size == -1)
  {
    g_warning ("SoundFont file size check failed: %s", ipatch_gerror_message (local_err));
    g_clear_error (&local_err);
  }
  else if (size != chunk->size + IPATCH_RIFF_HEADER_SIZE)
    {
      g_set_error (err, IPATCH_RIFF_ERROR,
		   IPATCH_RIFF_ERROR_SIZE_MISMATCH,
		   _("File size mismatch (chunk size = %d, actual = %d)"),
		   chunk->size + IPATCH_RIFF_HEADER_SIZE, size);
      return (NULL);
    }

  reader->sf = ipatch_sf2_new (); /* ++ ref new object */
  ipatch_sf2_set_file (reader->sf, IPATCH_SF2_FILE (riff->handle->file));

  if (!ipatch_sf2_load_level_0 (reader, err))
    goto err;

  ipatch_item_clear_flags (IPATCH_ITEM (reader->sf),
			   IPATCH_BASE_SAVED | IPATCH_BASE_CHANGED);

  /* ++ ref for caller, finalize() will remove reader's reference */
  return (g_object_ref (reader->sf));

 err:
  g_object_unref (reader->sf);
  reader->sf = NULL;

  return (NULL);
}

/**
 * ipatch_sf2_load_phdr: (skip)
 * @handle: File handle containing buffered data
 * @phdr: Pointer to a user supplied preset header structure
 *
 * Parses a raw preset header in file @handle with buffered data.
 */
static void
ipatch_sf2_load_phdr (IpatchFileHandle *handle, IpatchSF2Phdr *phdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (phdr != NULL);

  ipatch_file_buf_read (handle, &phdr->name, IPATCH_SFONT_NAME_SIZE);
  phdr->program = ipatch_file_buf_read_u16 (handle);
  phdr->bank = ipatch_file_buf_read_u16 (handle);
  phdr->bag_index = ipatch_file_buf_read_u16 (handle);
  phdr->library = ipatch_file_buf_read_u32 (handle);
  phdr->genre = ipatch_file_buf_read_u32 (handle);
  phdr->morphology = ipatch_file_buf_read_u32 (handle);
}

/**
 * ipatch_sf2_load_ihdr: (skip)
 * @handle: File handle containing buffered data
 * @ihdr: Pointer to a user supplied instrument header structure
 *
 * Parses a raw instrument header in file @handle with buffered data.
 */
static void
ipatch_sf2_load_ihdr (IpatchFileHandle *handle, IpatchSF2Ihdr *ihdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (ihdr != NULL);

  ipatch_file_buf_read (handle, &ihdr->name, IPATCH_SFONT_NAME_SIZE);
  ihdr->bag_index = ipatch_file_buf_read_u16 (handle);
}

/**
 * ipatch_sf2_load_shdr: (skip)
 * @handle: File handle containing buffered data
 * @shdr: Pointer to a user supplied sample header structure
 *
 * Parses a raw sample header in file @handle with buffered data.
 */
static void
ipatch_sf2_load_shdr (IpatchFileHandle *handle, IpatchSF2Shdr *shdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (shdr != NULL);

  ipatch_file_buf_read (handle, &shdr->name, IPATCH_SFONT_NAME_SIZE);
  shdr->start = ipatch_file_buf_read_u32 (handle);
  shdr->end = ipatch_file_buf_read_u32 (handle);
  shdr->loop_start = ipatch_file_buf_read_u32 (handle);
  shdr->loop_end = ipatch_file_buf_read_u32 (handle);
  shdr->rate = ipatch_file_buf_read_u32 (handle);
  shdr->root_note = ipatch_file_buf_read_u8 (handle);
  shdr->fine_tune = ipatch_file_buf_read_u8 (handle);
  shdr->link_index = ipatch_file_buf_read_u16 (handle);
  shdr->type = ipatch_file_buf_read_u16 (handle);
}

/**
 * ipatch_sf2_load_mod: (skip)
 * @handle: File handle containing buffered data
 * @mod: Pointer to a user supplied modulator structure
 *
 * Parses a raw modulator in file @handle with buffered data.
 */
static void
ipatch_sf2_load_mod (IpatchFileHandle *handle, IpatchSF2Mod *mod)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (mod != NULL);

  mod->src = ipatch_file_buf_read_u16 (handle);
  mod->dest = ipatch_file_buf_read_u16 (handle);
  mod->amount = ipatch_file_buf_read_u16 (handle);
  mod->amtsrc = ipatch_file_buf_read_u16 (handle);
  mod->trans = ipatch_file_buf_read_u16 (handle);
}

/**
 * ipatch_sf2_load_gen: (skip)
 * @handle: File handle containing buffered data
 * @genid: Pointer to store the generator ID in
 * @amount: Pointer to a generator amount to store the amount in
 *
 * Parses a raw generator in file @handle with buffered data.
 */
static void
ipatch_sf2_load_gen (IpatchFileHandle *handle, int *genid,
		     IpatchSF2GenAmount *amount)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (genid != NULL);
  g_return_if_fail (amount != NULL);

  *genid = ipatch_file_buf_read_u16 (handle);

  /* check if genid is valid (preset or inst) and is a range unit */
  if (ipatch_sf2_gen_is_valid (*genid, FALSE)
      && ipatch_sf2_gen_info[*genid].unit == IPATCH_UNIT_TYPE_RANGE)
    {				/* load the range */
      amount->range.low = ipatch_file_buf_read_u8 (handle);
      amount->range.high = ipatch_file_buf_read_u8 (handle);
    }
  else amount->sword = ipatch_file_buf_read_s16 (handle);
}

static gboolean
ipatch_sf2_load_level_0 (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  IpatchSF2File *sfont_file;

  /* load INFO LIST chunk */
  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_LIST,
				      IPATCH_SFONT_FOURCC_INFO, err))
    return (FALSE);
  if (!sfload_infos (reader, err)) return (FALSE);
  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  /* load SDTA LIST chunk */
  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_LIST,
				      IPATCH_SFONT_FOURCC_SDTA, err))
    return (FALSE);

  /* initialize sample positions to 0 so we know if they get set or not */
  sfont_file = IPATCH_SF2_FILE (IPATCH_BASE (reader->sf)->file);
  sfont_file->sample_pos = 0;
  sfont_file->sample24_pos = 0;

  /* smpl chunk is theoretically optional if all samples are ROM samples, but
   * these days thats a pretty useless SoundFont, so we assume SMPL exists.  */
  if (!(chunk = ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
					      IPATCH_SFONT_FOURCC_SMPL, err)))
    return (FALSE);

  /* store offset into file of sample data (if any) */
  if (chunk->size > 0)
    {
      sfont_file->sample_pos = ipatch_riff_get_position (riff);
      sfont_file->sample_size = chunk->size / 2; /* save size (in samples) */
    }

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE); /* </SMPL> */

  chunk = ipatch_riff_read_chunk (riff, err);

  /* check for "sm24" sample chunk (LS bytes of 24 bit audio) */
  if (chunk && chunk->type == IPATCH_RIFF_CHUNK_SUB
      && chunk->id == IPATCH_SFONT_FOURCC_SM24)
    { /* verify sm24 is equal to the number of samples in <smpl> */
      if (chunk->size == sfont_file->sample_size
	  || chunk->size == sfont_file->sample_size
	     + (sfont_file->sample_size & 1))
      { /* set 24 bit samples flag and store offset of chunk */
	ipatch_item_set_flags (reader->sf, IPATCH_SF2_SAMPLES_24BIT);
	sfont_file->sample24_pos = ipatch_riff_get_position (riff);
      }
      else
	g_critical ("Invalid size for SoundFont sample 24 chunk, ignoring");

      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE); /* </SM24> */
    }
  else if (!ipatch_riff_get_error (riff, NULL))
    return (FALSE);

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE); /* </SDTA> */

  /* load PDTA LIST chunk */
  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_LIST,
				      IPATCH_SFONT_FOURCC_PDTA, err))
    return (FALSE);
  if (!sfload_phdrs (reader, err)) return (FALSE);

  /* we load all presets/instruments/samples first so no fixup is required
     later for numeric indexes. Save RIFF state here so we can return. */
  ipatch_riff_push_state (riff);

  if (!ipatch_riff_skip_chunks (riff, 3, err)) /* skip pbags pmods pgens */
    return (FALSE);
  if (!sfload_ihdrs (reader, err)) return (FALSE);

  if (!ipatch_riff_skip_chunks (riff, 3, err)) /* skip ibags imods igens */
    return (FALSE);
  if (!sfload_shdrs (reader, err)) return (FALSE);

  /* return to preset bag chunk */
  if (!ipatch_riff_pop_state (riff, err)) return (FALSE);

  if (!sfload_pbags (reader, err)) return (FALSE);
  if (!sfload_pmods (reader, err)) return (FALSE);
  if (!sfload_pgens (reader, err)) return (FALSE);

  /* skip ihdrs (already loaded above) */
  if (!ipatch_riff_skip_chunks (riff, 1, err)) return (FALSE);

  if (!sfload_ibags (reader, err)) return (FALSE);
  if (!sfload_imods (reader, err)) return (FALSE);
  if (!sfload_igens (reader, err)) return (FALSE);

  return (TRUE);
}

static gboolean
sfload_infos (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;

  /* loop over info chunks */
  while ((chunk = ipatch_riff_read_chunk (riff, err)))
    {
      if (chunk->type == IPATCH_RIFF_CHUNK_SUB)
	{
	  if (chunk->id == IPATCH_SFONT_FOURCC_IFIL) /* version chunk? */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_VERSION_SIZE,
					 err))
		return (FALSE);

	      reader->sf->ver_major = ipatch_file_buf_read_u16 (riff->handle);
	      reader->sf->ver_minor = ipatch_file_buf_read_u16 (riff->handle);

	      if (reader->sf->ver_major != 2)
		{
		  g_critical (_("SoundFont version is %d.%02d which"
				" is not supported"),
			      reader->sf->ver_major, reader->sf->ver_minor);
		  return (FALSE);
		}

	      if (reader->sf->ver_minor > 4)
		g_warning (_("SoundFont version is newer than 2.04,"
			     " some information might be uneditable"));
	    }
	  else if (chunk->id == IPATCH_SFONT_FOURCC_IVER) /* ROM version? */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_VERSION_SIZE,
					 err))
		return (FALSE);

	      reader->sf->romver_major = ipatch_file_buf_read_u16 (riff->handle);
	      reader->sf->romver_minor = ipatch_file_buf_read_u16 (riff->handle);
	    }
	  else if (ipatch_sf2_info_id_is_valid (chunk->id))	/* regular string based info chunk */
	    {
	      int maxsize, size;
	      char *s;

	      if (chunk->size > 0)
		{
		  maxsize = ipatch_sf2_get_info_max_size (chunk->id);

		  if (chunk->size > maxsize)
		    {
		      g_warning (_("Invalid size %d for INFO chunk \"%.4s\""),
				 chunk->size, chunk->idstr);
		      size = maxsize;
		    }
		  else size = chunk->size;

		  s = g_malloc (size);   /* alloc for info string */

		  if (!ipatch_file_read (riff->handle, s, size, err))
		    {
		      g_free (s);
		      return (FALSE);
		    }

		  s[size - 1] = '\0'; /* force terminate info string */
		  ipatch_sf2_set_info (reader->sf, chunk->id, s);
		  g_free (s);
		}
	    }
	    else g_warning (_("Unknown INFO chunk \"%.4s\""), chunk->idstr);
	} /* chunk->type == IPATCH_RIFF_CHUNK_SUB */
      if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);
    } /* while () */

  return (ipatch_riff_get_error (riff, NULL));
}

/* preset header reader */
static gboolean
sfload_phdrs (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Preset *preset = NULL, *prev = NULL; /* current & previous preset item */
  IpatchIter preset_iter, zone_iter;
  IpatchItem *zone;
  IpatchRiffChunk *chunk;
  IpatchSF2Phdr phdr;
  guint16 zndx = 0, pzndx = 0;
  int i, i2;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_PHDR, err))
    return (FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size == 0) return (TRUE); /* no preset headers? */
  if (chunk->size % IPATCH_SFONT_PHDR_SIZE) /* verify chunk size */
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  /* initialize iterator to SoundFont preset list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->sf), &preset_iter,
			      IPATCH_TYPE_SF2_PRESET);

  /* loop over all preset headers (including dummy terminal record) */
  i = chunk->size / IPATCH_SFONT_PHDR_SIZE;
  for (; i > 0; i--)
    {
      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_PHDR_SIZE, err))
	return (FALSE);

      ipatch_sf2_load_phdr (riff->handle, &phdr);
      zndx = phdr.bag_index;

      if (i != 1)		/* don't add terminal record */
	{
	  preset = ipatch_sf2_preset_new (); /* ++ ref new preset */

	  preset->name = g_strndup (phdr.name, 20);
	  preset->program = phdr.program;
	  preset->bank = phdr.bank;
	  preset->library = phdr.library;
	  preset->genre = phdr.genre;
	  preset->morphology = phdr.morphology;

	  /* by default insert_iter keeps appending */
	  ipatch_container_insert_iter ((IpatchContainer *)(reader->sf),
					(IpatchItem *)preset, &preset_iter);
	  g_object_unref (preset); /* -- unref new preset */
	}

      if (prev)		/* not first preset? */
	{
	  if (zndx < pzndx)	/* make sure zone index isn't decreasing */
	    {
	      g_set_error (err, IPATCH_RIFF_ERROR,
			   IPATCH_RIFF_ERROR_INVALID_DATA,
			   "Invalid preset zone index");
	      return (FALSE);
	    }

	  /* init iterator to list of zones */
	  ipatch_container_init_iter ((IpatchContainer *)prev, &zone_iter,
				      IPATCH_TYPE_SF2_PZONE);
	  i2 = zndx - pzndx;	/* # of zones in last preset */
	  while (i2--)		/* create zones for last preset */
	    { /* ++ ref new zone and insert it */
	      zone = g_object_new (IPATCH_TYPE_SF2_PZONE, NULL);
	      ipatch_container_insert_iter ((IpatchContainer *)prev,
					    zone, &zone_iter);
	      g_object_unref (zone); /* -- unref new zone */
	    }
	}
      else if (zndx > 0)	/* 1st preset, warn if ofs > 0 */
	g_warning (_("%d preset zones not referenced, discarding"), zndx);

      prev = preset;		/* update previous preset ptr */
      pzndx = zndx;
    }

  reader->pbag_count = zndx;	/* total number of preset zones */

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* preset bag reader */
static gboolean
sfload_pbags (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  guint16 *bag_table;
  guint16 genndx, modndx;
  guint16 pgenndx, pmodndx;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_PBAG, err))
    return (FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size % IPATCH_SFONT_BAG_SIZE
      || chunk->size / IPATCH_SFONT_BAG_SIZE != reader->pbag_count + 1)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  bag_table = reader->pbag_table = g_malloc (chunk->size);
  if (!ipatch_file_read (riff->handle, bag_table, chunk->size, err))
    return (FALSE);	   /* bag_table will be freed by finalize() */

  pgenndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[0]);
  pmodndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[1]);

  for (i=0; i < reader->pbag_count; i++)
    {
      genndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[(i+1)*2]);
      modndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[(i+1)*2+1]);

      if (genndx < pgenndx)
	{
	  g_set_error (err, IPATCH_RIFF_ERROR,
		       IPATCH_RIFF_ERROR_INVALID_DATA,
		       "Invalid preset gen index");
	  return (FALSE);
	}
      if (modndx < pmodndx)
	{
	  g_set_error (err, IPATCH_RIFF_ERROR,
		       IPATCH_RIFF_ERROR_INVALID_DATA,
		       "Invalid preset mod index");
	  return (FALSE);
	}

      bag_table[i*2] = genndx - pgenndx;
      bag_table[i*2+1] = modndx - pmodndx;

      pgenndx = genndx;		/* update previous zone gen index */
      pmodndx = modndx;		/* update previous zone mod index */
    }

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* preset modulator reader */
static gboolean
sfload_pmods (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Zone *zone;
  IpatchSF2Mod mod;
  GSList *p, *p2;
  guint bagmod_index = 1;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_PMOD, err))
    return (FALSE);

  p = reader->sf->presets;
  while (p)			/* traverse through all presets */
    {
      p2 = IPATCH_SF2_PRESET (p->data)->zones;
      while (p2)		/* traverse this preset's zones */
	{
	  zone = IPATCH_SF2_ZONE (p2->data);

	  /* get stored modulator count for zone bag table */
	  i = reader->pbag_table[bagmod_index];
	  bagmod_index += 2;

	  while (i-- > 0)	/* load modulators */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_MOD_SIZE,
					 err)) return (FALSE);
	      ipatch_sf2_load_mod (riff->handle, &mod);
	      ipatch_sf2_mod_item_add ((IpatchSF2ModItem *)zone, &mod);
	    }
	  p2 = g_slist_next (p2);
	}
      p = g_slist_next (p);
    }

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* preset generator reader */
static gboolean
sfload_pgens (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Preset *preset;
  IpatchSF2Zone *zone;
  IpatchSF2GenAmount amount;
  int genid;
  GSList *p, *p2;
  guint pbag_index = 0;
  guint index;
  int level, discarded;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_PGEN, err))
    return (FALSE);

  p = reader->sf->presets;
  while (p)			/* traverse through all presets */
    {
      discarded = FALSE;

      preset = IPATCH_SF2_PRESET (p->data);
      p2 = preset->zones;
      while (p2)		/* traverse preset's zones */
	{
	  level = 0;
	  zone = IPATCH_SF2_ZONE (p2->data);

	  /* retrieve our stored gen count (from load_pbag) */
	  i = reader->pbag_table[pbag_index];
	  pbag_index += 2;

	  while (i-- > 0)	/* load zone's generators */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_GEN_SIZE,
					 err)) return (FALSE);
	      ipatch_sf2_load_gen (riff->handle, &genid, &amount);

	      /* check validity of generator */
	      if ((genid != IPATCH_SF2_GEN_INSTRUMENT_ID
		   && !ipatch_sf2_gen_is_valid (genid, TRUE))
		  || (genid == IPATCH_SF2_GEN_NOTE_RANGE && level != 0)
		  || (genid == IPATCH_SF2_GEN_VELOCITY_RANGE && level > 1))
		{
		  discarded = TRUE;
		  continue;
		}

	      /* IPATCH_SF2_GEN_NOTE_RANGE first (if any) followed by
		 IPATCH_SF2_GEN_VELOCITY_RANGE (if any),
		 IPATCH_SF2_GEN_INSTRUMENT_ID is last */
	      if (genid == IPATCH_SF2_GEN_NOTE_RANGE) level = 1;
	      else if (genid == IPATCH_SF2_GEN_VELOCITY_RANGE) level = 2;
	      else if (genid == IPATCH_SF2_GEN_INSTRUMENT_ID)
		{
		  index = amount.uword;
		  if (index >= reader->inst_count)
		    g_warning (_("Invalid reference in preset '%s'"),
			       preset->name); /* will get discarded below */
		  else ipatch_sf2_zone_set_link_item (zone, (IpatchItem *)
						      reader->inst_table[index]);
		  level = 3;
		  break;	/* break out of gen loop */
		}
	      else level = 2;

	      /* set the generator */
	      zone->genarray.values[genid] = amount;
	      IPATCH_SF2_GEN_ARRAY_SET_FLAG (&zone->genarray, genid);
	    } /* generator loop */

	  /* ignore (skip) any generators following an instrument ID */
	  while (i-- > 0)
	    {
	      discarded = TRUE;
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_GEN_SIZE,
					 err)) return (FALSE);
	    }

	  /* if level != 3 (no instrument ID) and not first zone, discard */
	  if (level != 3 && p2 != preset->zones)
	    {			/* discard invalid global zone */
	      IpatchItem *item = IPATCH_ITEM (p2->data);
	      p2 = g_slist_next (p2);
	      ipatch_container_remove (IPATCH_CONTAINER (preset), item);

	      g_warning (_("Preset \"%s\": Discarding invalid global zone"),
			 preset->name);
	      continue;
	    }
	  p2 = g_slist_next (p2); /* next zone */
	}

      /* global zone?  Migrate gens/mods to preset and remove zone. */
      if (preset->zones && !((IpatchSF2Zone *)(preset->zones->data))->item)
	{
	  IpatchSF2Zone *zone = (IpatchSF2Zone *)(preset->zones->data);

	  preset->genarray = zone->genarray;

	  preset->mods = zone->mods;	/* snatch modulator list */
	  zone->mods = NULL;

	  ipatch_container_remove (IPATCH_CONTAINER (preset), (IpatchItem *)zone);
	}

      if (discarded)
	g_warning (_("Preset \"%s\": Some invalid generators were discarded"),
		   preset->name);

      p = g_slist_next (p);
    }

  /* free some no longer needed tables */
  g_free (reader->pbag_table);
  reader->pbag_table = NULL;
  g_free (reader->inst_table);
  reader->inst_table = NULL;

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* instrument header reader */
static gboolean
sfload_ihdrs (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Inst *inst = NULL, *prev = NULL; /* current & previous instrument */
  IpatchIter inst_iter, zone_iter;
  IpatchItem *zone;
  IpatchRiffChunk *chunk;
  IpatchSF2Inst **iptr;
  IpatchSF2Ihdr ihdr;
  guint16 zndx=0, pzndx=0;
  int i, i2;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_INST, err))
    return (FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size == 0) return (TRUE); /* no instrument headers? */
  if (chunk->size % IPATCH_SFONT_INST_SIZE) /* verify chunk size */
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  /* initialize iterator to instrument list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->sf), &inst_iter,
			      IPATCH_TYPE_SF2_INST);

  i = chunk->size / IPATCH_SFONT_INST_SIZE; /* instrument count + 1 EOI */
  reader->inst_count = i - 1;

  /* allocate instrument table (used for PresetZone fixups) */
  iptr = reader->inst_table = g_malloc ((i - 1) * sizeof (IpatchSF2Inst *));

  /* loop over all instrument headers (including dummy terminal record) */
  for (; i > 0; i--)
    {
      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_INST_SIZE, err))
	return (FALSE);

      ipatch_sf2_load_ihdr (riff->handle, &ihdr);
      zndx = ihdr.bag_index;

      if (i != 1)		/* don't add terminal record, free instead */
	{ /* ++ ref new inst and add to instrument fixup table */
	  *(iptr++) = inst = ipatch_sf2_inst_new ();
	  inst->name = g_strndup (ihdr.name, 20);

	  /* insert the instrument (default is to keep appending) */
	  ipatch_container_insert_iter ((IpatchContainer *)(reader->sf),
					(IpatchItem *)inst, &inst_iter);
	  g_object_unref (inst); /* -- unref new instrument */
	}

      if (prev)			/* not first instrument? */
	{
	  if (zndx < pzndx)	/* make sure zone index isn't decreasing */
	    {
	      g_set_error (err, IPATCH_RIFF_ERROR,
			   IPATCH_RIFF_ERROR_INVALID_DATA,
			   "Invalid instrument zone index");
	      return (FALSE);
	    }

	  /* initialize iterator to instrument zone list */
	  ipatch_container_init_iter ((IpatchContainer *)prev, &zone_iter,
				      IPATCH_TYPE_SF2_IZONE);

	  i2 = zndx - pzndx;	/* # of zones in last instrument */
	  while (i2--)		/* create zones for last instrument */
	    { /* ++ ref new zone and insert it */
	      zone = g_object_new (IPATCH_TYPE_SF2_IZONE, NULL);
	      ipatch_container_insert_iter ((IpatchContainer *)prev, zone,
					    &zone_iter);
	      g_object_unref (zone); /* -- unref new zone */
	    }
	}
      else if (zndx > 0)	/* 1st instrument, warn if ofs > 0 */
	g_warning (_("Discarding %d unreferenced instrument zones"), zndx);

      prev = inst;		/* update previous instrument ptr */
      pzndx = zndx;
    }

  reader->ibag_count = zndx;	/* total number of instrument zones */

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* instrument bag reader */
static gboolean
sfload_ibags (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  guint16 *bag_table;
  guint16 genndx, modndx;
  guint16 pgenndx, pmodndx;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_IBAG, err))
    return (FALSE);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size % IPATCH_SFONT_BAG_SIZE
      || chunk->size / IPATCH_SFONT_BAG_SIZE != reader->ibag_count + 1)
    {
      SET_SIZE_ERROR (riff, -1, err);
      return (FALSE);
    }

  bag_table = reader->ibag_table = g_malloc (chunk->size);
  if (!ipatch_file_read (riff->handle, bag_table, chunk->size, err))
    return (FALSE);	   /* bag_table will be freed by finalize() */

  pgenndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[0]);
  pmodndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[1]);

  for (i=0; i < reader->ibag_count; i++)
    {
      genndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[(i+1)*2]);
      modndx = IPATCH_FILE_SWAP16 (riff->handle->file, &bag_table[(i+1)*2+1]);

      if (genndx < pgenndx)
	{
	  g_set_error (err, IPATCH_RIFF_ERROR,
		       IPATCH_RIFF_ERROR_INVALID_DATA,
		       "Invalid instrument gen index");
	  return (FALSE);
	}
      if (modndx < pmodndx)
	{
	  g_set_error (err, IPATCH_RIFF_ERROR,
		       IPATCH_RIFF_ERROR_INVALID_DATA,
		       "Invalid instrument mod index");
	  return (FALSE);
	}

      bag_table[i*2] = genndx - pgenndx;
      bag_table[i*2+1] = modndx - pmodndx;

      pgenndx = genndx;		/* update previous zone gen index */
      pmodndx = modndx;		/* update previous zone mod index */
    }

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* instrument modulator reader */
static gboolean
sfload_imods (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Zone *zone;
  IpatchSF2Mod mod;
  guint bagmod_index = 1;
  GSList *p, *p2;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_IMOD, err))
    return (FALSE);

  p = reader->sf->insts;
  while (p)			/* traverse through all instruments */
    {
      p2 = IPATCH_SF2_INST (p->data)->zones;
      while (p2)		/* traverse this instrument's zones */
	{
	  zone = IPATCH_SF2_ZONE (p2->data);

	  /* stored modulator count */
	  i = reader->ibag_table[bagmod_index];
	  bagmod_index += 2;

	  while (i-- > 0)	/* load zone's modulators */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_MOD_SIZE,
					 err)) return (FALSE);
	      ipatch_sf2_load_mod (riff->handle, &mod);
	      ipatch_sf2_mod_item_add ((IpatchSF2ModItem *)zone, &mod);
	    }
	  p2 = g_slist_next (p2);
	}
      p = g_slist_next (p);
    }

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* instrument generator reader */
static gboolean
sfload_igens (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchSF2Inst *inst;
  IpatchSF2Zone *zone;
  IpatchSF2GenAmount amount;
  int genid;
  GSList *p, *p2;
  guint ibag_index = 0;
  guint index;
  int level, discarded;
  int i;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_IGEN, err))
    return (FALSE);

  p = reader->sf->insts;
  while (p)			/* traverse through all instruments */
    {
      discarded = FALSE;

      inst = IPATCH_SF2_INST (p->data);
      p2 = inst->zones;
      while (p2)		/* traverse instruments's zones */
	{
	  level = 0;
	  zone = IPATCH_SF2_ZONE (p2->data);

	  /* retrieve our stored gen count (from load_ibag) */
	  i = reader->ibag_table[ibag_index];
	  ibag_index += 2;

	  while (i--)		/* load zone's generators */
	    {
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_GEN_SIZE,
					 err)) return (FALSE);
	      ipatch_sf2_load_gen (riff->handle, &genid, &amount);

	      /* check validity of generator */
	      if ((genid != IPATCH_SF2_GEN_SAMPLE_ID
		   && !ipatch_sf2_gen_is_valid (genid, FALSE))
		  || (genid == IPATCH_SF2_GEN_NOTE_RANGE && level != 0)
		  || (genid == IPATCH_SF2_GEN_VELOCITY_RANGE && level > 1))
		{
		  discarded = TRUE;
		  continue;
		}

	      /* IPATCH_SF2_GEN_NOTE_RANGE first (if any) followed by
		 IPATCH_SF2_GEN_VELOCITY_RANGE (if any),
		 IPATCH_SF2_GEN_SAMPLE_ID is last */
	      if (genid == IPATCH_SF2_GEN_NOTE_RANGE) level = 1;
	      else if (genid == IPATCH_SF2_GEN_VELOCITY_RANGE) level = 2;
	      else if (genid == IPATCH_SF2_GEN_SAMPLE_ID)
		{
		  index = amount.uword;
		  if (index >= reader->sample_count)
		    g_warning (_("Invalid reference in instrument '%s'"),
			       inst->name); /* will get discarded below */
		  else ipatch_sf2_zone_set_link_item (zone, (IpatchItem *)
						      reader->sample_table[index]);
		  level = 3;
		  break;	/* break out of gen loop */
		}
	      else level = 2;

	      /* set the generator */
	      zone->genarray.values[genid] = amount;
	      IPATCH_SF2_GEN_ARRAY_SET_FLAG (&zone->genarray, genid);
	    }			/* generator loop */

	  /* ignore (skip) any generators following a sample ID */
	  while (i-- > 0)
	    {
	      discarded = TRUE;
	      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_GEN_SIZE,
					 err)) return (FALSE);
	    }

	  /* if level !=3 (no sample ID) and not first zone, discard */
	  if (level != 3 && p2 != inst->zones)
	    {		/* discard invalid global zone */
	      IpatchItem *item = IPATCH_ITEM (p2->data);
	      p2 = g_slist_next (p2);
	      ipatch_container_remove (IPATCH_CONTAINER (inst), item);

	      g_warning (_("Instrument \"%s\": Discarding invalid"
			   " global zone"), inst->name);
	      continue;
	    }

	  p2 = g_slist_next (p2); /* next zone */
	}

      /* global zone?  Migrate gens/mods to instrument and remove zone. */
      if (inst->zones && !((IpatchSF2Zone *)(inst->zones->data))->item)
	{
	  IpatchSF2Zone *zone = (IpatchSF2Zone *)(inst->zones->data);

	  inst->genarray = zone->genarray;

	  inst->mods = zone->mods;	/* snatch modulator list */
	  zone->mods = NULL;

	  ipatch_container_remove (IPATCH_CONTAINER (inst), (IpatchItem *)zone);
	}

      if (discarded)
	g_warning (_("Instrument \"%s\": Some invalid generators"
		     " were discarded"), inst->name);

      p = g_slist_next (p);	/* next instrument */
    }

  /* free some no longer needed tables */
  g_free (reader->ibag_table);
  reader->ibag_table = NULL;
  g_free (reader->sample_table);
  reader->sample_table = NULL;

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}

/* sample header reader */
static gboolean
sfload_shdrs (IpatchSF2Reader *reader, GError **err)
{
  IpatchRiff *riff = IPATCH_RIFF (reader);
  IpatchRiffChunk *chunk;
  IpatchSF2Sample *sample, *link_sample, **sptr;
  IpatchSampleData *sampledata;
  IpatchSample *store;
  IpatchIter iter;
  IpatchSF2Shdr shdr;
  guint samchunk_pos, samchunk_size, sam24chunk_pos;
  guint openlink_count = 0;
  char *filename;
  int i, count;

  if (!ipatch_riff_read_chunk_verify (riff, IPATCH_RIFF_CHUNK_SUB,
				      IPATCH_SFONT_FOURCC_SHDR, err))
    return (FALSE);

  g_object_get (IPATCH_BASE (reader->sf)->file,
		"file-name", &filename,         /* ++ alloc filename */
		"sample-pos", &samchunk_pos,
		"sample-size", &samchunk_size,
		"sample24-pos", &sam24chunk_pos,
		NULL);

  chunk = ipatch_riff_get_chunk (riff, -1);
  if (chunk->size <= IPATCH_SFONT_SHDR_SIZE) return (TRUE); /* no samples? */
  if (chunk->size % IPATCH_SFONT_SHDR_SIZE) /* verify chunk size */
    {
      SET_SIZE_ERROR (riff, -1, err);
      g_free (filename);        /* -- free filename */
      return (FALSE);
    }

  /* initialize iterator to sample list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->sf), &iter,
			      IPATCH_TYPE_SF2_SAMPLE);

  /* get number of sample headers */
  count = chunk->size / IPATCH_SFONT_SHDR_SIZE - 1;
  reader->sample_count = count;

  /* allocate sample fixup table (to fixup instrument zone references) */
  sptr = reader->sample_table = g_malloc (count * sizeof (IpatchSF2Sample *));

  /* load all sample headers (not including terminal record) */
  for (i = 0; i < count; i++)
    {
      if (!ipatch_file_buf_load (riff->handle, IPATCH_SFONT_SHDR_SIZE, err))
      {
        g_free (filename);        /* -- free filename */
	return (FALSE);
      }

      ipatch_sf2_load_shdr (riff->handle, &shdr);

      /* ++ ref new sample and add to sample fixup table */
      *(sptr++) = sample = g_object_new (IPATCH_TYPE_SF2_SAMPLE, NULL);
      sample->name = g_strndup (shdr.name, 20);

      /* sample NOT totally screwed? (ROM or within sample chunk, ROM or sample
	 chunk actually exists, at least 4 samples in length) */
      if (((shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_ROM) || shdr.end <= samchunk_size)
	  && ((shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_ROM) || samchunk_pos > 0)
	  && (shdr.start < shdr.end && shdr.end - shdr.start > 4))
	{
          if (shdr.loop_start < shdr.start)
          {
            g_warning (_("Sample '%s' loop start begins before sample data, setting to offset 0"),
                       sample->name);
            sample->loop_start = 0;
          }
          else sample->loop_start = shdr.loop_start - shdr.start;

          if (shdr.loop_end < shdr.start)
          {
            g_warning (_("Sample '%s' loop end begins before sample data, setting to offset 0"),
                       sample->name);
            sample->loop_end = 0;
          }
          else sample->loop_end = shdr.loop_end - shdr.start;

          /* Keep invalid loop indexes since instrument zones offsets may correct them.
           * In particular samples have been seen with end loop points 1 sample off the
           * end of the sample.  Output warning though. */
          if (shdr.loop_end > shdr.end || shdr.loop_start >= shdr.loop_end)
            g_warning (_("Sample '%s' has invalid loop, keeping it (start:%u end:%u loop_start:%u loop_end:%u)"),
                       sample->name, shdr.start, shdr.end, shdr.loop_start, shdr.loop_end);

	  sample->rate = shdr.rate;
	  sample->root_note = shdr.root_note;
	  sample->fine_tune = shdr.fine_tune;

          if (shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_RIGHT)
            sample->channel = IPATCH_SF2_SAMPLE_CHANNEL_RIGHT;
          else if (shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_LEFT)
            sample->channel = IPATCH_SF2_SAMPLE_CHANNEL_LEFT;
          else sample->channel = IPATCH_SF2_SAMPLE_CHANNEL_MONO;

	  /* check for stereo linked sample */
	  if ((shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_RIGHT
	       || shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_LEFT))
	    {
	      if (shdr.link_index < i) /* can we fixup link? */
		{
		  link_sample = reader->sample_table[shdr.link_index];
		  if (!ipatch_sf2_sample_peek_linked (link_sample))     /* sample not already linked? */
		    {
		      /* FIXME - Ensure same size samples */

		      ipatch_sf2_sample_set_linked (sample, link_sample);
		      ipatch_sf2_sample_set_linked (link_sample, sample);

		      if (ipatch_item_get_flags (link_sample) & (1 << 31))
			{
			  openlink_count--;
			  ipatch_item_clear_flags (link_sample, 1 << 31);
			}
		    }
		  else	/* sample is already linked */
		    g_warning (_("Duplicate stereo link to sample '%s'"
				 " from '%s'"),
			       link_sample->name, sample->name);
		}
	      else /* could not fixup stereo link, do later (below) */
		{
		  openlink_count++;
		  ipatch_item_set_flags (sample, 1 << 31);
		}
	    }

	  /* sample is not a ROM sample? */
	  if (!(shdr.type & IPATCH_SF2_FILE_SAMPLE_TYPE_ROM))
	    { /* SoundFont contains 24 bit audio? */
	      if (sam24chunk_pos > 0)
		{ /* ++ ref new split 24 bit sample store */
		  store = ipatch_sample_store_split24_new (riff->handle->file,
                                                           samchunk_pos + shdr.start * 2,
                                                           sam24chunk_pos + shdr.start);

                  /* use host endian, Split24 stores will transform as necessary */
		  ipatch_sample_set_format (store, IPATCH_SAMPLE_24BIT
					    | IPATCH_SAMPLE_MONO
					    | IPATCH_SAMPLE_SIGNED
					    | IPATCH_SAMPLE_ENDIAN_HOST);
		}
	      else	/* 16 bit samples */
		{ /* ++ ref new file sample store */
		  store = ipatch_sample_store_file_new (riff->handle->file,
                                                        samchunk_pos + shdr.start * 2);
		  ipatch_sample_set_format (store, IPATCH_SAMPLE_16BIT
				            | IPATCH_SAMPLE_MONO
				            | IPATCH_SAMPLE_SIGNED
				            | IPATCH_SAMPLE_LENDIAN);
		}
	    }
	  else			/* ROM sample, create ROM sample store */
	    { /* Set ROM flag */
	      ipatch_item_set_flags (sample, IPATCH_SF2_SAMPLE_FLAG_ROM);

              /* ++ ref new ROM sample store */
	      store = ipatch_sample_store_rom_new (shdr.start * 2);
	      ipatch_sample_set_format (store, IPATCH_SAMPLE_16BIT
				        | IPATCH_SAMPLE_MONO
				        | IPATCH_SAMPLE_SIGNED
				        | IPATCH_SAMPLE_LENDIAN);
	    }

          g_object_set (store,
                        "sample-size", shdr.end - shdr.start,
                        "sample-rate", shdr.rate,
                        NULL);

          sampledata = ipatch_sample_data_new ();	/* ++ ref */
          ipatch_sample_data_add (sampledata, (IpatchSampleStore *)store);
          ipatch_sf2_sample_set_data (sample, sampledata);

	  g_object_unref (store); /* -- unref sample store */
          g_object_unref (sampledata);	/* -- unref sampledata */
	}
      else			/* sample is messed, set to blank data */
	{
	  g_warning (_("Invalid sample '%s'"), sample->name);
	  ipatch_sf2_sample_set_blank (sample);
	}

      /* insert sample into SoundFont */
      ipatch_container_insert_iter ((IpatchContainer *)(reader->sf),
				    (IpatchItem *)sample, &iter);
      g_object_unref (sample);	/* -- unref new sample */
    }

  if (openlink_count > 0)	/* any unresolved linked stereo samples? */
    {
      count = reader->sample_count;
      for (i = 0; i < reader->sample_count; i++)
	{
	  sample = reader->sample_table[i];
	  if ((ipatch_item_get_flags (sample) & (1 << 31)) && !ipatch_sf2_sample_peek_linked (sample))
	    {
	      g_warning (_("Invalid stereo link for sample '%s'"), sample->name);
	      sample->channel = IPATCH_SF2_SAMPLE_CHANNEL_MONO;
	    }
	}
    }

  g_free (filename);        /* -- free filename */

  if (!ipatch_riff_end_chunk (riff, err)) return (FALSE);

  return (TRUE);
}
