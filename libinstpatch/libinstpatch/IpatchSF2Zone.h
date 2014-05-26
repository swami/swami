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
#ifndef __IPATCH_SF2_ZONE_H__
#define __IPATCH_SF2_ZONE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSF2Gen.h>
#include <libinstpatch/IpatchSF2Mod.h>

/* forward type declarations */

typedef struct _IpatchSF2Zone IpatchSF2Zone;
typedef struct _IpatchSF2ZoneClass IpatchSF2ZoneClass;

#define IPATCH_TYPE_SF2_ZONE   (ipatch_sf2_zone_get_type ())
#define IPATCH_SF2_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_ZONE, \
  IpatchSF2Zone))
#define IPATCH_SF2_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_ZONE, \
  IpatchSF2ZoneClass))
#define IPATCH_IS_SF2_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_ZONE))
#define IPATCH_IS_SF2_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SF2_ZONE))
#define IPATCH_SF2_ZONE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SF2_ZONE, \
  IpatchSF2ZoneClass))

/* SoundFont zone item */
struct _IpatchSF2Zone
{
  IpatchItem parent_instance;

  /*< private >*/

  IpatchItem *item;		/* referenced item */
  GSList *mods;			/* modulators */
  IpatchSF2GenArray genarray;	/* generator array */
};

struct _IpatchSF2ZoneClass
{
  IpatchItemClass parent_class;
};

/* reserve 2 flags */
#define IPATCH_SF2_ZONE_UNUSED_FLAG_SHIFT  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 2)

/* a macro for getting or setting a generator value directly, normally
   ipatch_sf2_zone_(get/set)_gen should be used instead, zone is not locked
   and no flags are set (should only be used on zones with exclusive access) */
#define IPATCH_SF2_ZONE_GEN_AMT(zone, genid) \
  ((IpatchSF2Zone *)zone)->genarray.values[genid]

/* macros for manipulating zone generator set flags directly, should not
   normally be used except in the case of exclusive access, not locked, etc */
#define IPATCH_SF2_ZONE_GEN_TEST_FLAG(zone, genid) \
  IPATCH_SF2_GEN_ARRAY_TEST_FLAG (&((IpatchSF2Zone *)(zone))->genarray, genid)
#define IPATCH_SF2_ZONE_GEN_SET_FLAG(zone, genid) \
  IPATCH_SF2_GEN_ARRAY_SET_FLAG (&((IpatchSF2Zone *)(zone))->genarray, genid)
#define IPATCH_SF2_ZONE_GEN_CLEAR_FLAG(zone, genid) \
  IPATCH_SF2_GEN_ARRAY_CLEAR_FLAG (&((IpatchSF2Zone *)(zone))->genarray, genid)

GType ipatch_sf2_zone_get_type (void);

IpatchSF2Zone *ipatch_sf2_zone_first (IpatchIter *iter);
IpatchSF2Zone *ipatch_sf2_zone_next (IpatchIter *iter);

void ipatch_sf2_zone_set_link_item (IpatchSF2Zone *zone, IpatchItem *item);
gboolean ipatch_sf2_zone_set_link_item_no_notify (IpatchSF2Zone *zone,
						  IpatchItem *item,
						  IpatchItem **olditem);
IpatchItem *ipatch_sf2_zone_get_link_item (IpatchSF2Zone *zone);
IpatchItem *ipatch_sf2_zone_peek_link_item (IpatchSF2Zone *zone);

#endif
