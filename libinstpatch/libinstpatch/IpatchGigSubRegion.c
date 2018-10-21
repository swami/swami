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
 * SECTION: IpatchGigSubRegion
 * @short_description: GigaSampler sub region object
 * @see_also: #IpatchGigRegion
 * @stability: Stable
 *
 * Defines a GigaSampler sub region object which are children of
 * #IpatchGigRegion objects and define how a referenced #IpatchGigSample
 * is synthesized in a #IpatchGigInst.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchGigSubRegion.h"
#include "IpatchSample.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LINK_ITEM,
  PROP_SAMPLE_INFO_OVERRIDE,
  PROP_SAMPLE_SIZE,
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_SAMPLE_DATA
};

static void ipatch_gig_sub_region_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean ipatch_gig_sub_region_sample_iface_open (IpatchSampleHandle *handle,
                                                         GError **err);
static void ipatch_gig_sub_region_class_init (IpatchGigSubRegionClass *klass);
static void ipatch_gig_sub_region_get_title (IpatchGigSubRegion *gig_region,
					     GValue *value);
static void
ipatch_gig_sub_region_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec);
static void
ipatch_gig_sub_region_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec);

static void ipatch_gig_sub_region_init (IpatchGigSubRegion *gig_region);
static void ipatch_gig_sub_region_finalize (GObject *gobject);
static void ipatch_gig_sub_region_item_copy (IpatchItem *dest, IpatchItem *src,
					     IpatchItemCopyLinkFunc link_func,
					     gpointer user_data);
static void ipatch_gig_sub_region_item_remove_full (IpatchItem *item, gboolean full);
static void
ipatch_gig_sub_region_real_set_sample (IpatchGigSubRegion *subregion,
				       IpatchGigSample *sample,
				       gboolean sample_notify);
static void ipatch_gig_sub_region_get_sample_info (IpatchGigSubRegion *subregion,
						   IpatchDLS2SampleInfo *info);

static GParamSpec *link_item_pspec;

G_DEFINE_TYPE_WITH_CODE (IpatchGigSubRegion, ipatch_gig_sub_region, IPATCH_TYPE_ITEM,
    G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE, ipatch_gig_sub_region_sample_iface_init))


/* sample interface initialization */
static void
ipatch_gig_sub_region_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_gig_sub_region_sample_iface_open;
  sample_iface->loop_types = ipatch_sample_loop_types_standard_release;
}

static gboolean
ipatch_gig_sub_region_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchGigSubRegion *region = IPATCH_GIG_SUB_REGION (handle->sample);
  g_return_val_if_fail (region->sample != NULL, FALSE);
  return (ipatch_sample_handle_cascade_open (handle, IPATCH_SAMPLE (region->sample), err));
}

static void
ipatch_gig_sub_region_class_init (IpatchGigSubRegionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_gig_sub_region_finalize;
  obj_class->get_property = ipatch_gig_sub_region_get_property;

  item_class->item_set_property = ipatch_gig_sub_region_set_property;
  item_class->copy = ipatch_gig_sub_region_item_copy;
  item_class->remove_full = ipatch_gig_sub_region_item_remove_full;

  /* use parent's mutex (IpatchGigRegion) */
  item_class->mutex_slave = TRUE;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  link_item_pspec = g_param_spec_object ("link-item", _("Link item"),
					 _("Link item"),
					 IPATCH_TYPE_GIG_SAMPLE,
					 G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_LINK_ITEM, link_item_pspec);

  g_object_class_install_property (obj_class, PROP_SAMPLE_INFO_OVERRIDE,
		g_param_spec_boolean ("sample-info-override",
				      _("Sample info override"),
				      _("Override sample info"),
				      FALSE, G_PARAM_READWRITE));

  /* IpatchSample interface properties */
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA, "sample-data");

  ipatch_dls2_sample_info_install_class_properties (obj_class);
}

static void
ipatch_gig_sub_region_get_title (IpatchGigSubRegion *gig_region, GValue *value)
{
  IpatchGigSample *sample;
  char *s = NULL;

  g_object_get (gig_region, "link-item", &sample, NULL); /* ++ ref sample */

  if (sample)
    {
      g_object_get (sample, "name", &s, NULL); /* caller takes over alloc. */
      g_object_unref (sample);	/* -- unref sample */
    }

  g_value_take_string (value, s);
}

static void
ipatch_gig_sub_region_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
  IpatchGigSubRegion *subregion = IPATCH_GIG_SUB_REGION (object);
  IpatchDLS2SampleInfo saminfo = IPATCH_DLS2_SAMPLE_INFO_INIT;
  IpatchDLS2SampleInfo oldinfo, newinfo;
  gboolean retval, is_samprop;

  switch (property_id)
    {
    case PROP_LINK_ITEM:
      ipatch_gig_sub_region_real_set_sample (subregion, IPATCH_GIG_SAMPLE
					     (g_value_get_object (value)),
					     FALSE);
      break;
    case PROP_SAMPLE_INFO_OVERRIDE:
      ipatch_gig_sub_region_get_sample_info (subregion, &oldinfo);

      if (g_value_get_boolean (value))
	ipatch_item_set_flags ((IpatchItem *)subregion,
			       IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE);
      else
	ipatch_item_clear_flags ((IpatchItem *)subregion,
				 IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE);

      ipatch_gig_sub_region_get_sample_info (subregion, &newinfo);
      ipatch_dls2_sample_info_notify_changes ((IpatchItem *)subregion, &newinfo,
					      &oldinfo);
      break;
    default:
      is_samprop = ipatch_dls2_sample_info_is_property_id_valid (property_id);

      /* check if region override info valid but override flag not set.  If so
         then copy sample info to static 'saminfo'.  OK to test region without
	 locking it (worst that happens is default values get used). */
      if (is_samprop && subregion->sample_info
	  && !(ipatch_item_get_flags (subregion)
	       & IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE))
	ipatch_gig_sub_region_get_sample_info (subregion, &saminfo);

      IPATCH_ITEM_WLOCK (subregion);

      /* is override sample_info valid but override flag not set and it is
	 in fact a sample info property?  -  Copy values from sample
	 (or defaults) */
      if (is_samprop && subregion->sample_info
	  && !(ipatch_item_get_flags (subregion)
	       & IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE))
	*subregion->sample_info = saminfo;

      retval = ipatch_dls2_sample_info_set_property (&subregion->sample_info,
						     property_id, value);
      if (retval)	/* sample info set, set override flag */
	ipatch_item_set_flags (subregion,
			       IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE);
      IPATCH_ITEM_WUNLOCK (subregion);

      if (!retval)
	{
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
      break;
    }
}

static void
ipatch_gig_sub_region_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
  IpatchGigSubRegion *subregion = IPATCH_GIG_SUB_REGION (object);
  IpatchDLS2Sample *sample = NULL;
  gboolean retval = FALSE, get_from_sample = FALSE;

  switch (property_id)
    {
    case PROP_TITLE:
      ipatch_gig_sub_region_get_title (subregion, value);
      break;
    case PROP_LINK_ITEM:
      g_value_take_object (value, ipatch_gig_sub_region_get_sample (subregion));
      break;
    case PROP_SAMPLE_INFO_OVERRIDE:
      g_value_set_boolean (value, (ipatch_item_get_flags ((IpatchItem *)subregion)
			   & IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE) != 0);
      break;
    case PROP_SAMPLE_SIZE:
      /* ++ ref sample */
      sample = (IpatchDLS2Sample *)ipatch_gig_sub_region_get_sample (subregion);
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-size", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_FORMAT:
      /* ++ ref sample */
      sample = (IpatchDLS2Sample *)ipatch_gig_sub_region_get_sample (subregion);
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-format", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_RATE:
      /* ++ ref sample */
      sample = (IpatchDLS2Sample *)ipatch_gig_sub_region_get_sample (subregion);
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-rate", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_DATA:
      /* ++ ref sample */
      sample = (IpatchDLS2Sample *)ipatch_gig_sub_region_get_sample (subregion);
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-data", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    default:
      /* a sample info property? */
      if (property_id >= IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID
	  && property_id < (IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID
			    + IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT))
	{
	  IPATCH_ITEM_RLOCK (subregion);

	  /* sample override info is valid? */
	  if ((ipatch_item_get_flags (subregion)
	       & IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE)
	      && subregion->sample_info)
	    retval = ipatch_dls2_sample_info_get_property (subregion->sample_info,
							   property_id, value);
	  else
	    {
	      get_from_sample = TRUE;
	      sample = (IpatchDLS2Sample *)(subregion->sample);
	    }

	  IPATCH_ITEM_RUNLOCK (subregion);
	}

      if (get_from_sample)
	{
	  if (sample)
	    {
	      IPATCH_ITEM_RLOCK (sample);
	      ipatch_dls2_sample_info_get_property (sample->sample_info,
						    property_id, value);
	      IPATCH_ITEM_RUNLOCK (sample);
	      g_object_unref (sample);
	    }
	  else ipatch_dls2_sample_info_get_property (sample->sample_info,
						     property_id, value);
	}
      else if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_gig_sub_region_init (IpatchGigSubRegion *subregion)
{
  ipatch_gig_effects_init (&subregion->effects);
}

static void
ipatch_gig_sub_region_finalize (GObject *gobject)
{
  IpatchGigSubRegion *subregion = IPATCH_GIG_SUB_REGION (gobject);

  IPATCH_ITEM_WLOCK (subregion);

  if (subregion->sample)
    g_object_unref (subregion->sample);

  if (subregion->sample_info)
    ipatch_dls2_sample_info_free (subregion->sample_info);

  subregion->sample = NULL;
  subregion->sample_info = NULL;

  IPATCH_ITEM_WUNLOCK (subregion);

  if (G_OBJECT_CLASS (ipatch_gig_sub_region_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_gig_sub_region_parent_class)->finalize (gobject);
}

static void
ipatch_gig_sub_region_item_copy (IpatchItem *dest, IpatchItem *src,
				 IpatchItemCopyLinkFunc link_func,
				 gpointer user_data)
{
  IpatchGigSubRegion *src_sub, *dest_sub;
  IpatchGigSample *sample;

  src_sub = IPATCH_GIG_SUB_REGION (src);
  dest_sub = IPATCH_GIG_SUB_REGION (dest);

  IPATCH_ITEM_RLOCK (src_sub);

  dest_sub->effects = src_sub->effects;

  if (src_sub->sample)
    {
      sample = (IpatchGigSample *)
	IPATCH_ITEM_COPY_LINK_FUNC (IPATCH_ITEM (dest_sub),
				    IPATCH_ITEM (src_sub->sample),
				    link_func, user_data);
      if (sample)
	dest_sub->sample = g_object_ref (sample); /* ++ ref sample */
    }

  if (src_sub->sample_info)
    dest_sub->sample_info
      = ipatch_dls2_sample_info_duplicate (src_sub->sample_info);

  IPATCH_ITEM_RUNLOCK (src_sub);
}

static void
ipatch_gig_sub_region_item_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_gig_sub_region_real_set_sample (IPATCH_GIG_SUB_REGION (item), NULL, TRUE);

  if (IPATCH_ITEM_CLASS (ipatch_gig_sub_region_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_gig_sub_region_parent_class)->remove_full (item, full);
}

/**
 * ipatch_gig_sub_region_new:
 *
 * Create a new GigaSampler sub region.
 *
 * Returns: New GigaSampler sub region with a ref count of 1 which the caller
 * owns.
 */
IpatchGigSubRegion *
ipatch_gig_sub_region_new (void)
{
  return (IPATCH_GIG_SUB_REGION (g_object_new (IPATCH_TYPE_GIG_SUB_REGION,
					       NULL)));
}

/**
 * ipatch_gig_sub_region_first: (skip)
 * @iter: Patch item iterator containing #IpatchGigSubRegion items
 *
 * Gets the first item in a sub region iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first sub region in @iter or %NULL if empty.
 */
IpatchGigSubRegion *
ipatch_gig_sub_region_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_GIG_SUB_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_gig_sub_region_next: (skip)
 * @iter: Patch item iterator containing #IpatchGigSubRegion items
 *
 * Gets the next item in a sub region iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next sub region in @iter or %NULL if at the end of
 *   the list.
 */
IpatchGigSubRegion *
ipatch_gig_sub_region_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_GIG_SUB_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_gig_sub_region_get_sample:
 * @subregion: GigaSampler sub region to get sample of
 *
 * Returns: (transfer full): The sub region's sample, referenced for the caller, unref
 *   it when finished.
 */
IpatchGigSample *
ipatch_gig_sub_region_get_sample (IpatchGigSubRegion *subregion)
{
  IpatchGigSample *sample;

  g_return_val_if_fail (IPATCH_IS_GIG_SUB_REGION (subregion), NULL);

  IPATCH_ITEM_RLOCK (subregion);
  sample = g_object_ref (subregion->sample);
  IPATCH_ITEM_WUNLOCK (subregion);

  return (sample);
}

/**
 * ipatch_gig_sub_region_set_sample:
 * @subregion: GigaSampler sub region to set sample of
 * @sample: Sample to assign to the sub region
 *
 * Set the referenced sample of a sub region.
 */
void
ipatch_gig_sub_region_set_sample (IpatchGigSubRegion *subregion,
				  IpatchGigSample *sample)
{
  g_return_if_fail (IPATCH_IS_GIG_SUB_REGION (subregion));
  g_return_if_fail (IPATCH_IS_GIG_SAMPLE (sample));

  ipatch_gig_sub_region_real_set_sample (subregion, sample, TRUE);
}

static void
ipatch_gig_sub_region_real_set_sample (IpatchGigSubRegion *subregion,
				       IpatchGigSample *sample,
				       gboolean sample_notify)
{
  GValue newval = { 0 }, oldval = { 0 };
  IpatchDLS2SampleInfo oldinfo, newinfo;
  IpatchGigSample *oldsample;

  /* get all values of current sample info */
  ipatch_gig_sub_region_get_sample_info (subregion, &oldinfo);

  if (sample) g_object_ref (sample);

  IPATCH_ITEM_WLOCK (subregion);
  oldsample = subregion->sample;
  subregion->sample = sample;
  IPATCH_ITEM_WUNLOCK (subregion);

  if (sample_notify)
    {
      g_value_init (&oldval, IPATCH_TYPE_GIG_SAMPLE);
      g_value_take_object (&oldval, oldsample);

      g_value_init (&newval, IPATCH_TYPE_GIG_SAMPLE);
      g_value_set_object (&newval, sample);
      ipatch_item_prop_notify ((IpatchItem *)subregion, link_item_pspec,
			       &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }
  else if (oldsample) g_object_unref (oldsample); /* -- unref old sample */

  /* do the title notify */
  g_value_init (&newval, G_TYPE_STRING);
  ipatch_gig_sub_region_get_title (subregion, &newval);
  ipatch_item_prop_notify ((IpatchItem *)subregion, ipatch_item_pspec_title,
			   &newval, NULL);
  g_value_unset (&newval);

  /* notify for sample info properties */
  ipatch_gig_sub_region_get_sample_info (subregion, &newinfo);

  ipatch_dls2_sample_info_notify_changes ((IpatchItem *)subregion, &newinfo,
					  &oldinfo);
}

/* fetch active sample info from a Gig sub region */
static void
ipatch_gig_sub_region_get_sample_info (IpatchGigSubRegion *subregion,
				       IpatchDLS2SampleInfo *info)
{
  IpatchDLS2Sample *sample = NULL;
  gboolean info_set = TRUE;

  IPATCH_ITEM_RLOCK (subregion);

  if (ipatch_item_get_flags (subregion) & IPATCH_GIG_SUB_REGION_SAMPLE_INFO_OVERRIDE
      && subregion->sample_info)
    *info = *subregion->sample_info;
  else if (subregion->sample) sample = (IpatchDLS2Sample *)g_object_ref (subregion->sample);
  else info_set = FALSE;

  IPATCH_ITEM_RUNLOCK (subregion);

  if (sample)
    {
      IPATCH_ITEM_RLOCK (sample);

      if (sample->sample_info)
	*info = *sample->sample_info;
      else info_set = FALSE;

      IPATCH_ITEM_RUNLOCK (sample);

      g_object_unref (sample);
    }

  if (!info_set) ipatch_dls2_sample_info_init (info);
}
