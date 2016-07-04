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
#ifndef __IPATCH_SLI_FILE_PRIV_H__
#define __IPATCH_SLI_FILE_PRIV_H__

#include <glib.h>

/* forward type declarations */

typedef struct _IpatchSLISiFi IpatchSLISiFi;
typedef struct _IpatchSLISiIg IpatchSLISiIg;
typedef struct _IpatchSLIInstHeader IpatchSLIInstHeader;
typedef struct _IpatchSLIZoneParams IpatchSLIZoneParams;
typedef struct _IpatchSLIModParams IpatchSLIModParams;
typedef struct _IpatchSLIZoneHeader IpatchSLIZoneHeader;
typedef struct _IpatchSLISampleHeader IpatchSLISampleHeader;
typedef struct _IpatchSLISiDp IpatchSLISiDp;

/* Spectralis file header */
struct _IpatchSLISiFi
{
  guint32 ckid;			/* chunk id 'SiFi' */
  guint32 cklen;		/* chunk (whole file - 8) length */
  guint16 spechdr;		/* version? always 0x100 */
  guint16 unused;		/* 0 */
  guint16 ignum;		/* number of instrument groups */
  guint16 igstart_offs;		/* offset in file for start of inst. groups */
};

/* Spectralis instrument group header */
struct _IpatchSLISiIg
{
  guint32 ckid;			/* chunk id 'SiIg' */
  guint32 cklen;		/* number of bytes in chunk */
  guint16 spechdr;		/* version? always 0x100 */
  guint16 unused1;		/* 0 */
  guint16 inst_offs;		/* offset in chunk for start of inst. headers */
  guint16 instnum;		/* number of instruments in group */
  guint16 zones_offs;		/* offset to zone headers */
  guint16 allzones_num;		/* total number of zones in group */
  guint16 smphdr_offs;		/* offset to sample headers */
  guint16 maxzones_num;		/* largest number of zones in one inst */
  guint16 smpdata_offs;		/* offset to start of sample data */
  guint16 unused2;		/* 0 */
};

/* Spectralis instrument header */
struct _IpatchSLIInstHeader
{
  char name[24];		/* name of instrument */
  guint32 sound_id;		/* unique? id of the instrument */
  guint32 unused1;		/* 0 */
  guint16 category;		/* category code for sub and main category */
  guint16 unused2;		/* 0 */
  guint16 zone_idx;		/* index of first zone header for this inst. */
  guint16 zones_num;		/* number of zones for this inst. */
};

/* Spectralis zone params */
struct _IpatchSLIZoneParams
{
  guint8  keyrange_low;
  guint8  keyrange_high;
  guint8  velrange_low;
  guint8  velrange_high;
  guint32 start_offs1;
  guint32 start_offs2;
  guint32 unknown1;
  guint32 unknown2;
  gint8   coarse_tune1;
  gint8   fine_tune1;
  guint8  sample_modes;
  gint8   root_note;
  guint16 scale_tuning;
  gint8   coarse_tune2;
  gint8   fine_tune2;
};

/* Spectralis mod params */
struct _IpatchSLIModParams
{
  gint16 modLfoToPitch;
  gint16 vibLfoToPitch;
  gint16 modEnvToPitch;
  guint16 initialFilterFc;
  guint16 initialFilterQ;
  gint16 modLfoToFilterFc;
  gint16 modEnvToFilterFc;
  gint16 modLfoToVolume;
  gint16 freqModLfo;
  gint16 freqVibLfo;
  guint16 sustainModEnv;
  gint16 keynumToModEnvHold;
  gint16 keynumToModEnvDecay;
  guint16 sustainVolEnv;
  gint16 keynumToVolEnvHold;
  gint16 keynumToVolEnvDecay;
  gint8   pan;
  gint8  delayModLfo;
  gint8  delayVibLfo;
  gint8  attackModEnv;
  gint8  holdModEnv;
  gint8  decayModEnv;
  gint8  releaseModEnv;
  gint8  attackVolEnv;
  gint8  holdVolEnv;
  gint8  decayVolEnv;
  gint8  releaseVolEnv;
  guint8  initialAttenuation;
};

/* Spectralis Zone header */
struct _IpatchSLIZoneHeader
{
  IpatchSLIZoneParams zone_params;
  IpatchSLIModParams  mod_params;
  guint16	      sample_idx;
  guint16	      unused;
};

/* Spectralis file sample header */
struct _IpatchSLISampleHeader
{
  char name[24];		/* sample name */
  guint32 start;		/* offset to start of sample */
  guint32 end;			/* offset to end of sample */
  guint32 loop_start;		/* offset to start of loop */
  guint32 loop_end;		/* offset to end of loop */
  gint8   fine_tune;		/* pitch correction in cents */
  guint8  root_note;		/* root midi note number */
  guint8  channels;		/* number of channels */
  guint8  bits_per_sample;	/* number of bits per sample */
  guint32 sample_rate;		/* sample rate recorded at */
};

/* Spectralis Instrument End header */
struct _IpatchSLISiDp
{
  guint32 ckid;			/* chunk id 'SiDp' */
  guint32 cklen;		/* number of bytes in chunk */
  guint16 spechdr;		/* version? always 0x100 */
  guint16 unused;		/* 0 */
};


/* Spectralis chunk FOURCC guint32 integers */
#define IPATCH_SLI_FOURCC_SIFI  IPATCH_FOURCC ('S','i','F','i')
#define IPATCH_SLI_FOURCC_SIIG  IPATCH_FOURCC ('S','i','I','g')
#define IPATCH_SLI_FOURCC_SIDP  IPATCH_FOURCC ('S','i','D','p')

#define IPATCH_SLI_SPECHDR_VAL  0x100

/* Spectralis file chunk sizes */
#define IPATCH_SLI_SIFI_SIZE   8  /* file info header size (w/o RIFF header) */
#define IPATCH_SLI_SIIG_SIZE  28  /* inst. group header size */
#define IPATCH_SLI_INST_SIZE  40  /* instrument header size */
#define IPATCH_SLI_ZONE_SIZE  76  /* zone params header size */
#define IPATCH_SLI_SMPL_SIZE  48  /* sample data header size */
#define IPATCH_SLI_SIDP_SIZE  12  /* instrument terminator size */
#define IPATCH_SLI_HEAD_SIZE  64*1024  /* maximal size of headers */

#endif
