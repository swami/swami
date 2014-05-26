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
#ifndef __IPATCH_SF2_PZONE_H__
#define __IPATCH_SF2_PZONE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSF2Zone.h>
#include <libinstpatch/IpatchSF2Inst.h>

/* forward type declarations */

typedef struct _IpatchSF2PZone IpatchSF2PZone;
typedef struct _IpatchSF2PZoneClass IpatchSF2PZoneClass;

#define IPATCH_TYPE_SF2_PZONE   (ipatch_sf2_pzone_get_type ())
#define IPATCH_SF2_PZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_PZONE, \
  IpatchSF2PZone))
#define IPATCH_SF2_PZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_PZONE, \
  IpatchSF2PZoneClass))
#define IPATCH_IS_SF2_PZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_PZONE))
#define IPATCH_IS_SF2_PZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_PZONE))
#define IPATCH_SF2_PZONE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_PZONE, \
  IpatchSF2PZoneClass))

/* SoundFont pzone item */
struct _IpatchSF2PZone
{
  IpatchSF2Zone parent_instance;
};

struct _IpatchSF2PZoneClass
{
  IpatchSF2ZoneClass parent_class;
};

GType ipatch_sf2_pzone_get_type (void);
IpatchSF2PZone *ipatch_sf2_pzone_new (void);

IpatchSF2PZone *ipatch_sf2_pzone_first (IpatchIter *iter);
IpatchSF2PZone *ipatch_sf2_pzone_next (IpatchIter *iter);

void ipatch_sf2_pzone_set_inst (IpatchSF2PZone *pzone, IpatchSF2Inst *inst);
IpatchSF2Inst *ipatch_sf2_pzone_get_inst (IpatchSF2PZone *pzone);

#endif
