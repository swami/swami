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
#ifndef __IPATCH_SLI_INST_H__
#define __IPATCH_SLI_INST_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchSLIZone.h>
#include <libinstpatch/IpatchSLISample.h>

/* forward type declarations */
typedef struct _IpatchSLIInst IpatchSLIInst;
typedef struct _IpatchSLIInstClass IpatchSLIInstClass;
typedef struct _IpatchSLIInstCatMapEntry IpatchSLIInstCatMapEntry;

#define IPATCH_TYPE_SLI_INST   (ipatch_sli_inst_get_type ())
#define IPATCH_SLI_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_INST, \
  IpatchSLIInst))
#define IPATCH_SLI_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_INST, \
  IpatchSLIInstClass))
#define IPATCH_IS_SLI_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_INST))
#define IPATCH_IS_SLI_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_INST))
#define IPATCH_SLI_INST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SLI_INST, \
  IpatchSLIInstClass))

/* Index constants for category strings array */
enum  {
  IPATCH_SLI_INST_CAT_80S = 0,
  IPATCH_SLI_INST_CAT_A_SYNTH,
  IPATCH_SLI_INST_CAT_ACID,
  IPATCH_SLI_INST_CAT_ATTACK,
  IPATCH_SLI_INST_CAT_BASS,
  IPATCH_SLI_INST_CAT_BELL,
  IPATCH_SLI_INST_CAT_BIG_BEAT,
  IPATCH_SLI_INST_CAT_BLOCK,
  IPATCH_SLI_INST_CAT_BONGO,
  IPATCH_SLI_INST_CAT_BRASS,
  IPATCH_SLI_INST_CAT_CHIME,
  IPATCH_SLI_INST_CAT_CHINA,
  IPATCH_SLI_INST_CAT_CLAP,
  IPATCH_SLI_INST_CAT_CLAVE,
  IPATCH_SLI_INST_CAT_CONGA,
  IPATCH_SLI_INST_CAT_CRASH,
  IPATCH_SLI_INST_CAT_CUICA,
  IPATCH_SLI_INST_CAT_CYMBAL,
  IPATCH_SLI_INST_CAT_D_SYNTH,
  IPATCH_SLI_INST_CAT_DISCO,
  IPATCH_SLI_INST_CAT_DRUM_LOOP,
  IPATCH_SLI_INST_CAT_EFFECTS,
  IPATCH_SLI_INST_CAT_ELECTRO,
  IPATCH_SLI_INST_CAT_ETHNIC,
  IPATCH_SLI_INST_CAT_EXT_IN,
  IPATCH_SLI_INST_CAT_FB_LOOP,
  IPATCH_SLI_INST_CAT_FX_LOOP,
  IPATCH_SLI_INST_CAT_FUNK,
  IPATCH_SLI_INST_CAT_GONG,
  IPATCH_SLI_INST_CAT_GUIRO,
  IPATCH_SLI_INST_CAT_HIHAT,
  IPATCH_SLI_INST_CAT_HIPHOP,
  IPATCH_SLI_INST_CAT_HOUSE,
  IPATCH_SLI_INST_CAT_HUMAN,
  IPATCH_SLI_INST_CAT_INDUSTRY,
  IPATCH_SLI_INST_CAT_JAZZ,
  IPATCH_SLI_INST_CAT_KICK,
  IPATCH_SLI_INST_CAT_LEAD,
  IPATCH_SLI_INST_CAT_MARCH,
  IPATCH_SLI_INST_CAT_MARIMBA,
  IPATCH_SLI_INST_CAT_MULTI,
  IPATCH_SLI_INST_CAT_NATURAL,
  IPATCH_SLI_INST_CAT_OLDIE,
  IPATCH_SLI_INST_CAT_ORGAN,
  IPATCH_SLI_INST_CAT_OTHER,
  IPATCH_SLI_INST_CAT_PAD,
  IPATCH_SLI_INST_CAT_PERC_LOOP,
  IPATCH_SLI_INST_CAT_PERCUSSION,
  IPATCH_SLI_INST_CAT_PERCUSSIVE,
  IPATCH_SLI_INST_CAT_PIANO,
  IPATCH_SLI_INST_CAT_PLUG,
  IPATCH_SLI_INST_CAT_POP,
  IPATCH_SLI_INST_CAT_RELEASE,
  IPATCH_SLI_INST_CAT_RIDE,
  IPATCH_SLI_INST_CAT_ROCK,
  IPATCH_SLI_INST_CAT_SCRATCH,
  IPATCH_SLI_INST_CAT_SEQUENCER,
  IPATCH_SLI_INST_CAT_SHAKER,
  IPATCH_SLI_INST_CAT_SNARE,
  IPATCH_SLI_INST_CAT_SPLASH,
  IPATCH_SLI_INST_CAT_STRING,
  IPATCH_SLI_INST_CAT_SYNTH_BASS,
  IPATCH_SLI_INST_CAT_TR_ALIKE,
  IPATCH_SLI_INST_CAT_TECHNO,
  IPATCH_SLI_INST_CAT_TEXTURE,
  IPATCH_SLI_INST_CAT_TIMBALE,
  IPATCH_SLI_INST_CAT_TOM,
  IPATCH_SLI_INST_CAT_TONAL_LOOP,
  IPATCH_SLI_INST_CAT_TRIANGLE,
  IPATCH_SLI_INST_CAT_VOICE,
  IPATCH_SLI_INST_CAT_WHISTLE,
  IPATCH_SLI_INST_CAT_WIND,
  IPATCH_SLI_INST_CAT_WORLD
};

struct _IpatchSLIInstCatMapEntry
{
  char code;                        /* cat code */
  guint name_idx;                   /* cat string index */
  const IpatchSLIInstCatMapEntry *submap; /* poniter to subcat map array */
};

/* Defined in IpatchSLIInst_CatMaps.c */
extern const char *ipatch_sli_inst_cat_strings[];
extern const IpatchSLIInstCatMapEntry ipatch_sli_inst_cat_map[];

/* SoundFont instrument item */
struct _IpatchSLIInst
{
  IpatchContainer parent_instance;

  char *name;			/* name of inst */
  GSList *zones;		/* list of inst zones */
  guint sound_id;		/* instrument identifier */
  guint category;		/* category code for grouping */
};

struct _IpatchSLIInstClass
{
  IpatchContainerClass parent_class;
};

GType ipatch_sli_inst_get_type (void);
IpatchSLIInst *ipatch_sli_inst_new (void);

#define ipatch_sli_inst_zones_count(inst) \
    ipatch_container_count (IPATCH_CONTAINER (inst), \
                            IPATCH_TYPE_SLI_ZONE)
#define ipatch_sli_inst_get_zones(inst) \
    ipatch_container_get_children (IPATCH_CONTAINER (inst), \
				   IPATCH_TYPE_SLI_ZONE)

IpatchSLIInst *ipatch_sli_inst_first (IpatchIter *iter);
IpatchSLIInst *ipatch_sli_inst_next (IpatchIter *iter);

void ipatch_sli_inst_new_zone (IpatchSLIInst *inst, IpatchSLISample *sample);

void ipatch_sli_inst_set_name (IpatchSLIInst *inst, const char *name);
char *ipatch_sli_inst_get_name (IpatchSLIInst *inst);

char *ipatch_sli_inst_get_category_as_path (IpatchSLIInst *inst);
#endif
