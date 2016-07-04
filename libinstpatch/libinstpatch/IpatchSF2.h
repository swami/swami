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
#ifndef __IPATCH_SF2_H__
#define __IPATCH_SF2_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchBase.h>
#include <libinstpatch/IpatchSF2Preset.h>
#include <libinstpatch/IpatchSF2Inst.h>
#include <libinstpatch/IpatchSF2Sample.h>
#include <libinstpatch/IpatchSF2File.h>

/* forward type declarations */

typedef struct _IpatchSF2 IpatchSF2;
typedef struct _IpatchSF2Class IpatchSF2Class;

#define IPATCH_TYPE_SF2   (ipatch_sf2_get_type ())
#define IPATCH_SF2(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2, IpatchSF2))
#define IPATCH_SF2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2, IpatchSF2Class))
#define IPATCH_IS_SF2(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2))
#define IPATCH_IS_SF2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2))
#define IPATCH_SF2_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2, IpatchSF2Class))

/* SoundFont object */
struct _IpatchSF2
{
  /*< public >*/
  IpatchBase parent_instance;

  guint16 ver_major, ver_minor;	/* SoundFont version */
  guint16 romver_major, romver_minor; /* ROM version (if any) */

  GHashTable *info;		/* hash of info strings by 4 char ID */
  GSList *presets;		/* list of #IpatchSF2Preset structures */
  GSList *insts;		/* list of #IpatchSF2Inst structures */
  GSList *samples;		/* list of #IpatchSF2Sample structures */
};

/* SoundFont class */
struct _IpatchSF2Class
{
  IpatchBaseClass parent_class;
};

/**
 * IpatchSF2Flags:
 * @IPATCH_SF2_SAMPLES_24BIT: SoundFont 24 bit samples enabled flag
 */
typedef enum
{
  IPATCH_SF2_SAMPLES_24BIT = 1 << IPATCH_BASE_UNUSED_FLAG_SHIFT
} IpatchSF2Flags;

/**
 * IPATCH_SF2_UNUSED_FLAG_SHIFT: (skip)
 */
/* we reserve a couple flags for backwards compatible expansion */
#define IPATCH_SF2_UNUSED_FLAG_SHIFT (IPATCH_BASE_UNUSED_FLAG_SHIFT + 3)


/* SoundFont INFO enums (keep order synced with IpatchSF2.c) */
typedef enum
{
  IPATCH_SF2_UNKNOWN,		/* unknown chunk ("NULL" value) */
  IPATCH_SF2_VERSION = IPATCH_SFONT_FOURCC_IFIL, /* SoundFont version */
  IPATCH_SF2_ENGINE = IPATCH_SFONT_FOURCC_ISNG,/* target sound engine */
  IPATCH_SF2_NAME = IPATCH_SFONT_FOURCC_INAM, /* SoundFont name */
  IPATCH_SF2_ROM_NAME = IPATCH_SFONT_FOURCC_IROM, /* ROM name */
  IPATCH_SF2_ROM_VERSION = IPATCH_SFONT_FOURCC_IVER, /* ROM version */
  IPATCH_SF2_DATE = IPATCH_SFONT_FOURCC_ICRD, /* creation date */
  IPATCH_SF2_AUTHOR = IPATCH_SFONT_FOURCC_IENG,/* sound designers/engineers */
  IPATCH_SF2_PRODUCT = IPATCH_SFONT_FOURCC_IPRD, /* product intended for */
  IPATCH_SF2_COPYRIGHT = IPATCH_SFONT_FOURCC_ICOP, /* copyright message */
  IPATCH_SF2_COMMENT = IPATCH_SFONT_FOURCC_ICMT, /* comments */
  IPATCH_SF2_SOFTWARE = IPATCH_SFONT_FOURCC_ISFT /* software "create:modify" */
}IpatchSF2InfoType;

/**
 * IPATCH_SF2_INFO_COUNT: (skip)
 */
#define IPATCH_SF2_INFO_COUNT 11

/**
 * IPATCH_SF2_DEFAULT_ENGINE: (skip)
 */
#define IPATCH_SF2_DEFAULT_ENGINE "EMU8000"

/* structure used for ipatch_sf2_get_info_array */
typedef struct
{
  guint32 id;			/* FOURCC info id */
  char *val;			/* info string value */
} IpatchSF2Info;

GType ipatch_sf2_get_type (void);
IpatchSF2 *ipatch_sf2_new (void);

#define ipatch_sf2_get_presets(sfont) \
    ipatch_container_get_children (IPATCH_CONTAINER (sfont), \
				   IPATCH_TYPE_SF2_PRESET)
#define ipatch_sf2_get_insts(sfont) \
    ipatch_container_get_children (IPATCH_CONTAINER (sfont), \
				   IPATCH_TYPE_SF2_INST)
#define ipatch_sf2_get_samples(sfont) \
    ipatch_container_get_children (IPATCH_CONTAINER (sfont), \
				   IPATCH_TYPE_SF2_SAMPLE)

void ipatch_sf2_set_file (IpatchSF2 *sf, IpatchSF2File *file);
IpatchSF2File *ipatch_sf2_get_file (IpatchSF2 *sf);

char *ipatch_sf2_get_info (IpatchSF2 *sf, IpatchSF2InfoType id);
void ipatch_sf2_set_info (IpatchSF2 *sf, IpatchSF2InfoType id,
			  const char *val);
IpatchSF2Info *ipatch_sf2_get_info_array (IpatchSF2 *sf);
void ipatch_sf2_free_info_array (IpatchSF2Info *array);
gboolean ipatch_sf2_info_id_is_valid (guint32 id);
int ipatch_sf2_get_info_max_size (IpatchSF2InfoType infotype);

IpatchSF2Preset *ipatch_sf2_find_preset (IpatchSF2 *sf, const char *name,
					 int bank, int program,
					 const IpatchSF2Preset *exclude);
IpatchSF2Inst *ipatch_sf2_find_inst (IpatchSF2 *sf, const char *name,
				     const IpatchSF2Inst *exclude);
IpatchSF2Sample *ipatch_sf2_find_sample (IpatchSF2 *sf, const char *name,
					 const IpatchSF2Sample *exclude);
IpatchList *ipatch_sf2_get_zone_references (IpatchItem *item);

char *ipatch_sf2_make_unique_name (IpatchSF2 *sfont, GType child_type,
				   const char *name,
				   const IpatchItem *exclude);
#endif
