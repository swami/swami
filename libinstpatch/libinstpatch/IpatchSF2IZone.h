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
#ifndef __IPATCH_SF2_IZONE_H__
#define __IPATCH_SF2_IZONE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchSF2Zone.h>
#include <libinstpatch/IpatchSF2Sample.h>

/* forward type declarations */

typedef struct _IpatchSF2IZone IpatchSF2IZone;
typedef struct _IpatchSF2IZoneClass IpatchSF2IZoneClass;

#define IPATCH_TYPE_SF2_IZONE   (ipatch_sf2_izone_get_type ())
#define IPATCH_SF2_IZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_IZONE, \
  IpatchSF2IZone))
#define IPATCH_SF2_IZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_IZONE, \
  IpatchSF2IZoneClass))
#define IPATCH_IS_SF2_IZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_IZONE))
#define IPATCH_IS_SF2_IZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_IZONE))
#define IPATCH_SF2_IZONE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_IZONE, \
  IpatchSF2IZoneClass))

/* SoundFont izone item */
struct _IpatchSF2IZone
{
  IpatchSF2Zone parent_instance;
};

struct _IpatchSF2IZoneClass
{
  IpatchSF2ZoneClass parent_class;
};

/* reserve 2 flags */
#define IPATCH_SF2_IZONE_UNUSED_FLAG_SHIFT \
  (IPATCH_SF2_ZONE_UNUSED_FLAG_SHIFT + 2)


GType ipatch_sf2_izone_get_type (void);
IpatchSF2IZone *ipatch_sf2_izone_new (void);

IpatchSF2IZone *ipatch_sf2_izone_first (IpatchIter *iter);
IpatchSF2IZone *ipatch_sf2_izone_next (IpatchIter *iter);

void ipatch_sf2_izone_set_sample (IpatchSF2IZone *izone,
				  IpatchSF2Sample *sample);
IpatchSF2Sample *ipatch_sf2_izone_get_sample (IpatchSF2IZone *izone);
IpatchSF2IZone *ipatch_sf2_izone_get_stereo_link (IpatchSF2IZone *izone);

#endif
