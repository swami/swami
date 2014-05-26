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
#ifndef __IPATCH_SF2_INST_H__
#define __IPATCH_SF2_INST_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchSF2IZone.h>
#include <libinstpatch/IpatchSF2Sample.h>

/* forward type declarations */

typedef struct _IpatchSF2Inst IpatchSF2Inst;
typedef struct _IpatchSF2InstClass IpatchSF2InstClass;

#define IPATCH_TYPE_SF2_INST   (ipatch_sf2_inst_get_type ())
#define IPATCH_SF2_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_INST, \
  IpatchSF2Inst))
#define IPATCH_SF2_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_INST, \
  IpatchSF2InstClass))
#define IPATCH_IS_SF2_INST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_INST))
#define IPATCH_IS_SF2_INST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_INST))
#define IPATCH_SF2_INST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_INST, \
  IpatchSF2InstClass))

/* SoundFont instrument item */
struct _IpatchSF2Inst
{
  IpatchContainer parent_instance;

  char *name;			/* name of inst */
  GSList *zones;		/* list of inst zones */
  GSList *mods;			/* modulators for global zone */
  IpatchSF2GenArray genarray;	/* generator array for global zone */
};

struct _IpatchSF2InstClass
{
  IpatchContainerClass parent_class;
};

GType ipatch_sf2_inst_get_type (void);
IpatchSF2Inst *ipatch_sf2_inst_new (void);

#define ipatch_sf2_inst_get_zones(inst) \
    ipatch_container_get_children (IPATCH_CONTAINER (inst), \
				   IPATCH_TYPE_SF2_ZONE)

IpatchSF2Inst *ipatch_sf2_inst_first (IpatchIter *iter);
IpatchSF2Inst *ipatch_sf2_inst_next (IpatchIter *iter);

void ipatch_sf2_inst_new_zone (IpatchSF2Inst *inst, IpatchSF2Sample *sample);

void ipatch_sf2_inst_set_name (IpatchSF2Inst *inst, const char *name);
char *ipatch_sf2_inst_get_name (IpatchSF2Inst *inst);

#endif
