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
#ifndef __IPATCH_SF2_FILE_H__
#define __IPATCH_SF2_FILE_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchRiff.h>

/* forward type declarations */

typedef struct _IpatchSF2File IpatchSF2File;
typedef struct _IpatchSF2FileClass IpatchSF2FileClass;
typedef struct _IpatchSF2Phdr IpatchSF2Phdr;
typedef struct _IpatchSF2Ihdr IpatchSF2Ihdr;
typedef struct _IpatchSF2Shdr IpatchSF2Shdr;
typedef struct _IpatchSF2Bag IpatchSF2Bag;

#define IPATCH_TYPE_SF2_FILE   (ipatch_sf2_file_get_type ())
#define IPATCH_SF2_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_FILE, IpatchSF2File))
#define IPATCH_SF2_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_FILE, \
  IpatchSF2FileClass))
#define IPATCH_IS_SF2_FILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_FILE))
#define IPATCH_IS_SF2_FILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_FILE))
#define IPATCH_SF2_FILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_FILE, \
  IpatchSF2FileClass))

/* SoundFont file object (derived from IpatchFile) */
struct _IpatchSF2File
{
  IpatchFile parent_instance;
  guint32 sample_pos;		/* position in file of the sample data */
  guint32 sample_size;		/* sample data chunk size (in samples) */
  guint32 sample24_pos;	/* position in file of LS bytes of 24 bit samples or 0 */
};

/* SoundFont file class (derived from IpatchFile) */
struct _IpatchSF2FileClass
{
  IpatchFileClass parent_class;
};


/* SoundFont file preset header */
struct _IpatchSF2Phdr
{
  char name[20];		/* preset name */
  guint16 program;		/* MIDI program number */
  guint16 bank;			/* MIDI bank number */
  guint16 bag_index;	      /* index into preset bag (#IPFileBag) */
  guint32 library;		/* Not used (preserved) */
  guint32 genre;		/* Not used (preserved) */
  guint32 morphology;		/* Not used (preserved) */
};

/* SoundFont file instrument header */
struct _IpatchSF2Ihdr
{
  char name[20];		/* name of instrument */
  guint16 bag_index;		/* instrument bag index (#IPFileBag) */
};

/* SoundFont file sample header */
struct _IpatchSF2Shdr
{
  char name[20];		/* sample name */
  guint32 start;		/* offset to start of sample */
  guint32 end;			/* offset to end of sample */
  guint32 loop_start;		/* offset to start of loop */
  guint32 loop_end;		/* offset to end of loop */
  guint32 rate;			/* sample rate recorded at */
  guint8 root_note;		/* root midi note number */
  gint8 fine_tune;		/* pitch correction in cents */
  guint16 link_index;	  /* linked sample index for stereo samples */
  guint16 type;	       /* type of sample (see IpatchSF2SampleFlags) */
};

/* SoundFont file bag (zone), indexes for zone's generators and modulators */
struct _IpatchSF2Bag
{
  guint16 mod_index;		/* index into modulator list */
  guint16 gen_index;		/* index into generator list */
};


/* RIFF chunk FOURCC guint32 integers */
#define IPATCH_SFONT_FOURCC_SFBK  IPATCH_FOURCC ('s','f','b','k')
#define IPATCH_SFONT_FOURCC_INFO  IPATCH_FOURCC ('I','N','F','O')
#define IPATCH_SFONT_FOURCC_SDTA  IPATCH_FOURCC ('s','d','t','a')
#define IPATCH_SFONT_FOURCC_PDTA  IPATCH_FOURCC ('p','d','t','a')
#define IPATCH_SFONT_FOURCC_SMPL  IPATCH_FOURCC ('s','m','p','l')
#define IPATCH_SFONT_FOURCC_SM24  IPATCH_FOURCC ('s','m','2','4')
#define IPATCH_SFONT_FOURCC_PHDR  IPATCH_FOURCC ('p','h','d','r')
#define IPATCH_SFONT_FOURCC_PBAG  IPATCH_FOURCC ('p','b','a','g')
#define IPATCH_SFONT_FOURCC_PMOD  IPATCH_FOURCC ('p','m','o','d')
#define IPATCH_SFONT_FOURCC_PGEN  IPATCH_FOURCC ('p','g','e','n')
#define IPATCH_SFONT_FOURCC_INST  IPATCH_FOURCC ('i','n','s','t')
#define IPATCH_SFONT_FOURCC_IBAG  IPATCH_FOURCC ('i','b','a','g')
#define IPATCH_SFONT_FOURCC_IMOD  IPATCH_FOURCC ('i','m','o','d')
#define IPATCH_SFONT_FOURCC_IGEN  IPATCH_FOURCC ('i','g','e','n')
#define IPATCH_SFONT_FOURCC_SHDR  IPATCH_FOURCC ('s','h','d','r')

/* SoundFont info IDs */
#define IPATCH_SFONT_FOURCC_IFIL  IPATCH_FOURCC ('i','f','i','l')
#define IPATCH_SFONT_FOURCC_ISNG  IPATCH_FOURCC ('i','s','n','g')
#define IPATCH_SFONT_FOURCC_INAM  IPATCH_FOURCC ('I','N','A','M')
#define IPATCH_SFONT_FOURCC_IROM  IPATCH_FOURCC ('i','r','o','m')
#define IPATCH_SFONT_FOURCC_IVER  IPATCH_FOURCC ('i','v','e','r')
#define IPATCH_SFONT_FOURCC_ICRD  IPATCH_FOURCC ('I','C','R','D')
#define IPATCH_SFONT_FOURCC_IENG  IPATCH_FOURCC ('I','E','N','G')
#define IPATCH_SFONT_FOURCC_IPRD  IPATCH_FOURCC ('I','P','R','D')
#define IPATCH_SFONT_FOURCC_ICOP  IPATCH_FOURCC ('I','C','O','P')
#define IPATCH_SFONT_FOURCC_ICMT  IPATCH_FOURCC ('I','C','M','T')
#define IPATCH_SFONT_FOURCC_ISFT  IPATCH_FOURCC ('I','S','F','T')

/* SoundFont file chunk sizes */
#define IPATCH_SFONT_VERSION_SIZE 4 /* file version info size */
#define IPATCH_SFONT_PHDR_SIZE 38  /* file preset header size */
#define IPATCH_SFONT_INST_SIZE 22  /* file instrument header size */
#define IPATCH_SFONT_SHDR_SIZE 46  /* file sample header size */
#define IPATCH_SFONT_BAG_SIZE  4   /* file bag (zone) size */
#define IPATCH_SFONT_MOD_SIZE  10  /* file modulator size */
#define IPATCH_SFONT_GEN_SIZE  4   /* file generator size */
#define IPATCH_SFONT_NAME_SIZE 20  /* name string size (Preset/Inst/Sample) */

/**
 * IpatchSF2FileSampleType:
 * @IPATCH_SF2_FILE_SAMPLE_TYPE_MONO: Mono channel
 * @IPATCH_SF2_FILE_SAMPLE_TYPE_RIGHT: Right channel of a stereo pair
 * @IPATCH_SF2_FILE_SAMPLE_TYPE_LEFT: Left channel of a stereo pair
 * @IPATCH_SF2_FILE_SAMPLE_TYPE_LINKED: Linked list of samples (not yet used)
 * @IPATCH_SF2_FILE_SAMPLE_TYPE_ROM: A ROM sample
 *
 * SoundFont file sample channel mode
 */
typedef enum
{
  IPATCH_SF2_FILE_SAMPLE_TYPE_MONO      = 1 << 0,
  IPATCH_SF2_FILE_SAMPLE_TYPE_RIGHT     = 1 << 1,
  IPATCH_SF2_FILE_SAMPLE_TYPE_LEFT      = 1 << 2,
  IPATCH_SF2_FILE_SAMPLE_TYPE_LINKED    = 1 << 3,
  IPATCH_SF2_FILE_SAMPLE_TYPE_ROM       = 1 << 15
} IpatchSF2FileSampleType;

GType ipatch_sf2_file_get_type (void);
IpatchSF2File *ipatch_sf2_file_new (void);
void ipatch_sf2_file_set_sample_pos (IpatchSF2File *file, guint sample_pos);
guint ipatch_sf2_file_get_sample_pos (IpatchSF2File *file);
void ipatch_sf2_file_set_sample_size (IpatchSF2File *file, guint sample_size);
guint ipatch_sf2_file_get_sample_size (IpatchSF2File *file);
void ipatch_sf2_file_set_sample24_pos (IpatchSF2File *file, guint sample24_pos);
guint ipatch_sf2_file_get_sample24_pos (IpatchSF2File *file);

#endif
