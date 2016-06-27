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
/**
 * SECTION: IpatchSF2IZone
 * @short_description: SoundFont instrument zone object
 * @see_also: #IpatchSF2Inst, #IpatchSF2Sample
 * @stability: Stable
 *
 * Instrument zones are children to #IpatchSF2Inst objects and define how
 * their referenced #IpatchSF2Sample is synthesized.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2IZone.h"
#include "IpatchSF2GenItem.h"
#include "IpatchSample.h"
#include "IpatchContainer.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

enum
{ /* generator IDs are used for lower numbers */
  PROP_LINK_ITEM = IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID,
  PROP_SAMPLE_SIZE,
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_SAMPLE_DATA,
  PROP_LOOP_TYPE,
  PROP_LOOP_START,
  PROP_LOOP_END,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE
};

static void ipatch_sf2_izone_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean ipatch_sf2_izone_sample_iface_open (IpatchSampleHandle *handle,
                                                    GError **err);
static void ipatch_sf2_izone_class_init (IpatchSF2IZoneClass *klass);
static void ipatch_sf2_izone_gen_item_iface_init
  (IpatchSF2GenItemIface *genitem_iface);
static void ipatch_sf2_izone_init (IpatchSF2IZone *izone);
static inline void ipatch_sf2_izone_get_root_note (IpatchSF2IZone *izone,
                                                   GValue *value);
static inline void ipatch_sf2_izone_get_fine_tune (IpatchSF2IZone *izone,
                                                   GValue *value);
static void ipatch_sf2_izone_set_property (GObject *object,
					   guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void ipatch_sf2_izone_get_property (GObject *object,
					   guint property_id, GValue *value,
					   GParamSpec *pspec);
/* For quicker access without lookup */
static GParamSpec *root_note_pspec;
static GParamSpec *fine_tune_pspec;

/* For passing data from class init to gen item interface init */
static GParamSpec **gen_item_specs = NULL;
static GParamSpec **gen_item_setspecs = NULL;


G_DEFINE_TYPE_WITH_CODE (IpatchSF2IZone, ipatch_sf2_izone,
                         IPATCH_TYPE_SF2_ZONE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sf2_izone_sample_iface_init)
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SF2_GEN_ITEM,
                                                ipatch_sf2_izone_gen_item_iface_init))

/* sample interface initialization */
static void
ipatch_sf2_izone_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_sf2_izone_sample_iface_open;
  sample_iface->loop_types = ipatch_sample_loop_types_standard_release;
}

static gboolean
ipatch_sf2_izone_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchSF2Zone *zone = IPATCH_SF2_ZONE (handle->sample);
  IpatchItem *link_item;
  gboolean retval;

  link_item = ipatch_sf2_zone_get_link_item (zone);     /* ++ ref link_item */
  g_return_val_if_fail (link_item != NULL, FALSE);
  retval = ipatch_sample_handle_cascade_open (handle, IPATCH_SAMPLE (link_item), err);
  g_object_unref (link_item);   /* -- unref link_item */
  return (retval);
}

/* gen item interface initialization */
static void
ipatch_sf2_izone_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface)
{
  genitem_iface->genarray_ofs = G_STRUCT_OFFSET (IpatchSF2Zone, genarray);
  genitem_iface->propstype = IPATCH_SF2_GEN_PROPS_INST;

  g_return_if_fail (gen_item_specs != NULL);
  g_return_if_fail (gen_item_setspecs != NULL);

  memcpy (&genitem_iface->specs, gen_item_specs, sizeof (genitem_iface->specs));
  memcpy (&genitem_iface->setspecs, gen_item_setspecs, sizeof (genitem_iface->setspecs));
  g_free (gen_item_specs);
  g_free (gen_item_setspecs);
}

static void
ipatch_sf2_izone_class_init (IpatchSF2IZoneClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_sf2_izone_get_property;

  item_class->item_set_property = ipatch_sf2_izone_set_property;

  g_object_class_install_property (obj_class, PROP_LINK_ITEM,
		    g_param_spec_object ("link-item", _("Link item"),
					 _("Link item"),
					 IPATCH_TYPE_SF2_SAMPLE,
					 G_PARAM_READWRITE));

  /* properties defined by IpatchSample interface */
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA, "sample-data");
  ipatch_sample_install_property (obj_class, PROP_LOOP_TYPE, "loop-type");
  ipatch_sample_install_property (obj_class, PROP_LOOP_START, "loop-start");
  ipatch_sample_install_property (obj_class, PROP_LOOP_END, "loop-end");
  root_note_pspec = ipatch_sample_install_property (obj_class,
                                                    PROP_ROOT_NOTE, "root-note");
  fine_tune_pspec = ipatch_sample_install_property (obj_class,
                                                    PROP_FINE_TUNE, "fine-tune");

  /* install generator properties */
  ipatch_sf2_gen_item_iface_install_properties (obj_class,
                                                IPATCH_SF2_GEN_PROPS_INST,
                                                &gen_item_specs, &gen_item_setspecs);
}

static inline void
ipatch_sf2_izone_get_root_note (IpatchSF2IZone *izone, GValue *value)
{
  IpatchSF2GenAmount amt;
  IpatchSF2Sample *sample;
  int val = 0;

  /* root note override not set or -1? - Get sample root note value. */
  if (!ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
                                       IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amt)
      || amt.sword == -1)
  {		/* root note override not set, get from sample */
    sample = ipatch_sf2_izone_get_sample (izone); /* ++ ref sample */
    if (sample)
    {
      g_object_get (sample, "root-note", &val, NULL);
      g_object_unref (sample); /* -- unref sample */
    }
  }
  else val = amt.uword;

  g_value_set_int (value, val);
}

static inline void
ipatch_sf2_izone_get_fine_tune (IpatchSF2IZone *izone, GValue *value)
{
  IpatchSF2GenAmount amt;
  IpatchSF2Sample *sample;
  int val = 0;

  /* fine tune override set? */
  if (!ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
                                       IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amt))
  {		/* fine tune override not set, get from sample */
    sample = ipatch_sf2_izone_get_sample (izone); /* ++ ref sample */
    if (sample)
    {
      g_object_get (sample, "fine-tune", &val, NULL);
      g_object_unref (sample); /* -- unref sample */
    }
  }
  else val = amt.sword;

  g_value_set_int (value, val);
}

static void
ipatch_sf2_izone_set_property (GObject *object, guint property_id,
			       const GValue *value, GParamSpec *pspec)
{
  IpatchSF2IZone *izone = IPATCH_SF2_IZONE (object);
  IpatchSF2Sample *sample;
  IpatchSF2GenAmount amt;
  GValue vals[2];	/* Gets zeroed below */
  guint genid;
  guint uval;
  int val = 0;

  /* "root-note" and "fine-tune" sample properties get updated for IZone
   * override property or -set property */
  if (property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID)
    genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID;
  else genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID;

  if (genid == IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE)
  {
    memset (vals, 0, sizeof (vals));
    g_value_init (&vals[0], G_TYPE_INT);
    ipatch_sf2_izone_get_root_note (izone, &vals[0]);
  }
  else if (genid == IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE)
  {
    memset (vals, 0, sizeof (vals));
    g_value_init (&vals[0], G_TYPE_INT);
    ipatch_sf2_izone_get_fine_tune (izone, &vals[0]);
  }

  if (ipatch_sf2_gen_item_iface_set_property ((IpatchSF2GenItem *)object,
					      property_id, value))
    {
      if (genid == IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE)
      {
        g_value_init (&vals[1], G_TYPE_INT);
	ipatch_sf2_izone_get_root_note (izone, &vals[1]);
	ipatch_item_prop_notify ((IpatchItem *)object, root_note_pspec,
	                         &vals[1], &vals[0]);
      }
      else if (genid == IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE)
      {
        g_value_init (&vals[1], G_TYPE_INT);
        ipatch_sf2_izone_get_fine_tune (izone, &vals[1]);
	ipatch_item_prop_notify ((IpatchItem *)object, fine_tune_pspec,
	                         &vals[1], &vals[0]);
      }
    }
  else
    {
      switch (property_id)
	{
	case PROP_LINK_ITEM:
	  sample = g_value_get_object (value);
	  g_return_if_fail (IPATCH_IS_SF2_SAMPLE (sample));
	  ipatch_sf2_zone_set_link_item_no_notify ((IpatchSF2Zone *)izone,
						   (IpatchItem *)sample, NULL);
	  break;
	case PROP_SAMPLE_RATE:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_set (sample, "sample-rate", g_value_get_int (value), NULL);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_LOOP_TYPE:
	  val = g_value_get_enum (value);

	  if (val == IPATCH_SAMPLE_LOOP_NONE)
	    amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP;
	  else if (val == IPATCH_SAMPLE_LOOP_RELEASE)
	    amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE;
	  else amt.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;

	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
					  IPATCH_SF2_GEN_SAMPLE_MODES, &amt);
	  break;
	case PROP_LOOP_START:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-start", &uval, NULL);
	      val = g_value_get_uint (value) - uval; /* loop start offset */
	      g_object_unref (sample); /* -- unref sample */

	      if (val >= 0) amt.sword = val >> 15;
	      else amt.sword = -(-val >> 15);

	      ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
				       IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START,
				       &amt);

	      if (val >= 0) amt.sword = val & 0x7FFF;
	      else amt.sword = -(-val & 0x7FFF);

	      ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
				       IPATCH_SF2_GEN_SAMPLE_LOOP_START, &amt);
	    }
	  break;
	case PROP_LOOP_END:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-end", &uval, NULL);
	      val = g_value_get_uint (value) - uval; /* loop end offset */
	      g_object_unref (sample); /* -- unref sample */

	      if (val >= 0) amt.sword = val >> 15;
	      else amt.sword = -(-val >> 15);

	      ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
				       IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END,
				       &amt);

	      if (val >= 0) amt.sword = val & 0x7FFF;
	      else amt.sword = -(-val & 0x7FFF);

	      ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
				       IPATCH_SF2_GEN_SAMPLE_LOOP_END, &amt);
	    }
	  break;
	case PROP_ROOT_NOTE:
	  amt.uword = g_value_get_int (value);
	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
					  IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amt);
	  break;
	case PROP_FINE_TUNE:
	  amt.sword = g_value_get_int (value);
	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (izone),
					  IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amt);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
    }
}

static void
ipatch_sf2_izone_get_property (GObject *object, guint property_id,
			       GValue *value, GParamSpec *pspec)
{
  IpatchSF2IZone *izone = IPATCH_SF2_IZONE (object);
  IpatchSF2Sample *sample;
  IpatchSF2GenAmount amt;
  guint uval = 0;
  int val = 0;

  if (!ipatch_sf2_gen_item_iface_get_property ((IpatchSF2GenItem *)object,
					       property_id, value))
    {
      switch (property_id)
	{
	case PROP_LINK_ITEM:
	  g_value_take_object (value, ipatch_sf2_zone_get_link_item
			       ((IpatchSF2Zone *)izone));
	  break;
	case PROP_SAMPLE_SIZE:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-size", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_FORMAT:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-format", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_RATE:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-rate", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_DATA:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-data", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_LOOP_TYPE:
	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
					  IPATCH_SF2_GEN_SAMPLE_MODES, &amt);

	  if (amt.uword == IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP)
	    val = IPATCH_SAMPLE_LOOP_NONE;
	  else if (amt.uword == IPATCH_SF2_GEN_SAMPLE_MODE_LOOP_RELEASE)
	    val = IPATCH_SAMPLE_LOOP_RELEASE;
	  else val = IPATCH_SAMPLE_LOOP_STANDARD;

	  g_value_set_enum (value, val);
	  break;
	case PROP_LOOP_START:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-start", &uval, NULL);
	      g_object_unref (sample); /* -- unref sample */
	      val = uval;
	    }

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
				   IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START,
				   &amt);
	  val += (int)amt.sword << 15;

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
				   IPATCH_SF2_GEN_SAMPLE_LOOP_START, &amt);
	  val += amt.sword;

	  g_value_set_uint (value, CLAMP (val, 0, G_MAXINT));
	  break;
	case PROP_LOOP_END:
	  sample = ipatch_sf2_izone_get_sample (izone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-end", &uval, NULL);
	      g_object_unref (sample); /* -- unref sample */
	      val = uval;
	    }

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
				   IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END,
				   &amt);
	  val += (int)amt.sword << 15;

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (izone),
				   IPATCH_SF2_GEN_SAMPLE_LOOP_END, &amt);
	  val += amt.sword;

	  g_value_set_uint (value, CLAMP (val, 0, G_MAXINT));
	  break;
	case PROP_ROOT_NOTE:
	  ipatch_sf2_izone_get_root_note (izone, value);
	  break;
	case PROP_FINE_TUNE:
	  ipatch_sf2_izone_get_fine_tune (izone, value);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
    }
}

static
void ipatch_sf2_izone_init (IpatchSF2IZone *izone)
{
  ipatch_sf2_gen_array_init (&((IpatchSF2Zone *)izone)->genarray, FALSE, FALSE);
}

/**
 * ipatch_sf2_izone_new:
 *
 * Create a new SoundFont instrument zone object.
 *
 * Returns: New SoundFont instrument zone with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSF2IZone *
ipatch_sf2_izone_new (void)
{
  return (IPATCH_SF2_IZONE (g_object_new (IPATCH_TYPE_SF2_IZONE, NULL)));
}

/**
 * ipatch_sf2_izone_first: (skip)
 * @iter: Patch item iterator containing #IpatchSF2IZone items
 *
 * Gets the first item in an instrument zone iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first instrument zone in @iter or %NULL if empty.
 */
IpatchSF2IZone *
ipatch_sf2_izone_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SF2_IZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_izone_next: (skip)
 * @iter: Patch item iterator containing #IpatchSF2IZone items
 *
 * Gets the next item in an instrument zone iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next instrument zone in @iter or %NULL if at the end of
 *   the list.
 */
IpatchSF2IZone *
ipatch_sf2_izone_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SF2_IZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_izone_set_sample:
 * @izone: Instrument zone to set referenced sample of
 * @sample: Sample to set instrument zone's referenced item to
 *
 * Sets the referenced sample of an instrument zone.
 */
void
ipatch_sf2_izone_set_sample (IpatchSF2IZone *izone, IpatchSF2Sample *sample)
{
  g_return_if_fail (IPATCH_IS_SF2_IZONE (izone));
  g_return_if_fail (IPATCH_IS_SF2_SAMPLE (sample));

  ipatch_sf2_zone_set_link_item (IPATCH_SF2_ZONE (izone), IPATCH_ITEM (sample));
}

/**
 * ipatch_sf2_izone_get_sample:
 * @izone: Instrument zone to get referenced sample from
 *
 * Gets the referenced sample from an instrument zone.
 * The returned sample's reference count is incremented and the caller
 * is responsible for unrefing it with g_object_unref().
 *
 * Returns: (transfer full): Instrument zone's referenced sample or %NULL if global
 * zone. Remember to unreference the sample with g_object_unref() when
 * done with it.
 */
IpatchSF2Sample *
ipatch_sf2_izone_get_sample (IpatchSF2IZone *izone)
{
  IpatchItem *item;

  g_return_val_if_fail (IPATCH_IS_SF2_IZONE (izone), NULL);

  item = ipatch_sf2_zone_get_link_item (IPATCH_SF2_ZONE (izone));
  return (item ? IPATCH_SF2_SAMPLE (item) : NULL);
}

/**
 * ipatch_sf2_izone_get_stereo_link:
 * @izone: Instrument zone
 *
 * Get the stereo linked instrument zone of another zone.  This is a zone which
 * has the same #IpatchSF2Inst parent and has its link-item set to the counter
 * part of @izone.
 *
 * Returns: (transfer full): Stereo linked instrument zone or %NULL if not stereo or it could not
 *   be found in the same instrument.  Caller owns a reference to the returned
 *   object.
 */
/* FIXME - This function is kind of a hack, until stereo IpatchSF2Sample and
 * IpatchSF2IZones are implemented */
IpatchSF2IZone *
ipatch_sf2_izone_get_stereo_link (IpatchSF2IZone *izone)
{
  IpatchSF2IZone *linked_izone = NULL;
  IpatchSF2Sample *sample = NULL, *linked_sample = NULL;
  IpatchItem *parent = NULL;
  IpatchList *children = NULL;
  IpatchSF2GenAmount z_noterange, cmp_noterange, z_velrange, cmp_velrange;
  int channel;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_SF2_IZONE (izone), NULL);

  sample = ipatch_sf2_izone_get_sample (izone);         /* ++ ref sample */
  if (!sample) return (NULL);

  g_object_get (sample,
                "channel", &channel,
                "linked-sample", &linked_sample,        /* ++ ref linked sample */
                NULL);

  if (channel == IPATCH_SF2_SAMPLE_CHANNEL_MONO || !linked_sample) goto ret;

  parent = ipatch_item_get_parent ((IpatchItem *)izone);        /* ++ ref parent */
  if (!IPATCH_IS_CONTAINER (parent)) goto ret;

  /* ++ ref children */
  if (!(children = ipatch_container_get_children ((IpatchContainer *)parent,
                                                  IPATCH_TYPE_SF2_IZONE)))
    goto ret;

  /* Check likely previous and next zone of izone for performance */

  p = g_list_find (children->items, izone);

  if (p->prev && ipatch_sf2_zone_peek_link_item (p->prev->data)
      == (IpatchItem *)linked_sample)
    linked_izone = g_object_ref (p->prev->data);

  if (p->next && ipatch_sf2_zone_peek_link_item (p->next->data)
      == (IpatchItem *)linked_sample)
  {
    if (!linked_izone)
    {
      linked_izone = g_object_ref (p->next->data);
      goto ret;
    }

    /* prev is also a match, this can happen in instruments with multiple pairs
     * of the same stereo sample - Return zone with intersecting note/velocity
     * ranges or fall through to exhaustive search. */

    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)izone,
                                    IPATCH_SF2_GEN_NOTE_RANGE, &z_noterange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)izone,
                                    IPATCH_SF2_GEN_VELOCITY_RANGE, &z_velrange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)linked_izone,
                                    IPATCH_SF2_GEN_NOTE_RANGE, &cmp_noterange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)linked_izone,
                                    IPATCH_SF2_GEN_VELOCITY_RANGE, &cmp_velrange);
    if (ipatch_sf2_gen_range_intersect_test (&z_noterange, &cmp_noterange)
        && ipatch_sf2_gen_range_intersect_test (&z_velrange, &cmp_velrange))
      goto ret;

    g_object_unref (linked_izone);      /* -- unref linked izone */
    linked_izone = NULL;

    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)(p->next->data),
                                    IPATCH_SF2_GEN_NOTE_RANGE, &cmp_noterange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)(p->next->data),
                                    IPATCH_SF2_GEN_VELOCITY_RANGE, &cmp_velrange);
    if (ipatch_sf2_gen_range_intersect_test (&z_noterange, &cmp_noterange)
        && ipatch_sf2_gen_range_intersect_test (&z_velrange, &cmp_velrange))
    {
      linked_izone = g_object_ref (p->next->data);
      goto ret;
    }
  }
  else
  {
    if (linked_izone) goto ret;      /* prev matched, but next did not */

    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)izone,
                                    IPATCH_SF2_GEN_NOTE_RANGE, &z_noterange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)izone,
                                    IPATCH_SF2_GEN_VELOCITY_RANGE, &z_velrange);
  }

  /* Not previous/next or both of them match, check all items. */
  for (p = children->items; p; p = p->next)
  {
    if (p->data == izone || ipatch_sf2_zone_peek_link_item (p->data)
        != (IpatchItem *)linked_sample)
      continue;

    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)(p->data),
                                    IPATCH_SF2_GEN_NOTE_RANGE, &cmp_noterange);
    ipatch_sf2_gen_item_get_amount ((IpatchSF2GenItem *)(p->data),
                                    IPATCH_SF2_GEN_VELOCITY_RANGE, &cmp_velrange);
    if (!ipatch_sf2_gen_range_intersect_test (&z_noterange, &cmp_noterange)
        || !ipatch_sf2_gen_range_intersect_test (&z_velrange, &cmp_velrange))
      continue;

    linked_izone = g_object_ref (p->data);
    break;
  }

ret:

  if (children) g_object_unref (children);              /* -- unref children */
  if (parent) g_object_unref (parent);                  /* -- unref parent */
  if (linked_sample) g_object_unref (linked_sample);    /* -- unref linked sample */
  g_object_unref (sample);                              /* -- unref sample */

  return (linked_izone);
}
