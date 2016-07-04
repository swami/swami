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
 * SECTION: IpatchSLIReader
 * @short_description: Spectralis file reader
 * @see_also: 
 *
 * Reads a Spectralis SLI or SLC file and loads it into a object tree
 * (#IpatchSLI).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "IpatchRiff.h"
#include "IpatchSLIReader.h"
#include "IpatchSampleStore.h"
#include "IpatchSampleStoreRam.h"
#include "IpatchSampleStoreFile.h"
#include "IpatchSLIFile_priv.h"
#include "IpatchUnit.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* ----- ** WARNING ** -----
   We don't care about locking in here, because we are the exclusive
   owner of the loading SoundFont. Many fields are accessed directly,
   etc. This is not an example of proper use of libInstPatch..  Don't
   try this at home.. Blah blah. */

static void ipatch_sli_reader_class_init (IpatchSLIReaderClass *klass);
static void ipatch_sli_reader_init (IpatchSLIReader *reader);
static void ipatch_sli_reader_finalize (GObject *object);
static GQuark ipatch_sli_reader_error_quark (void);
static gboolean ipatch_sli_load_level_0 (IpatchSLIReader *reader,
					 GError **err);

static void ipatch_sli_load_siig (IpatchFileHandle *handle,
                                  IpatchSLISiIg *siig);
static void ipatch_sli_load_ihdr (IpatchFileHandle *handle,
                                  IpatchSLIInstHeader *ihdr);
static void ipatch_sli_load_shdr (IpatchFileHandle *handle,
                                  IpatchSLISampleHeader *shdr);
static void ipatch_sli_set_gen (IpatchSLIZone *zone,
                                const int genid,
                                IpatchSF2GenAmount *amount);
static guint32 ipatch_sli_load_zone (IpatchFileHandle *handle,
                                     IpatchSLIZone *zone);
static IpatchSLISample *ipatch_sli_load_sample (IpatchFileHandle *handle,
                                                guint32 smpdata_offs);

#define SLI_ERROR_MSG "Spectralis reader error: %s"

G_DEFINE_TYPE (IpatchSLIReader, ipatch_sli_reader, G_TYPE_OBJECT)

static void
ipatch_sli_reader_class_init (IpatchSLIReaderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->finalize = ipatch_sli_reader_finalize;
}

static void
ipatch_sli_reader_init (IpatchSLIReader *reader)
{
  reader->handle = NULL;
  reader->sli = NULL;
}

static void
ipatch_sli_reader_finalize (GObject *object)
{
  IpatchSLIReader *reader = IPATCH_SLI_READER (object);

  if (reader->handle)
    ipatch_file_close (reader->handle); /* -- unref file object */

  if (reader->sli)
  {
    g_object_unref (reader->sli); /* -- unref SLI */
    reader->sli = NULL;
  }

  if (G_OBJECT_CLASS (ipatch_sli_reader_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sli_reader_parent_class)->finalize (object);
}

static GQuark
ipatch_sli_reader_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("sli-reader-error-quark");

  return (q);
}

/**
 * ipatch_sli_reader_new:
 * @handle: Spectralis file handle to parse or %NULL to set later
 *
 * Create a new Spectralis file reader
 *
 * Returns: The new Spectralis file reader
 */
IpatchSLIReader *
ipatch_sli_reader_new (IpatchFileHandle *handle)
{
  IpatchSLIReader *reader;

  g_return_val_if_fail (!handle || IPATCH_IS_SLI_FILE (handle->file), NULL);

  reader = g_object_new (IPATCH_TYPE_SLI_READER, NULL);
  ipatch_sli_reader_set_file_handle (reader, handle);

  return (reader);
}

/**
 * ipatch_sli_reader_set_file_handle:
 * @reader: Spectralis reader object
 * @handle: Spectralis file handle
 *
 * Set the Spectralis file handle of a Spectralis reader.
 */
void
ipatch_sli_reader_set_file_handle (IpatchSLIReader *reader, IpatchFileHandle *handle)
{
  g_return_if_fail (IPATCH_IS_SLI_READER (reader));
  g_return_if_fail (handle && IPATCH_IS_SLI_FILE (handle->file));

  /* Close old handle, if any */
  if (reader->handle) ipatch_file_close (reader->handle);

  reader->handle = handle;
}

/**
 * ipatch_sli_reader_load:
 * @reader: Spectralis reader object
 * @err: Location to store error info or %NULL
 *
 * Load a Spectralis file.
 *
 * Returns: (transfer full): New #IpatchSLI object with refcount of 1.
 */
IpatchSLI *
ipatch_sli_reader_load (IpatchSLIReader *reader, GError **err)
{
  GError *local_err = NULL;
  guint32 buf[IPATCH_RIFF_HEADER_SIZE / sizeof(guint32)];
  int size;
 
  g_return_val_if_fail (IPATCH_IS_SLI_READER (reader), NULL);
  g_return_val_if_fail (reader->handle &&
                        IPATCH_IS_SLI_FILE (reader->handle->file), NULL);
  g_return_val_if_fail (!err || !*err, NULL);

  /* read ID and chunk size */
  if (!ipatch_file_read (reader->handle, buf, IPATCH_RIFF_HEADER_SIZE, err))
    return (NULL);

  if (buf[0] != IPATCH_SLI_FOURCC_SIFI)
  {
    char idstr[4];

    memcpy (idstr, buf, 4);
    g_set_error (err, ipatch_sli_reader_error_quark(),
                 IPATCH_RIFF_ERROR_UNEXPECTED_ID,
                 _("Not a Spectralis file (RIFF id = '%4s')"), idstr);
    return (NULL);
  }
 
  /* verify total size of file with RIFF chunk size */
  size = ipatch_file_get_size (reader->handle->file, &local_err);

  if (size == -1)
  {
    g_warning ("Spectralis file size check failed: %s",
               ipatch_gerror_message (local_err));
    g_clear_error (&local_err);
  }
  else if (size != buf[1])
  {
    g_set_error (err, ipatch_sli_reader_error_quark(),
                 IPATCH_RIFF_ERROR_SIZE_MISMATCH,
                 _("File size mismatch (chunk size = %d, actual = %d)"),
                 buf[1], size);
    return (NULL);
  }

  reader->sli = ipatch_sli_new (); /* ++ ref new object */
  ipatch_sli_set_file (reader->sli, IPATCH_SLI_FILE (reader->handle->file));

  if (!ipatch_file_skip (reader->handle, IPATCH_SLI_SIFI_SIZE, err)
      || !ipatch_sli_load_level_0 (reader, err))
  {
    g_object_unref (reader->sli);
    reader->sli = NULL;
    return (NULL);
  }

  ipatch_item_clear_flags (IPATCH_ITEM (reader->sli),
			   IPATCH_BASE_SAVED | IPATCH_BASE_CHANGED);

  /* ++ ref for caller, finalize() will remove reader's reference */
  return (g_object_ref (reader->sli));
}

static gboolean
ipatch_sli_load_level_0 (IpatchSLIReader *reader, GError **err)
{
  GError *local_err = NULL;
  guint32 pos, size;
  IpatchSLISiIg siig;
  IpatchSLIInstHeader ihdr;
  IpatchSLIInst *inst;
  IpatchSLIZone *zone;
  IpatchIter inst_iter, zone_iter, smpl_iter;
  unsigned int i, z, sample_idx;
  IpatchSLISample **sample_map;

  /* initialize iterator to instrument list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->sli), &inst_iter,
                              IPATCH_TYPE_SLI_INST);

  /* initialize iterator to sample list */
  ipatch_container_init_iter (IPATCH_CONTAINER (reader->sli), &smpl_iter,
			      IPATCH_TYPE_SLI_SAMPLE);

  /* clear siig just to silence the compiler */
  memset (&siig, 0, sizeof (siig));

  size = ipatch_file_get_size (reader->handle->file, &local_err);
  pos = ipatch_file_get_position (reader->handle);
  while (size > pos)
  {
    /* get headers */
    if (!ipatch_file_buf_load (reader->handle, IPATCH_SLI_HEAD_SIZE, err))
      return (FALSE);
    /* load next SiIg header */
    ipatch_sli_load_siig (reader->handle, &siig);
    if (siig.ckid != IPATCH_SLI_FOURCC_SIIG)
    {
      char idstr[4];

      memcpy (idstr, &siig.ckid, 4);
      g_set_error (err, ipatch_sli_reader_error_quark(),
                   IPATCH_RIFF_ERROR_UNEXPECTED_ID,
                   _("Not an instument group header (RIFF id = '%4s', position = %d)"),
                   idstr, pos);
      return (FALSE);
    }
    if (siig.cklen > size - pos) /* verify chunk size */
    {
      g_set_error (err, ipatch_sli_reader_error_quark(),
                   IPATCH_RIFF_ERROR_SIZE_MISMATCH,
                   _(SLI_ERROR_MSG), "Unexpected chunk size in instument group header");
      return (FALSE);
    }
    if (!siig.instnum) /* empty inst group? */
      goto skip;

    sample_map = g_malloc0 (siig.maxzones_num * sizeof (gpointer));

    /* loop over all instrument headers */
    for (i = 0; i < siig.instnum; i++)
    {
      ipatch_file_buf_seek (reader->handle, siig.inst_offs +
                            i * IPATCH_SLI_INST_SIZE, G_SEEK_SET);
      ipatch_sli_load_ihdr (reader->handle, &ihdr);

      inst = ipatch_sli_inst_new (); /* ++ ref new instrument */
      inst->name = g_strndup (ihdr.name, IPATCH_SLI_NAME_SIZE);
      inst->sound_id = ihdr.sound_id;
      inst->category = ihdr.category;

      /* insert the instrument (default is to keep appending) */
      ipatch_container_insert_iter ((IpatchContainer *)(reader->sli),
                                    (IpatchItem *)inst, &inst_iter);
      g_object_unref (inst); /* -- unref new instrument */

      /* initialize iterator to instrument zone list */
      ipatch_container_init_iter ((IpatchContainer *)inst, &zone_iter,
                                  IPATCH_TYPE_SLI_ZONE);

      for (z = 0; z < ihdr.zones_num; z++)
      {
        /* read zone headers */
        ipatch_file_buf_seek (reader->handle, siig.zones_offs +
                              (ihdr.zone_idx + z) * IPATCH_SLI_ZONE_SIZE,
                              G_SEEK_SET);
        zone = g_object_new (IPATCH_TYPE_SLI_ZONE, NULL); /* ++ ref new zone */
        sample_idx = ipatch_sli_load_zone(reader->handle, zone);
        ipatch_container_insert_iter ((IpatchContainer *)inst,
                                      (IpatchItem *)zone, &zone_iter);

        if (sample_idx >= siig.maxzones_num)
        {
          g_set_error (err, ipatch_sli_reader_error_quark(),
                       IPATCH_RIFF_ERROR_INVALID_DATA,
                       "Sample index is too large in zone %d of inst '%s'",
                       z, inst->name);
          return (FALSE);
        }
        /* use existing or create new sample from corresponding sample header */
        if (!sample_map[sample_idx])
        {
          ipatch_file_buf_seek (reader->handle, siig.smphdr_offs +
                                sample_idx * IPATCH_SLI_SMPL_SIZE, G_SEEK_SET);
          /* ++ ref new sample */
          sample_map[sample_idx] = ipatch_sli_load_sample (reader->handle, pos +
                                                           siig.smpdata_offs);
          /* insert sample */
          ipatch_container_insert_iter ((IpatchContainer *)(reader->sli),
                                        (IpatchItem *)sample_map[sample_idx],
                                        &smpl_iter);
        }
        ipatch_sli_zone_set_sample (zone, sample_map[sample_idx]);
        g_object_unref (zone); /* -- unref new zone */
      } /* for (zone) */
    } /* for (inst) */
    for (sample_idx = 0; sample_idx < siig.maxzones_num; sample_idx++)
      if (!sample_map[sample_idx])
        g_object_unref (sample_map[sample_idx]); /* -- unref new sample */
    g_free (sample_map);
skip:
    /* seek to the end of the current chunk */
    if (!ipatch_file_seek (reader->handle, pos + siig.cklen, G_SEEK_SET, err))
      return (FALSE);

    /* skip SiDp chunks (one for each instrument) */
    if (!ipatch_file_skip (reader->handle, 
                           siig.instnum * IPATCH_SLI_SIDP_SIZE, err))
      return (FALSE);

    pos = ipatch_file_get_position (reader->handle);
  } /* while (chunks) */

  return (TRUE);
}

static void
ipatch_sli_load_siig (IpatchFileHandle *handle, IpatchSLISiIg *siig)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (siig != NULL);

  siig->ckid = ipatch_file_buf_read_u32 (handle);
  siig->cklen = ipatch_file_buf_read_u32 (handle);
  siig->spechdr = ipatch_file_buf_read_u16 (handle);
  siig->unused1 = ipatch_file_buf_read_u16 (handle);
  siig->inst_offs = ipatch_file_buf_read_u16 (handle);
  siig->instnum = ipatch_file_buf_read_u16 (handle);
  siig->zones_offs = ipatch_file_buf_read_u16 (handle);
  siig->allzones_num = ipatch_file_buf_read_u16 (handle);
  siig->smphdr_offs = ipatch_file_buf_read_u16 (handle);
  siig->maxzones_num = ipatch_file_buf_read_u16 (handle);
  siig->smpdata_offs = ipatch_file_buf_read_u16 (handle);
  siig->unused2 = ipatch_file_buf_read_u16 (handle);
}

static void
ipatch_sli_load_ihdr (IpatchFileHandle *handle, IpatchSLIInstHeader *ihdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (ihdr != NULL);

  ipatch_file_buf_read (handle, &ihdr->name, IPATCH_SLI_NAME_SIZE);
  ihdr->sound_id = ipatch_file_buf_read_u32 (handle);
  ihdr->unused1 = ipatch_file_buf_read_u32 (handle);
  ihdr->category = ipatch_file_buf_read_u16 (handle);
  ihdr->unused2 = ipatch_file_buf_read_u16 (handle);
  ihdr->zone_idx = ipatch_file_buf_read_u16 (handle);
  ihdr->zones_num = ipatch_file_buf_read_u16 (handle);
}

static void
ipatch_sli_load_shdr (IpatchFileHandle *handle, IpatchSLISampleHeader *shdr)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (shdr != NULL);

  ipatch_file_buf_read (handle, &shdr->name, IPATCH_SLI_NAME_SIZE);
  shdr->start = ipatch_file_buf_read_u32 (handle);
  shdr->end = ipatch_file_buf_read_u32 (handle);
  shdr->loop_start = ipatch_file_buf_read_u32 (handle);
  shdr->loop_end = ipatch_file_buf_read_u32 (handle);
  shdr->fine_tune = ipatch_file_buf_read_s8 (handle);
  shdr->root_note = ipatch_file_buf_read_u8 (handle);
  shdr->channels = ipatch_file_buf_read_u8 (handle);
  shdr->bits_per_sample = ipatch_file_buf_read_u8 (handle);
  shdr->sample_rate = ipatch_file_buf_read_u32 (handle);
}

static void
ipatch_sli_set_gen (IpatchSLIZone *zone, const int genid,
                    IpatchSF2GenAmount *amount)
{
  IpatchSF2GenAmount def_amount;

  g_return_if_fail (zone != NULL);
  g_return_if_fail (genid >= 0 && genid < IPATCH_SF2_GEN_COUNT);
  g_return_if_fail (amount != NULL);

  /* check if amount is different from default... */
  ipatch_sf2_gen_default_value(genid, FALSE, &def_amount);
  if (amount->sword != def_amount.sword)
  { /* ...and set it on zone if so */
    zone->genarray.values[genid] = *amount;
    IPATCH_SF2_GEN_ARRAY_SET_FLAG (&zone->genarray, genid);
  }
}

static guint32
ipatch_sli_load_zone (IpatchFileHandle *handle, IpatchSLIZone *zone)
{
  IpatchSF2GenAmount amount;
  guint32 offs;

  amount.range.low = ipatch_file_buf_read_u8 (handle); /* keyrange_low */
  amount.range.high = ipatch_file_buf_read_u8 (handle); /* keyrange_high */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_NOTE_RANGE, &amount);

  amount.range.low = ipatch_file_buf_read_u8 (handle); /* velrange_low */
  amount.range.high = ipatch_file_buf_read_u8 (handle); /* velrange_high */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VELOCITY_RANGE, &amount);

  offs = ipatch_file_buf_read_u32 (handle); /* start_offs1 */
  if (offs != ipatch_file_buf_read_u32 (handle)) /* start_offs2 */
    g_warning (_("Ignoring different 2nd start offset for zone"));
  amount.uword = offs >> 16;
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_SAMPLE_COARSE_START, &amount);
  amount.uword = (guint16)offs / 2;
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_SAMPLE_START, &amount);

  offs = ipatch_file_buf_read_u32 (handle); /* unknown1 */
  if (offs)
    g_warning (_("Ignoring 1st unknown value for zone"));
  offs = ipatch_file_buf_read_u32 (handle); /* unknown2 */
  if (offs)
    g_warning (_("Ignoring 2nd unknown value for zone"));

  amount.sword = ipatch_file_buf_read_s8 (handle); /* coarse_tune1 */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_COARSE_TUNE, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle); /* fine_tune1 */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amount);

  zone->flags = ipatch_file_buf_read_u8 (handle); /* sample_modes */
  if (zone->flags & IPATCH_SF2_GEN_SAMPLE_MODE_LOOP)
  {
    amount.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_SAMPLE_MODES, &amount);
  }

  amount.sword = ipatch_file_buf_read_s8 (handle); /* root_note */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amount);

  amount.uword = ipatch_file_buf_read_u16 (handle); /* scale_tuning */
  if (amount.uword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_SCALE_TUNE, &amount);

  amount = IPATCH_SLI_ZONE_GEN_AMT(zone, IPATCH_SF2_GEN_COARSE_TUNE);
  if (amount.sword != ipatch_file_buf_read_s8 (handle)) /* coarse_tune2 */
    g_warning (_("Ignoring different 2nd coarse tune value for zone"));

  amount = IPATCH_SLI_ZONE_GEN_AMT(zone, IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE);
  if (amount.sword != ipatch_file_buf_read_s8 (handle)) /* fine_tune2 */
    g_warning (_("Ignoring different 2nd fine tune value for zone"));

  amount.sword = ipatch_file_buf_read_s16 (handle); /* modLfoToPitch */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_LFO_TO_PITCH, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* vibLfoToPitch */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VIB_LFO_TO_PITCH, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* modEnvToPitch */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_TO_PITCH, &amount);

  amount.uword = ipatch_file_buf_read_u16 (handle); /* initialFilterFc */
  if (amount.uword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_FILTER_CUTOFF, &amount);

  amount.uword = ipatch_file_buf_read_u16 (handle); /* initialFilterQ */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_FILTER_Q, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* modLfoToFilterFc */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_CUTOFF, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* modEnvToFilterFc */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_CUTOFF, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* modLfoToVolume */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_LFO_TO_VOLUME, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* freqModLfo */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_LFO_FREQ, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* freqVibLfo */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VIB_LFO_FREQ, &amount);

  amount.uword = ipatch_file_buf_read_u16 (handle); /* sustainModEnv */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_SUSTAIN, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* keynumToModEnvHold */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* keynumToModEnvDecay */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY, &amount);

  amount.uword = ipatch_file_buf_read_u16 (handle); /* sustainVolEnv */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VOL_ENV_SUSTAIN, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* keynumToVolEnvHold */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD, &amount);

  amount.sword = ipatch_file_buf_read_s16 (handle); /* keynumToVolEnvDecay */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 5; /* pan */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_PAN, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* delayModLfo */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_LFO_DELAY, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* delayVibLfo */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VIB_LFO_DELAY, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* attackModEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_ATTACK, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* holdModEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_HOLD, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* decayModEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_DECAY, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* releaseModEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_MOD_ENV_RELEASE, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* attackVolEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VOL_ENV_ATTACK, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* holdVolEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VOL_ENV_HOLD, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* decayVolEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VOL_ENV_DECAY, &amount);

  amount.sword = ipatch_file_buf_read_s8 (handle) * 100; /* releaseVolEnv */
  if (amount.sword)
    ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_VOL_ENV_RELEASE, &amount);

  amount.uword = ipatch_file_buf_read_u8 (handle) * 10; /* initialAttenuation */
  ipatch_sli_set_gen(zone, IPATCH_SF2_GEN_ATTENUATION, &amount);

  return ipatch_file_buf_read_u16 (handle);
}

static IpatchSLISample *
ipatch_sli_load_sample (IpatchFileHandle *handle, guint32 smpdata_offs)
{
  IpatchSLISampleHeader shdr;
  IpatchSLISample *sample;
  IpatchSample *store;
  IpatchSampleData *sampledata;
  guint bytes_per_sample, length;
  int format;

  ipatch_sli_load_shdr (handle, &shdr);
  sample = g_object_new (IPATCH_TYPE_SLI_SAMPLE, NULL); /* ++ ref new sample */
  sample->name = g_strndup (shdr.name, IPATCH_SLI_NAME_SIZE);

  /* some basic sanity check of sample header data */
  if (shdr.start > shdr.end || shdr.end - shdr.start < 48)
  {
    g_warning (_("Invalid sample '%s'"), sample->name);
    goto bad;
  }

  bytes_per_sample = shdr.bits_per_sample / 8;
  length = shdr.end - shdr.start;

  if (shdr.loop_start <= shdr.loop_end &&
      shdr.loop_start <= length && shdr.loop_end <= length)
  {    /* loop is okay? */
    sample->loop_start = shdr.loop_start / bytes_per_sample;
    sample->loop_end = shdr.loop_end / bytes_per_sample;
  }
  else /* loop is bad */
    g_warning (_("Invalid loop for sample '%s'"), sample->name);

  sample->rate = shdr.sample_rate;
  sample->root_note = shdr.root_note;
  sample->fine_tune = shdr.fine_tune;

  format = (shdr.bits_per_sample == 8 ? IPATCH_SAMPLE_8BIT : IPATCH_SAMPLE_16BIT)
           | (shdr.channels == 2 ? IPATCH_SAMPLE_STEREO : IPATCH_SAMPLE_MONO)
           | IPATCH_SAMPLE_SIGNED | IPATCH_SAMPLE_LENDIAN;

  store = ipatch_sample_store_file_new (handle->file,  /* ++ ref sample store */
                                        smpdata_offs + shdr.start);
  g_object_set (store,
                "sample-size", length / bytes_per_sample / shdr.channels,
                "sample-format", format,
                "sample-rate", sample->rate,
                NULL);
  sampledata = ipatch_sample_data_new ();  /* ++ ref sampledata */
  ipatch_sample_data_add (sampledata, IPATCH_SAMPLE_STORE (store));
  ipatch_sli_sample_set_data (sample, sampledata);
  g_object_unref (store);  /* -- unref sample store */
  g_object_unref (sampledata);  /* -- unref sampledata */

  return sample; /* !! pass on ref to caller */

bad:
  ipatch_sli_sample_set_blank (sample);
  return sample; /* !! pass on ref to caller */
}
