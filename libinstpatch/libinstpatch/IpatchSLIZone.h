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
#ifndef __IPATCH_SLI_ZONE_H__
#define __IPATCH_SLI_ZONE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchSF2Gen.h>
#include <libinstpatch/IpatchSLISample.h>

/* forward type declarations */

typedef struct _IpatchSLIZone IpatchSLIZone;
typedef struct _IpatchSLIZoneClass IpatchSLIZoneClass;

#define IPATCH_TYPE_SLI_ZONE   (ipatch_sli_zone_get_type ())
#define IPATCH_SLI_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SLI_ZONE, \
  IpatchSLIZone))
#define IPATCH_SLI_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SLI_ZONE, \
  IpatchSLIZoneClass))
#define IPATCH_IS_SLI_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SLI_ZONE))
#define IPATCH_IS_SLI_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_SLI_ZONE))
#define IPATCH_SLI_ZONE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_SLI_ZONE, \
  IpatchSLIZoneClass))

/* SoundFont zone item */
struct _IpatchSLIZone
{
  IpatchItem parent_instance;

  /*< private >*/

  IpatchSLISample *sample;	/* referenced sample */
  IpatchSF2GenArray genarray;	/* generator array */
  int flags;                    /* misc flags */
};

struct _IpatchSLIZoneClass
{
  IpatchItemClass parent_class;
};

/**
 * IPATCH_SLI_ZONE_UNUSED_FLAG_SHIFT: (skip)
 */
/* reserve 2 flags */
#define IPATCH_SLI_ZONE_UNUSED_FLAG_SHIFT  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 2)

/* a macro for getting or setting a generator value directly, normally
   ipatch_sli_zone_(get/set)_gen should be used instead, zone is not locked
   and no flags are set (should only be used on zones with exclusive access) */
#define IPATCH_SLI_ZONE_GEN_AMT(zone, genid) \
  ((IpatchSLIZone *)zone)->genarray.values[genid]

/* macros for manipulating zone generator set flags directly, should not
   normally be used except in the case of exclusive access, not locked, etc */
#define IPATCH_SLI_ZONE_GEN_TEST_FLAG(zone, genid) \
  IPATCH_SLI_GEN_ARRAY_TEST_FLAG (&((IpatchSLIZone *)(zone))->genarray, genid)
#define IPATCH_SLI_ZONE_GEN_SET_FLAG(zone, genid) \
  IPATCH_SLI_GEN_ARRAY_SET_FLAG (&((IpatchSLIZone *)(zone))->genarray, genid)
#define IPATCH_SLI_ZONE_GEN_CLEAR_FLAG(zone, genid) \
  IPATCH_SLI_GEN_ARRAY_CLEAR_FLAG (&((IpatchSLIZone *)(zone))->genarray, genid)

GType ipatch_sli_zone_get_type (void);
IpatchSLIZone *ipatch_sli_zone_new (void);

IpatchSLIZone *ipatch_sli_zone_first (IpatchIter *iter);
IpatchSLIZone *ipatch_sli_zone_next (IpatchIter *iter);

void ipatch_sli_zone_set_sample (IpatchSLIZone *zone, IpatchSLISample *sample);
IpatchSLISample *ipatch_sli_zone_get_sample (IpatchSLIZone *zone);
IpatchSLISample *ipatch_sli_zone_peek_sample (IpatchSLIZone *zone);
#endif
