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
#ifndef __IPATCH_SF2_GEN_ITEM_H__
#define __IPATCH_SF2_GEN_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/IpatchSF2Gen.h>

/* for generator properties (zones, instruments, and presets) */

/**
 * IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID: (skip)
 */
#define IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID	1	/* first gen prop */

/**
 * IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID: (skip)
 */
#define IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID	80	/* first gen-set prop */

/**
 * IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID: (skip)
 */
#define IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID	160	/* first ID usable for other properties */

/* forward type declarations */
typedef struct _IpatchSF2GenItem IpatchSF2GenItem;              // dummy typedef
typedef struct _IpatchSF2GenItemIface IpatchSF2GenItemIface;

#define IPATCH_TYPE_SF2_GEN_ITEM   (ipatch_sf2_gen_item_get_type ())
#define IPATCH_SF2_GEN_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_SF2_GEN_ITEM, \
   IpatchSF2GenItem))
#define IPATCH_SF2_GEN_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_SF2_GEN_ITEM, \
   IpatchSF2GenItemIface))
#define IPATCH_IS_SF2_GEN_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_SF2_GEN_ITEM))
#define IPATCH_SF2_GEN_ITEM_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), IPATCH_TYPE_SF2_GEN_ITEM, \
   IpatchSF2GenItemIface))

/* generator item interface */
struct _IpatchSF2GenItemIface
{
  GTypeInterface parent_iface;

  /*< public >*/

  IpatchSF2GenPropsType propstype;	/* gen properties type for this class */
  guint genarray_ofs;	/* offset in item instance to generator array pointer */
  GParamSpec *specs[IPATCH_SF2_GEN_COUNT];  /* genid -> prop pspec array */
  GParamSpec *setspecs[IPATCH_SF2_GEN_COUNT];  /* genid -> "-set" prop pspec array */
};

GType ipatch_sf2_gen_item_get_type (void);

gboolean ipatch_sf2_gen_item_get_amount (IpatchSF2GenItem *item, guint genid,
					 IpatchSF2GenAmount *out_amt);
void ipatch_sf2_gen_item_set_amount (IpatchSF2GenItem *item, guint genid,
				     IpatchSF2GenAmount *amt);
void ipatch_sf2_gen_item_set_gen_flag (IpatchSF2GenItem *item, guint genid,
				       gboolean setflag);
guint ipatch_sf2_gen_item_count_set (IpatchSF2GenItem *item);

void ipatch_sf2_gen_item_copy_all (IpatchSF2GenItem *item,
				   IpatchSF2GenArray *array);
void ipatch_sf2_gen_item_copy_set (IpatchSF2GenItem *item,
				   IpatchSF2GenArray *array);
void ipatch_sf2_gen_item_set_note_range (IpatchSF2GenItem *item, int low, int high);
void ipatch_sf2_gen_item_set_velocity_range (IpatchSF2GenItem *item, int low, int high);
gboolean ipatch_sf2_gen_item_in_range (IpatchSF2GenItem *item, int note, int velocity);
gboolean ipatch_sf2_gen_item_intersect_test (IpatchSF2GenItem *item,
                                             const IpatchSF2GenArray *genarray);

GParamSpec *ipatch_sf2_gen_item_class_get_pspec (GObjectClass *klass, guint genid);
GParamSpec *ipatch_sf2_gen_item_class_get_pspec_set (GObjectClass *klass, guint genid);

void
ipatch_sf2_gen_item_iface_install_properties (GObjectClass *klass,
                                              IpatchSF2GenPropsType propstype,
                                              GParamSpec ***specs,
                                              GParamSpec ***setspecs);
gboolean
ipatch_sf2_gen_item_iface_set_property (IpatchSF2GenItem *item,
					guint property_id, const GValue *value);
gboolean
ipatch_sf2_gen_item_iface_get_property (IpatchSF2GenItem *item,
					guint property_id, GValue *value);
#endif
