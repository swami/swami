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
/**
 * SECTION: IpatchSLIZone
 * @short_description: Spectralis instrument zone object
 * @see_also: 
 *
 * Zones are children of #IpatchSLIInst and define how
 * their referenced #IpatchSLISample is synthesized.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSLIZone.h"
#include "IpatchSF2GenItem.h"
#include "IpatchSample.h"
#include "IpatchContainer.h"
#include "IpatchTypeProp.h"
#include "IpatchRange.h"
#include "builtin_enums.h"
#include "ipatch_priv.h"

static void ipatch_sli_zone_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean ipatch_sli_zone_sample_iface_open (IpatchSampleHandle *handle,
                                                   GError **err);
static void ipatch_sli_zone_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface);
static void ipatch_sli_zone_class_init (IpatchSLIZoneClass *klass);
static inline void ipatch_sli_zone_get_root_note (IpatchSLIZone *zone,
                                                  GValue *value);
static inline void ipatch_sli_zone_get_fine_tune (IpatchSLIZone *zone,
                                                  GValue *value);
static void ipatch_sli_zone_finalize (GObject *gobject);
static void ipatch_sli_zone_get_title (IpatchSLIZone *zone, GValue *value);
static void ipatch_sli_zone_set_property (GObject *object, guint property_id,
					  const GValue *value, GParamSpec *pspec);
static void ipatch_sli_zone_get_property (GObject *object,
					  guint property_id, GValue *value,
					  GParamSpec *pspec);
static void ipatch_sli_zone_item_copy (IpatchItem *dest, IpatchItem *src,
				       IpatchItemCopyLinkFunc link_func,
				       gpointer user_data);
static void ipatch_sli_zone_item_remove_full (IpatchItem *item, gboolean full);
static void ipatch_sli_zone_real_set_sample (IpatchSLIZone *zone,
                                             IpatchSLISample *sample,
                                             gboolean sample_notify);

/* generator property IDs go before these */
enum
{
  PROP_TITLE = IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID,
  PROP_LINK_ITEM,
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

/* For quicker access without lookup */
static GParamSpec *link_item_pspec;
static GParamSpec *root_note_pspec;
static GParamSpec *fine_tune_pspec;

/* For passing data from class init to gen item interface init */
static GParamSpec **gen_item_specs = NULL;
static GParamSpec **gen_item_setspecs = NULL;

G_DEFINE_TYPE_WITH_CODE (IpatchSLIZone, ipatch_sli_zone, IPATCH_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sli_zone_sample_iface_init)
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SF2_GEN_ITEM,
                                                ipatch_sli_zone_gen_item_iface_init))

/* sample interface initialization */
static void
ipatch_sli_zone_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_sli_zone_sample_iface_open;
  sample_iface->loop_types = ipatch_sample_loop_types_standard;
}

static gboolean
ipatch_sli_zone_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchSLIZone *zone = IPATCH_SLI_ZONE (handle->sample);
  IpatchSLISample *sample;
  gboolean retval;

  sample = ipatch_sli_zone_get_sample (zone);     /* ++ ref sample */
  g_return_val_if_fail (sample != NULL, FALSE);
  retval = ipatch_sample_handle_cascade_open (handle, IPATCH_SAMPLE (sample), err);
  g_object_unref (sample);   /* -- unref sample */
  return (retval);
}

/* gen item interface initialization */
static void
ipatch_sli_zone_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface)
{
  genitem_iface->genarray_ofs = G_STRUCT_OFFSET (IpatchSLIZone, genarray);
  genitem_iface->propstype = IPATCH_SF2_GEN_PROPS_INST;

  g_return_if_fail (gen_item_specs != NULL);
  g_return_if_fail (gen_item_setspecs != NULL);

  memcpy (&genitem_iface->specs, gen_item_specs, sizeof (genitem_iface->specs));
  memcpy (&genitem_iface->setspecs, gen_item_setspecs, sizeof (genitem_iface->setspecs));
  g_free (gen_item_specs);
  g_free (gen_item_setspecs);
}

static void
ipatch_sli_zone_class_init (IpatchSLIZoneClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_sli_zone_finalize;
  obj_class->get_property = ipatch_sli_zone_get_property;

  item_class->copy = ipatch_sli_zone_item_copy;
  item_class->item_set_property = ipatch_sli_zone_set_property;
  item_class->remove_full = ipatch_sli_zone_item_remove_full;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  link_item_pspec = g_param_spec_object ("link-item", _("Link item"),
                                         _("Link item"), IPATCH_TYPE_SLI_SAMPLE,
					 G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_LINK_ITEM, link_item_pspec);

  /* properties defined by IpatchSample interface */
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE,
                                           "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT,
                                           "sample-format");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA,
                                           "sample-data");
  ipatch_sample_install_property (obj_class, PROP_LOOP_TYPE, "loop-type");
  ipatch_sample_install_property (obj_class, PROP_LOOP_START, "loop-start");
  ipatch_sample_install_property (obj_class, PROP_LOOP_END, "loop-end");
  root_note_pspec = ipatch_sample_install_property (obj_class, PROP_ROOT_NOTE,
                                                    "root-note");
  fine_tune_pspec = ipatch_sample_install_property (obj_class, PROP_FINE_TUNE,
                                                    "fine-tune");

  /* install generator properties */
  ipatch_sf2_gen_item_iface_install_properties (obj_class,
                                                IPATCH_SF2_GEN_PROPS_INST,
                                                &gen_item_specs,
                                                &gen_item_setspecs);
}

static void
ipatch_sli_zone_init (IpatchSLIZone *zone)
{
  ipatch_sf2_gen_array_init (&zone->genarray, FALSE, FALSE);
}

static void
ipatch_sli_zone_finalize (GObject *gobject)
{
  IpatchSLIZone *zone = IPATCH_SLI_ZONE (gobject);

  IPATCH_ITEM_WLOCK (zone);

  if (zone->sample)
  {
    g_object_unref (zone->sample);
    zone->sample = NULL;
  }

  IPATCH_ITEM_WUNLOCK (zone);

  if (G_OBJECT_CLASS (ipatch_sli_zone_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sli_zone_parent_class)->finalize (gobject);
}

static inline void
ipatch_sli_zone_get_root_note (IpatchSLIZone *zone, GValue *value)
{
  IpatchSF2GenAmount amount;
  IpatchSLISample *sample;
  int val = 0;

  /* root note override not set or -1? - Get sample root note value. */
  if (!ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
                                       IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amount)
      || amount.sword == -1)
  {		/* root note override not set, get from sample */
    sample = ipatch_sli_zone_get_sample (zone); /* ++ ref sample */
    if (sample)
    {
      g_object_get (sample, "root-note", &val, NULL);
      g_object_unref (sample); /* -- unref sample */
    }
  }
  else val = amount.uword;

  g_value_set_int (value, val);
}

static inline void
ipatch_sli_zone_get_fine_tune (IpatchSLIZone *zone, GValue *value)
{
  IpatchSF2GenAmount amount;
  IpatchSLISample *sample;
  int val = 0;

  /* fine tune override set? */
  if (!ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
                                       IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amount))
  {		/* fine tune override not set, get from sample */
    sample = ipatch_sli_zone_get_sample (zone); /* ++ ref sample */
    if (sample)
    {
      g_object_get (sample, "fine-tune", &val, NULL);
      g_object_unref (sample); /* -- unref sample */
    }
  }
  else val = amount.sword;

  g_value_set_int (value, val);
}

static void
ipatch_sli_zone_get_title (IpatchSLIZone *zone, GValue *value)
{
  IpatchSLISample *sample;
  char *s = NULL;

  sample = ipatch_sli_zone_get_sample (zone); /* ++ ref sample */
  if (sample)
  {
    g_object_get (sample, "name", &s, NULL);
    g_object_unref (sample); /* -- unref sample */
  }

  g_value_take_string (value, s);
}

static void
ipatch_sli_zone_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  IpatchSLIZone *zone = IPATCH_SLI_ZONE (object);
  IpatchSLISample *sample;
  IpatchSF2GenAmount amount;
  GValue vals[2];	/* Gets zeroed below */
  guint genid;
  guint uval;
  int val = 0;

  /* "root-note" and "fine-tune" sample properties get updated for Zone
   * override property or -set property */
  if (property_id >= IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID)
    genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_SET_ID;
  else genid = property_id - IPATCH_SF2_GEN_ITEM_FIRST_PROP_ID;

  if (genid == IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE)
  {
    memset (vals, 0, sizeof (vals));
    g_value_init (&vals[0], G_TYPE_INT);
    ipatch_sli_zone_get_root_note (zone, &vals[0]);
  }
  else if (genid == IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE)
  {
    memset (vals, 0, sizeof (vals));
    g_value_init (&vals[0], G_TYPE_INT);
    ipatch_sli_zone_get_fine_tune (zone, &vals[0]);
  }

  if (ipatch_sf2_gen_item_iface_set_property ((IpatchSF2GenItem *)object,
					      property_id, value))
    {
      if (genid == IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE)
      {
        g_value_init (&vals[1], G_TYPE_INT);
	ipatch_sli_zone_get_root_note (zone, &vals[1]);
	ipatch_item_prop_notify ((IpatchItem *)object, root_note_pspec,
	                         &vals[1], &vals[0]);
      }
      else if (genid == IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE)
      {
        g_value_init (&vals[1], G_TYPE_INT);
        ipatch_sli_zone_get_fine_tune (zone, &vals[1]);
	ipatch_item_prop_notify ((IpatchItem *)object, fine_tune_pspec,
	                         &vals[1], &vals[0]);
      }
    }
  else
    {
      switch (property_id)
	{
	case PROP_LINK_ITEM:
	  ipatch_sli_zone_real_set_sample (zone, IPATCH_SLI_SAMPLE
                                           (g_value_get_object (value)), FALSE);
	  break;
	case PROP_SAMPLE_RATE:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
          {
            g_object_set (sample, "sample-rate", g_value_get_int (value), NULL);
            g_object_unref (sample); /* -- unref sample */
          }
	  break;
	case PROP_LOOP_TYPE:
	  val = g_value_get_enum (value);

	  if (val == IPATCH_SAMPLE_LOOP_NONE)
	    amount.uword = IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP;
	  else amount.uword = IPATCH_SF2_GEN_SAMPLE_MODE_LOOP;

	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
					  IPATCH_SF2_GEN_SAMPLE_MODES, &amount);
	  break;
	case PROP_LOOP_START:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
          {
            g_object_get (sample, "loop-start", &uval, NULL);
            val = g_value_get_uint (value) - uval; /* loop start offset */
            g_object_unref (sample); /* -- unref sample */

            if (val >= 0) amount.sword = val >> 15;
            else amount.sword = -(-val >> 15);

            ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                              IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START, &amount);

            if (val >= 0) amount.sword = val & 0x7FFF;
            else amount.sword = -(-val & 0x7FFF);

            ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                                     IPATCH_SF2_GEN_SAMPLE_LOOP_START, &amount);
          }
	  break;
	case PROP_LOOP_END:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
          {
            g_object_get (sample, "loop-end", &uval, NULL);
            val = g_value_get_uint (value) - uval; /* loop end offset */
            g_object_unref (sample); /* -- unref sample */

            if (val >= 0) amount.sword = val >> 15;
            else amount.sword = -(-val >> 15);

            ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                                IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END, &amount);

            if (val >= 0) amount.sword = val & 0x7FFF;
            else amount.sword = -(-val & 0x7FFF);

            ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                                       IPATCH_SF2_GEN_SAMPLE_LOOP_END, &amount);
          }
	  break;
	case PROP_ROOT_NOTE:
	  amount.uword = g_value_get_int (value);
	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                                    IPATCH_SF2_GEN_ROOT_NOTE_OVERRIDE, &amount);
	  break;
	case PROP_FINE_TUNE:
	  amount.sword = g_value_get_int (value);
	  ipatch_sf2_gen_item_set_amount (IPATCH_SF2_GEN_ITEM (zone),
                                    IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, &amount);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
    }
}

static void
ipatch_sli_zone_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  IpatchSLIZone *zone = IPATCH_SLI_ZONE (object);
  IpatchSLISample *sample;
  IpatchSF2GenAmount amount;
  guint uval = 0;
  int val = 0;

  if (!ipatch_sf2_gen_item_iface_get_property ((IpatchSF2GenItem *)object,
					       property_id, value))
    {
      switch (property_id)
	{
        case PROP_TITLE:
          ipatch_sli_zone_get_title (zone, value);
          break;
	case PROP_LINK_ITEM:
	  g_value_take_object (value, ipatch_sli_zone_get_sample (zone));
	  break;
	case PROP_SAMPLE_SIZE:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-size", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_FORMAT:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-format", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_RATE:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-rate", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_SAMPLE_DATA:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get_property ((GObject *)sample, "sample-data", value);
	      g_object_unref (sample); /* -- unref sample */
	    }
	  break;
	case PROP_LOOP_TYPE:
	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
					  IPATCH_SF2_GEN_SAMPLE_MODES, &amount);

	  if (amount.uword == IPATCH_SF2_GEN_SAMPLE_MODE_NOLOOP)
	    val = IPATCH_SAMPLE_LOOP_NONE;
	  else val = IPATCH_SAMPLE_LOOP_STANDARD;

	  g_value_set_enum (value, val);
	  break;
	case PROP_LOOP_START:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-start", &uval, NULL);
	      g_object_unref (sample); /* -- unref sample */
	      val = uval;
	    }

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
				   IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START,
				   &amount);
	  val += (int)amount.sword << 15;

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
				   IPATCH_SF2_GEN_SAMPLE_LOOP_START, &amount);
	  val += amount.sword;

	  g_value_set_uint (value, CLAMP (val, 0, G_MAXINT));
	  break;
	case PROP_LOOP_END:
	  sample = ipatch_sli_zone_get_sample (zone);	/* ++ ref sample */
	  if (sample)
	    {
	      g_object_get (sample, "loop-end", &uval, NULL);
	      g_object_unref (sample); /* -- unref sample */
	      val = uval;
	    }

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
				   IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END,
				   &amount);
	  val += (int)amount.sword << 15;

	  ipatch_sf2_gen_item_get_amount (IPATCH_SF2_GEN_ITEM (zone),
				   IPATCH_SF2_GEN_SAMPLE_LOOP_END, &amount);
	  val += amount.sword;

	  g_value_set_uint (value, CLAMP (val, 0, G_MAXINT));
	  break;
	case PROP_ROOT_NOTE:
	  ipatch_sli_zone_get_root_note (zone, value);
	  break;
	case PROP_FINE_TUNE:
	  ipatch_sli_zone_get_fine_tune (zone, value);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
    }
}

static void
ipatch_sli_zone_item_copy (IpatchItem *dest, IpatchItem *src,
			   IpatchItemCopyLinkFunc link_func,
			   gpointer user_data)
{
  IpatchSLIZone *src_zone, *dest_zone;
  IpatchSLISample *refsample;

  src_zone = IPATCH_SLI_ZONE (src);
  dest_zone = IPATCH_SLI_ZONE (dest);

  IPATCH_ITEM_RLOCK (src_zone);

  /* pass the link to the link handler (if any) */
  refsample = (IpatchSLISample *)
    IPATCH_ITEM_COPY_LINK_FUNC (dest,
				IPATCH_ITEM (src_zone->sample),
				link_func, user_data);
  if (refsample) ipatch_sli_zone_set_sample (dest_zone, refsample);

  /* duplicate generator array */
  memcpy (&dest_zone->genarray, &src_zone->genarray,
	  sizeof (IpatchSF2GenArray));

  IPATCH_ITEM_RUNLOCK (src_zone);
}

static void
ipatch_sli_zone_item_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_sli_zone_set_sample (IPATCH_SLI_ZONE (item), NULL);

  if (IPATCH_ITEM_CLASS (ipatch_sli_zone_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_sli_zone_parent_class)->remove_full (item, full);
}

/**
 * ipatch_sli_zone_new:
 *
 * Create a new Spectralis instrument zone object.
 *
 * Returns: New Spectralis instrument zone with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSLIZone *
ipatch_sli_zone_new (void)
{
  return (IPATCH_SLI_ZONE (g_object_new (IPATCH_TYPE_SLI_ZONE, NULL)));
}

/**
 * ipatch_sli_zone_first: (skip)
 * @iter: Patch item iterator containing #IpatchSLIZone items
 *
 * Gets the first item in a zone iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first zone in @iter or %NULL if empty.
 */
IpatchSLIZone *
ipatch_sli_zone_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SLI_ZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sli_zone_next: (skip)
 * @iter: Patch item iterator containing #IpatchSLIZone items
 *
 * Gets the next item in a zone iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next zone in @iter or %NULL if at the end of
 *   the list.
 */
IpatchSLIZone *
ipatch_sli_zone_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SLI_ZONE (obj));
  else return (NULL);
}

/**
 * ipatch_sli_zone_set_sample:
 * @zone: Zone to set sample of
 * @sample: Sample to set zone to
 *
 * Sets the referenced sample of a zone.
 */
void
ipatch_sli_zone_set_sample (IpatchSLIZone *zone, IpatchSLISample *sample)
{
  g_return_if_fail (IPATCH_IS_SLI_ZONE (zone));
  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (sample));

  ipatch_sli_zone_real_set_sample(zone, sample, TRUE);
}

static void
ipatch_sli_zone_real_set_sample (IpatchSLIZone *zone, IpatchSLISample *sample,
                                 gboolean sample_notify)
{
  GValue oldval = { 0 }, newval = { 0 };
  IpatchSLISample *oldsample;

  if (sample_notify)
    ipatch_item_get_property_fast ((IpatchItem *)zone, link_item_pspec,
				   &oldval);

  if (sample) g_object_ref (sample); /* ref for zone */

  IPATCH_ITEM_WLOCK (zone);
  oldsample = zone->sample;
  zone->sample = sample;
  IPATCH_ITEM_WUNLOCK (zone);

  if (oldsample) g_object_unref (oldsample);

  if (sample_notify)
  {
    g_value_init (&newval, IPATCH_TYPE_SLI_SAMPLE);
    g_value_set_object (&newval, sample);
    ipatch_item_prop_notify ((IpatchItem *)zone, link_item_pspec,
                             &newval, &oldval);
    g_value_unset (&oldval);
    g_value_unset (&newval);
  }

  /* notify title property change */
  g_value_init (&newval, G_TYPE_STRING);
  ipatch_sli_zone_get_title (zone, &newval);
  ipatch_item_prop_notify ((IpatchItem *)zone, ipatch_item_pspec_title,
			   &newval, NULL);
  g_value_unset (&newval);
}

/**
 * ipatch_sli_zone_get_sample:
 * @zone: Zone to get referenced sample from
 *
 * Gets the referenced sample from a zone.
 * The returned item's reference count is incremented and the caller
 * is responsible for unrefing it with g_object_unref() when
 * done with it.
 *
 * Returns: (transfer full): Zone's referenced sample or %NULL if global zone. Remember to
 * unreference the item with g_object_unref() when done with it.
 */
IpatchSLISample *
ipatch_sli_zone_get_sample (IpatchSLIZone *zone)
{
  IpatchSLISample *sample;

  g_return_val_if_fail (IPATCH_IS_SLI_ZONE (zone), NULL);

  IPATCH_ITEM_RLOCK (zone);
  sample = zone->sample;
  if (sample) g_object_ref (sample);
  IPATCH_ITEM_RUNLOCK (zone);

  return sample;
}

/**
 * ipatch_sli_zone_peek_sample: (skip)
 * @zone: Zone to get referenced item of
 *
 * Like ipatch_sli_zone_get_sample() but does not add a reference to
 * the returned sample. This function should only be used if a reference
 * of the returned sample is ensured or only the pointer value is of
 * interest.
 *
 * Returns: Zone's linked sample.
 * Remember that the item has NOT been referenced.
 */
IpatchSLISample *
ipatch_sli_zone_peek_sample (IpatchSLIZone *zone)
{
  g_return_val_if_fail (IPATCH_IS_SLI_ZONE (zone), NULL);
  return (zone->sample);	/* atomic read */
}
