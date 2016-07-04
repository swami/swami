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
#ifndef __IPATCH_GIG_REGION_H__
#define __IPATCH_GIG_REGION_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchGigRegion IpatchGigRegion;
typedef struct _IpatchGigRegionClass IpatchGigRegionClass;

#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchGigEffects.h>
#include <libinstpatch/IpatchGigSample.h>
#include <libinstpatch/IpatchGigDimension.h>
#include <libinstpatch/IpatchGigSubRegion.h>

#define IPATCH_TYPE_GIG_REGION   (ipatch_gig_region_get_type ())
#define IPATCH_GIG_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_REGION, \
  IpatchGigRegion))
#define IPATCH_GIG_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_REGION, \
  IpatchGigRegionClass))
#define IPATCH_IS_GIG_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_REGION))
#define IPATCH_IS_GIG_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_REGION))
#define IPATCH_GIG_REGION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_REGION, \
  IpatchGigRegionClass))

/* GigaSampler region object */
struct _IpatchGigRegion
{
  IpatchContainer parent_instance;

  /*< private >*/

  guint8 note_range_low;	/* MIDI note range low value */
  guint8 note_range_high;	/* MIDI note range high value */
  guint8 velocity_range_low;	/* MIDI velocity range low value */
  guint8 velocity_range_high;	/* MIDI velocity range high value */

  guint16 key_group;		/* Exclusive key group number or 0 */
  guint16 layer_group;		/* layer group (descriptive only) */

  guint16 phase_group;		/* Phase locked group number or 0 */
  guint16 channel;	/* channel ID (IpatchDLS2RegionChannelType) */

  IpatchDLS2Info *info;		/* info string values */

  guint8 dimension_count;	/* dimension count (0-5) */
  guint8 sub_region_count;	/* 2 ^ sum (dimensions[].split_count) (1-32) */
  IpatchGigDimension *dimensions[5];	/* [dimension_count] */
  IpatchGigSubRegion *sub_regions[32];	/* [sub_region_count] */
  guint8 chunk_3ddp[10];   /* FIXME - what is it? (16 bits / dimension?) */
};

/* GigaSampler region class */
struct _IpatchGigRegionClass
{
  IpatchContainerClass parent_class;
};

/* Flags crammed into IpatchItem flags (ditched 2 - 16 bit flag fields) */
/* FIXME - Are these used in GigaSampler files? */
typedef enum
{
  IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE = 1 << IPATCH_CONTAINER_UNUSED_FLAG_SHIFT,
  IPATCH_GIG_REGION_PHASE_MASTER = 1 << (IPATCH_CONTAINER_UNUSED_FLAG_SHIFT + 1),
  IPATCH_GIG_REGION_MULTI_CHANNEL = 1 << (IPATCH_CONTAINER_UNUSED_FLAG_SHIFT + 2)
} IpatchGigRegionFlags;

/**
 * IPATCH_GIG_REGION_FLAG_MASK: (skip)
 */
#define IPATCH_GIG_REGION_FLAG_MASK  (0x0F << IPATCH_CONTAINER_UNUSED_FLAG_SHIFT)

/**
 * IPATCH_GIG_REGION_UNUSED_FLAG_SHIFT: (skip)
 */
/* 3 flags + 1 for expansion */
#define IPATCH_GIG_REGION_UNUSED_FLAG_SHIFT \
  (IPATCH_CONTAINER_UNUSED_FLAG_SHIFT + 4)


GType ipatch_gig_region_get_type (void);
IpatchGigRegion *ipatch_gig_region_new (void);

IpatchGigRegion *ipatch_gig_region_first (IpatchIter *iter);
IpatchGigRegion *ipatch_gig_region_next (IpatchIter *iter);

void ipatch_gig_region_set_note_range (IpatchGigRegion *region,
				       int low, int high);
void ipatch_gig_region_set_velocity_range (IpatchGigRegion *region,
					   int low, int high);

void ipatch_gig_region_new_dimension (IpatchGigRegion *region,
				      IpatchGigDimensionType type,
				      int split_count);
void ipatch_gig_region_remove_dimension (IpatchGigRegion *region,
					 int dim_index, int split_index);
#endif
