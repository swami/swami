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

/**
 * IPATCH_SFONT_NAME_SIZE: (skip)
 */
#define IPATCH_SFONT_NAME_SIZE 20  /* name string size (Preset/Inst/Sample) */

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

GType ipatch_sf2_file_get_type (void);
IpatchSF2File *ipatch_sf2_file_new (void);
void ipatch_sf2_file_set_sample_pos (IpatchSF2File *file, guint sample_pos);
guint ipatch_sf2_file_get_sample_pos (IpatchSF2File *file);
void ipatch_sf2_file_set_sample_size (IpatchSF2File *file, guint sample_size);
guint ipatch_sf2_file_get_sample_size (IpatchSF2File *file);
void ipatch_sf2_file_set_sample24_pos (IpatchSF2File *file, guint sample24_pos);
guint ipatch_sf2_file_get_sample24_pos (IpatchSF2File *file);

#endif
