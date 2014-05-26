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
#ifndef __IPATCH_SF2_PRESET_H__
#define __IPATCH_SF2_PRESET_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchSF2PZone.h>
#include <libinstpatch/IpatchSF2Inst.h>

/* forward type declarations */

typedef struct _IpatchSF2Preset IpatchSF2Preset;
typedef struct _IpatchSF2PresetClass IpatchSF2PresetClass;

#define IPATCH_TYPE_SF2_PRESET   (ipatch_sf2_preset_get_type ())
#define IPATCH_SF2_PRESET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_PRESET, \
  IpatchSF2Preset))
#define IPATCH_SF2_PRESET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_PRESET, \
  IpatchSF2PresetClass))
#define IPATCH_IS_SF2_PRESET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_PRESET))
#define IPATCH_IS_SF2_PRESET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_PRESET))
#define IPATCH_SF2_PRESET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_PRESET, \
  IpatchSF2PresetClass))

/* SoundFont preset item */
struct _IpatchSF2Preset
{
  IpatchContainer parent_instance;

  char *name;			/* name of preset */
  guint16 program;		/* MIDI preset map number */
  guint16 bank;			/* MIDI bank map number */
  GSList *zones;		/* list of preset zones */
  GSList *mods;			/* modulators for global zone */
  IpatchSF2GenArray genarray;	/* generator array for global zone */

  guint32 library;		/* Not used (preserved) */
  guint32 genre;		/* Not used (preserved) */
  guint32 morphology;		/* Not used (preserved) */
};

struct _IpatchSF2PresetClass
{
  IpatchContainerClass parent_class;
};

GType ipatch_sf2_preset_get_type (void);
IpatchSF2Preset *ipatch_sf2_preset_new (void);

#define ipatch_sf2_preset_get_zones(preset) \
    ipatch_container_get_children (IPATCH_CONTAINER (preset), \
				   IPATCH_TYPE_SF2_ZONE)

IpatchSF2Preset *ipatch_sf2_preset_first (IpatchIter *iter);
IpatchSF2Preset *ipatch_sf2_preset_next (IpatchIter *iter);

void ipatch_sf2_preset_new_zone (IpatchSF2Preset *preset, IpatchSF2Inst *inst);

void ipatch_sf2_preset_set_name (IpatchSF2Preset *preset, const char *name);
char *ipatch_sf2_preset_get_name (IpatchSF2Preset *preset);
void ipatch_sf2_preset_set_midi_locale (IpatchSF2Preset *preset,
					int bank, int program);
void ipatch_sf2_preset_get_midi_locale (IpatchSF2Preset *preset,
					int *bank, int *program);

int ipatch_sf2_preset_compare (const IpatchSF2Preset *p1,
			       const IpatchSF2Preset *p2);

#endif
