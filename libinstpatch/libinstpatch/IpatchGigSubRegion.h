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
#ifndef __IPATCH_GIG_SUB_REGION_H__
#define __IPATCH_GIG_SUB_REGION_H__

#include <glib.h>
#include <glib-object.h>

/* forward type declarations */
typedef struct _IpatchGigSubRegion IpatchGigSubRegion;
typedef struct _IpatchGigSubRegionClass IpatchGigSubRegionClass;

#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchGigEffects.h>
#include <libinstpatch/IpatchGigSample.h>

#define IPATCH_TYPE_GIG_SUB_REGION   (ipatch_gig_sub_region_get_type ())
#define IPATCH_GIG_SUB_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_GIG_SUB_REGION, \
  IpatchGigSubRegion))
#define IPATCH_GIG_SUB_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_GIG_SUB_REGION, \
  IpatchGigSubRegionClass))
#define IPATCH_IS_GIG_SUB_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_GIG_SUB_REGION))
#define IPATCH_IS_GIG_SUB_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_GIG_SUB_REGION))
#define IPATCH_GIG_SUB_REGION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_GIG_SUB_REGION, \
  IpatchGigSubRegionClass))

/* a GigaSampler sub region */
struct _IpatchGigSubRegion
{
  IpatchItem parent_instance;

  IpatchGigEffects effects;	/* effects for this sub region */
  IpatchGigSample *sample;	/* sample for this sub region */
  IpatchDLS2SampleInfo *sample_info; /* sample info override or NULL */
};

/* GigaSampler sub region class */
struct _IpatchGigSubRegionClass
{
  IpatchItemClass parent_class;
};

/* Flags crammed into IpatchItem flags */
typedef enum
{
  IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE = 1 << IPATCH_ITEM_UNUSED_FLAG_SHIFT
} IpatchGigSubRegionFlags;

/**
 * IPATCH_GIG_SUB_REGION_UNUSED_FLAG_SHIFT: (skip)
 */
/* 1 flag + 3 for expansion */
#define IPATCH_GIG_SUB_REGION_UNUSED_FLAG_SHIFT \
  (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 4)


GType ipatch_gig_sub_region_get_type (void);
IpatchGigSubRegion *ipatch_gig_sub_region_new (void);

IpatchGigSubRegion *ipatch_gig_sub_region_first (IpatchIter *iter);
IpatchGigSubRegion *ipatch_gig_sub_region_next (IpatchIter *iter);

IpatchGigSample *ipatch_gig_sub_region_get_sample (IpatchGigSubRegion *subregion);
void ipatch_gig_sub_region_set_sample (IpatchGigSubRegion *subregion,
				       IpatchGigSample *sample);

#endif
