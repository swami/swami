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
 * SECTION: IpatchDLS2Region
 * @short_description: DLS region object
 * @see_also: #IpatchDLSInst
 * @stability: Stable
 *
 * DLS regions are child items of #IpatchDLSInst objects and define how an
 * individual audio sample is synthesized in an instrument.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2Region.h"
#include "IpatchGigRegion.h"
#include "IpatchRange.h"
#include "IpatchSample.h"
#include "IpatchTypeProp.h"
#include "builtin_enums.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,

  PROP_TITLE,

  PROP_NOTE_RANGE,
  PROP_VELOCITY_RANGE,

  PROP_KEY_GROUP,
  PROP_LAYER_GROUP,
  PROP_PHASE_GROUP,
  PROP_CHANNEL,
  PROP_LINK_ITEM,
  PROP_SAMPLE_INFO_OVERRIDE,		/* sample info override boolean */

  /* IpatchItem flags (no one needs to know that though) */
  PROP_SELF_NON_EXCLUSIVE,
  PROP_PHASE_MASTER,
  PROP_MULTI_CHANNEL,

  /* IpatchSample interface properties */
  PROP_SAMPLE_SIZE,
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_SAMPLE_DATA
};

enum
{
  SET_CONN,
  UNSET_CONN,
  LAST_SIGNAL
};

static void ipatch_dls2_region_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean ipatch_dls2_region_sample_iface_open (IpatchSampleHandle *handle,
                                                      GError **err);
static void ipatch_dls2_region_class_init (IpatchDLS2RegionClass *klass);
static void ipatch_dls2_region_init (IpatchDLS2Region *region);
static void ipatch_dls2_region_finalize (GObject *gobject);
static void ipatch_dls2_region_get_title (IpatchDLS2Region *region,
					  GValue *value);
static void ipatch_dls2_region_set_property (GObject *object,
					     guint property_id,
					     const GValue *value,
					     GParamSpec *pspec);
static void ipatch_dls2_region_get_property (GObject *object,
					     guint property_id, GValue *value,
					     GParamSpec *pspec);
static void ipatch_dls2_region_item_copy (IpatchItem *dest, IpatchItem *src,
					  IpatchItemCopyLinkFunc link_func,
					  gpointer user_data);
static void ipatch_dls2_region_item_remove_full (IpatchItem *item, gboolean full);
static void ipatch_dls2_region_real_set_sample (IpatchDLS2Region *region,
						IpatchDLS2Sample *sample,
						gboolean sample_notify);
static void ipatch_dls2_region_get_sample_info (IpatchDLS2Region *region,
						IpatchDLS2SampleInfo *info);

/* cached param specs to speed up prop notifies */
static GParamSpec *link_item_pspec;

G_DEFINE_TYPE_WITH_CODE (IpatchDLS2Region, ipatch_dls2_region, IPATCH_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_dls2_region_sample_iface_init))

/* sample interface initialization */
static void
ipatch_dls2_region_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_dls2_region_sample_iface_open;
  sample_iface->loop_types = ipatch_sample_loop_types_standard_release;
}

static gboolean
ipatch_dls2_region_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchDLS2Region *region = IPATCH_DLS2_REGION (handle->sample);
  g_return_val_if_fail (region->sample != NULL, FALSE);
  return (ipatch_sample_handle_cascade_open (handle, IPATCH_SAMPLE (region->sample), err));
}

static void
ipatch_dls2_region_class_init (IpatchDLS2RegionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_dls2_region_finalize;
  obj_class->get_property = ipatch_dls2_region_get_property;

  item_class->item_set_property = ipatch_dls2_region_set_property;
  item_class->copy = ipatch_dls2_region_item_copy;
  item_class->remove_full = ipatch_dls2_region_item_remove_full;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  g_object_class_install_property (obj_class, PROP_NOTE_RANGE,
	ipatch_param_spec_range ("note-range", _("Note range"),
				 _("MIDI note range"),
				 0, 127, 0, 127,
				 G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_VELOCITY_RANGE,
	ipatch_param_spec_range ("velocity-range", _("Velocity range"),
				 _("MIDI velocity range"),
				 0, 127, 0, 127,
				 G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, PROP_KEY_GROUP,
			g_param_spec_int ("key-group", _("Key group"),
					  _("Percussion key group"),
					  0, 15, 0,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LAYER_GROUP,
			g_param_spec_int ("layer-group", _("Layer group"),
					  _("Layer group"),
					  0, G_MAXUSHORT, 0,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PHASE_GROUP,
			g_param_spec_int ("phase-group", _("Phase group"),
					  _("Phase locked sample group"),
					  0, G_MAXUSHORT, 0,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_CHANNEL,
			g_param_spec_int ("channel", _("Channel"),
					  _("DLS audio channel identifier"),
					  0, 0x03FFFF, 0,
					  G_PARAM_READWRITE));

  link_item_pspec =
    g_param_spec_object ("link-item", _("Link item"), _("Link item"),
			 IPATCH_TYPE_DLS2_SAMPLE, G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_LINK_ITEM, link_item_pspec);

  g_object_class_install_property (obj_class, PROP_SAMPLE_INFO_OVERRIDE,
		g_param_spec_boolean ("sample-info-override",
				      _("Override sample info"),
				      _("Override sample info"),
				      FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, PROP_SELF_NON_EXCLUSIVE,
			g_param_spec_boolean ("self-non-exclusive",
					      _("Non exclusive"),
					      _("Self non exclusive"),
					      FALSE,
					      G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_PHASE_MASTER,
		g_param_spec_boolean ("phase-master",
				      _("Phase master"),
				      _("Multi channel phase lock master"),
				      FALSE,
				      G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MULTI_CHANNEL,
			g_param_spec_boolean ("multi-channel",
					      _("Multi channel"),
					      _("Multi channel"),
					      FALSE,
					      G_PARAM_READWRITE));

  /* IpatchSample interface properties */
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_DATA, "sample-data");

  ipatch_dls2_info_install_class_properties (obj_class);
  ipatch_dls2_sample_info_install_class_properties (obj_class);
}

static void
ipatch_dls2_region_init (IpatchDLS2Region *region)
{
  region->note_range_low = 0;
  region->note_range_high = 127;
  region->velocity_range_low = 0;
  region->velocity_range_high = 127;
  region->key_group = 0;
  region->layer_group = 0;
  region->phase_group = 0;
  region->channel = 0;
  region->info = NULL;
  region->sample_info = NULL;
  region->sample = NULL;
  region->conns = NULL;
}

static void
ipatch_dls2_region_finalize (GObject *gobject)
{
  IpatchDLS2Region *region = IPATCH_DLS2_REGION (gobject);
  GSList *p;

  IPATCH_ITEM_WLOCK (region);

  if (region->sample_info)
    {
      ipatch_dls2_sample_info_free (region->sample_info);
      region->sample_info = NULL;
    }

  if (region->sample)
    {
      g_object_unref (region->sample);
      region->sample = NULL;
    }

  p = region->conns;
  while (p)
    {
      ipatch_dls2_conn_free ((IpatchDLS2Conn *)(p->data));
      p = g_slist_delete_link (p, p);
    }
  region->conns = NULL;

  IPATCH_ITEM_WUNLOCK (region);

  if (G_OBJECT_CLASS (ipatch_dls2_region_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_dls2_region_parent_class)->finalize (gobject);
}

static void
ipatch_dls2_region_get_title (IpatchDLS2Region *region, GValue *value)
{
  IpatchDLS2Sample *sample;
  char *s = NULL;

  sample = ipatch_dls2_region_get_sample (region);  /* ++ ref sample */
  if (sample)
    {
      g_object_get (sample, "name", &s, NULL);
      g_object_unref (sample);	/* -- unref sample */
    }

  g_value_take_string (value, s);
}

static void
ipatch_dls2_region_set_property (GObject *object, guint property_id,
				 const GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Region *region = IPATCH_DLS2_REGION (object);
  IpatchDLS2SampleInfo saminfo = IPATCH_DLS2_SAMPLE_INFO_INIT;
  IpatchDLS2SampleInfo oldinfo, newinfo;
  IpatchRange *range;
  gboolean is_samprop;
  gboolean retval;
  
  switch (property_id)
    {
    case PROP_NOTE_RANGE:
      range = ipatch_value_get_range (value);
      if (range)
	{
	  IPATCH_ITEM_WLOCK (region);
	  region->note_range_low = range->low;
	  region->note_range_high = range->high;
	  IPATCH_ITEM_WUNLOCK (region);
	}
      break;
    case PROP_VELOCITY_RANGE:
      range = ipatch_value_get_range (value);
      if (range)
	{
	  IPATCH_ITEM_WLOCK (region);
	  region->velocity_range_low = range->low;
	  region->velocity_range_high = range->high;
	  IPATCH_ITEM_WUNLOCK (region);
	}
      break;
    case PROP_KEY_GROUP:
      region->key_group = g_value_get_int (value);
      break;
    case PROP_LAYER_GROUP:
      region->layer_group = g_value_get_int (value);
      break;
    case PROP_PHASE_GROUP:
      region->phase_group = g_value_get_int (value);
      break;
    case PROP_CHANNEL:
      region->channel = g_value_get_int (value);
      break;
    case PROP_LINK_ITEM:
      ipatch_dls2_region_real_set_sample (region, IPATCH_DLS2_SAMPLE
					  (g_value_get_object (value)), FALSE);
      break;
    case PROP_SAMPLE_INFO_OVERRIDE:
      ipatch_dls2_region_get_sample_info (region, &oldinfo);

      if (g_value_get_boolean (value))
	ipatch_item_set_flags ((IpatchItem *)region,
			       IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE);
      else
	ipatch_item_clear_flags ((IpatchItem *)region,
				 IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE);

      ipatch_dls2_region_get_sample_info (region, &newinfo);
      ipatch_dls2_sample_info_notify_changes ((IpatchItem *)region, &newinfo,
					      &oldinfo);
      break;
    case PROP_SELF_NON_EXCLUSIVE:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE);
      break;
    case PROP_PHASE_MASTER:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_DLS2_REGION_PHASE_MASTER);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_DLS2_REGION_PHASE_MASTER);
      break;
    case PROP_MULTI_CHANNEL:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_DLS2_REGION_MULTI_CHANNEL);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_DLS2_REGION_MULTI_CHANNEL);
      break;
    default:
      is_samprop = ipatch_dls2_sample_info_is_property_id_valid (property_id);

      /* check if region override info valid but override flag not set.  If so
         then copy sample info to static 'saminfo'.  OK to test region without
	 locking it (worst that happens is default values get used). */
      if (is_samprop && region->sample_info
	  && !(ipatch_item_get_flags (region) & IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE))
	ipatch_dls2_region_get_sample_info (region, &saminfo);

      IPATCH_ITEM_WLOCK (region);

      /* is override sample_info valid but override flag not set and it is
	 in fact a sample info property?  -  Copy values from sample
	 (or defaults) */
      if (is_samprop && region->sample_info
	  && !(ipatch_item_get_flags (region) & IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE))
	*region->sample_info = saminfo;

      retval = ipatch_dls2_sample_info_set_property (&region->sample_info,
						     property_id, value);
      if (retval)	/* sample info set, set override flag */
	ipatch_item_set_flags (region, IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE);
      else retval = ipatch_dls2_info_set_property (&region->info,
						   property_id, value);
      IPATCH_ITEM_WUNLOCK (region);

      if (!retval)
	{
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
      break;
    }
}

static void
ipatch_dls2_region_get_property (GObject *object, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Region *region = IPATCH_DLS2_REGION (object);
  IpatchDLS2Sample *sample = NULL;
  IpatchRange range;
  gboolean bool, retval = 0;
  gboolean get_from_sample = FALSE;

  switch (property_id)
    {
    case PROP_TITLE:
      ipatch_dls2_region_get_title (region, value);
      break;
    case PROP_NOTE_RANGE:
      IPATCH_ITEM_RLOCK (region);
      range.low = region->note_range_low;
      range.high = region->note_range_high;
      IPATCH_ITEM_RUNLOCK (region);

      ipatch_value_set_range (value, &range);
      break;
    case PROP_VELOCITY_RANGE:
      IPATCH_ITEM_RLOCK (region);
      range.low = region->velocity_range_low;
      range.high = region->velocity_range_high;
      IPATCH_ITEM_RUNLOCK (region);

      ipatch_value_set_range (value, &range);
      break;
    case PROP_KEY_GROUP:
      g_value_set_int (value, region->key_group);
      break;
    case PROP_LAYER_GROUP:
      g_value_set_int (value, region->layer_group);
      break;
    case PROP_PHASE_GROUP:
      g_value_set_int (value, region->phase_group);
      break;
    case PROP_CHANNEL:
      g_value_set_int (value, region->channel);
      break;
    case PROP_LINK_ITEM:
      g_value_take_object (value, ipatch_dls2_region_get_sample (region));
      break;
    case PROP_SAMPLE_INFO_OVERRIDE:
      g_value_set_boolean (value, (ipatch_item_get_flags ((IpatchItem *)region)
				   & IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE) != 0);
      break;
    case PROP_SELF_NON_EXCLUSIVE:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	& IPATCH_DLS2_REGION_SELF_NON_EXCLUSIVE) > 0;
      g_value_set_boolean (value, bool);
      break;
    case PROP_PHASE_MASTER:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	      & IPATCH_DLS2_REGION_PHASE_MASTER) > 0;
      g_value_set_boolean (value, bool);
      break;
    case PROP_MULTI_CHANNEL:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	      & IPATCH_DLS2_REGION_MULTI_CHANNEL) > 0;
      g_value_set_boolean (value, bool);
      break;
    case PROP_SAMPLE_SIZE:
      sample = ipatch_dls2_region_get_sample (region); /* ++ ref sample */
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-size", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_FORMAT:
      sample = ipatch_dls2_region_get_sample (region); /* ++ ref sample */
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-size", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_RATE:
      sample = ipatch_dls2_region_get_sample (region); /* ++ ref sample */
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-rate", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    case PROP_SAMPLE_DATA:
      sample = ipatch_dls2_region_get_sample (region); /* ++ ref sample */
      g_return_if_fail (sample != NULL);
      g_object_get_property ((GObject *)sample, "sample-data", value);
      g_object_unref (sample);	/* -- unref sample */
      break;
    default:
      IPATCH_ITEM_RLOCK (region);

      /* a sample info property? */
      if (property_id >= IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID
	  && property_id < (IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID
			    + IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT))
	{
	  if (ipatch_item_get_flags (region) & IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE
	      && region->sample_info)
	    retval = ipatch_dls2_sample_info_get_property (region->sample_info,
							   property_id, value);
	  else
	    {
	      get_from_sample = TRUE;
	      sample = region->sample ? g_object_ref (region->sample) : NULL;
	    }
	}  /* not sample info, is it a DLS text info property? */
      else retval = ipatch_dls2_info_get_property (region->info,
						   property_id, value);
      IPATCH_ITEM_RUNLOCK (region);

      if (get_from_sample)	/* get sample info from linked sample? */
	{
	  if (sample)
	    {
	      IPATCH_ITEM_RLOCK (sample);
	      ipatch_dls2_sample_info_get_property (sample->sample_info,
						    property_id, value);
	      IPATCH_ITEM_RUNLOCK (sample);
	      g_object_unref (sample);
	    }
	  else ipatch_dls2_sample_info_get_property (NULL, property_id, value);
	}
      else if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);

      break;
    }
}

static void
ipatch_dls2_region_item_copy (IpatchItem *dest, IpatchItem *src,
			      IpatchItemCopyLinkFunc link_func,
			      gpointer user_data)
{
  IpatchDLS2Region *src_reg, *dest_reg;
  IpatchDLS2Sample *refsample;

  src_reg = IPATCH_DLS2_REGION (src);
  dest_reg = IPATCH_DLS2_REGION (dest);

  IPATCH_ITEM_RLOCK (src_reg);

  /* duplicate the flags */
  ipatch_item_set_flags (dest_reg, ipatch_item_get_flags (src_reg)
			 & IPATCH_DLS2_REGION_FLAG_MASK);

  dest_reg->note_range_low = src_reg->note_range_low;
  dest_reg->note_range_high = src_reg->note_range_high;
  dest_reg->velocity_range_low = src_reg->velocity_range_low;
  dest_reg->velocity_range_high = src_reg->velocity_range_high;
  dest_reg->key_group = src_reg->key_group;
  dest_reg->layer_group = src_reg->layer_group;
  dest_reg->phase_group = src_reg->phase_group;
  dest_reg->channel = src_reg->channel;

  dest_reg->info = ipatch_dls2_info_duplicate (src_reg->info);

  dest_reg->sample_info = src_reg->sample_info
    ? ipatch_dls2_sample_info_duplicate (src_reg->sample_info) : NULL;

  /* pass the link to the link handler (if any) */
  refsample = (IpatchDLS2Sample *)
    IPATCH_ITEM_COPY_LINK_FUNC (IPATCH_ITEM (dest_reg),
				IPATCH_ITEM (src_reg->sample),
				link_func, user_data);
  if (refsample) ipatch_dls2_region_set_sample (dest_reg, refsample);

  /* duplicate the connection list */
  dest_reg->conns = ipatch_dls2_conn_list_duplicate (src_reg->conns);

  IPATCH_ITEM_RUNLOCK (src_reg);
}

static void
ipatch_dls2_region_item_remove_full (IpatchItem *item, gboolean full)
{
  if (full)
    ipatch_dls2_region_set_sample (IPATCH_DLS2_REGION (item), NULL);

  if (IPATCH_ITEM_CLASS (ipatch_dls2_region_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_dls2_region_parent_class)->remove_full (item, full);
}

/**
 * ipatch_dls2_region_new:
 *
 * Create a new DLS region object.
 *
 * Returns: Newly created DLS region with a ref count of 1 which the caller
 * owns.
 */
IpatchDLS2Region *
ipatch_dls2_region_new (void)
{
  return (IPATCH_DLS2_REGION (g_object_new (IPATCH_TYPE_DLS2_REGION, NULL)));
}

/**
 * ipatch_dls2_region_first: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Region items
 *
 * Gets the first item in a region iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first region in @iter or %NULL if empty.
 */
IpatchDLS2Region *
ipatch_dls2_region_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_DLS2_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_region_next: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Region items
 *
 * Gets the next item in a region iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next region in @iter or %NULL if at the end of
 *   the list.
 */
IpatchDLS2Region *
ipatch_dls2_region_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_DLS2_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_region_get_info:
 * @region: DLS region to get info from
 * @fourcc: FOURCC integer id of INFO to get
 *
 * Get a DLS region info string by FOURCC integer ID (integer
 * representation of a 4 character RIFF chunk ID, see
 * #IpatchRiff).
 *
 * Returns: New allocated info string value or %NULL if no info with the
 * given @fourcc ID. String should be freed when finished with it.
 */
char *
ipatch_dls2_region_get_info (IpatchDLS2Region *region, guint32 fourcc)
{
  char *val;

  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  val = ipatch_dls2_info_get (region->info, fourcc);
  IPATCH_ITEM_RUNLOCK (region);

  return (val);
}

/**
 * ipatch_dls2_region_set_info:
 * @region: DLS region to set info of
 * @fourcc: FOURCC integer ID of INFO to set
 * @val: (nullable): Value to set info to or %NULL to unset (clear) info.
 *
 * Sets an INFO value in a DLS region object.
 * Emits changed signal.
 */
void
ipatch_dls2_region_set_info (IpatchDLS2Region *region, guint32 fourcc,
			     const char *val)
{
  GValue newval = { 0 }, oldval = { 0 };

  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, val);

  g_value_init (&oldval, G_TYPE_STRING);
  g_value_take_string (&oldval, ipatch_dls2_region_get_info (region, fourcc));

  IPATCH_ITEM_WLOCK (region);
  ipatch_dls2_info_set (&region->info, fourcc, val);
  IPATCH_ITEM_WUNLOCK (region);

  ipatch_dls2_info_notify ((IpatchItem *)region, fourcc, &newval, &oldval);

  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_dls2_region_set_sample:
 * @region: Region to set sample of
 * @sample: Sample to set region to
 *
 * Sets the referenced sample of a region.
 */
void
ipatch_dls2_region_set_sample (IpatchDLS2Region *region,
			       IpatchDLS2Sample *sample)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (IPATCH_IS_DLS2_SAMPLE (sample));

  ipatch_dls2_region_real_set_sample (region, sample, TRUE);
}

static void
ipatch_dls2_region_real_set_sample (IpatchDLS2Region *region,
				    IpatchDLS2Sample *sample,
				    gboolean sample_notify)
{
  GValue newval = { 0 }, oldval = { 0 };
  IpatchDLS2SampleInfo oldinfo, newinfo;

  if (sample_notify)
    ipatch_item_get_property_fast ((IpatchItem *)region, link_item_pspec,
				   &oldval);

  /* get all values of current sample info */
  ipatch_dls2_region_get_sample_info (region, &oldinfo);

  IPATCH_ITEM_WLOCK (region);
  if (region->sample) g_object_unref (region->sample);
  if (sample) g_object_ref (sample);
  region->sample = sample;
  IPATCH_ITEM_WUNLOCK (region);

  if (sample_notify)
    {
      g_value_init (&newval, IPATCH_TYPE_DLS2_SAMPLE);
      g_value_set_object (&newval, sample);
      ipatch_item_prop_notify ((IpatchItem *)region, link_item_pspec,
			       &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  /* notify title property change */
  g_value_init (&newval, G_TYPE_STRING);
  ipatch_dls2_region_get_title (region, &newval);
  ipatch_item_prop_notify ((IpatchItem *)region, ipatch_item_pspec_title,
			   &newval, NULL);
  g_value_unset (&newval);

  /* notify for sample info properties */
  ipatch_dls2_region_get_sample_info (region, &newinfo);

  ipatch_dls2_sample_info_notify_changes ((IpatchItem *)region, &newinfo,
					  &oldinfo);
}

static void
ipatch_dls2_region_get_sample_info (IpatchDLS2Region *region,
				    IpatchDLS2SampleInfo *info)
{
  IpatchDLS2Sample *sample = NULL;
  gboolean info_set = TRUE;

  IPATCH_ITEM_RLOCK (region);

  if (ipatch_item_get_flags (region) & IPATCH_DLS2_REGION_SAMPLE_INFO_OVERRIDE
      && region->sample_info)
    *info = *region->sample_info;
  else if (region->sample) sample = g_object_ref (region->sample);
  else info_set = FALSE;

  IPATCH_ITEM_RUNLOCK (region);

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

/**
 * ipatch_dls2_region_get_sample:
 * @region: Region to get referenced sample from
 *
 * Gets the referenced sample from a region.  The returned item's
 * reference count is incremented and the caller is responsible for
 * unrefing it with g_object_unref().
 *
 * Returns: (transfer full): Region's referenced sample or %NULL if not set yet. Remember to
 * unreference the item with g_object_unref() when done with it.
 */
IpatchDLS2Sample *
ipatch_dls2_region_get_sample (IpatchDLS2Region *region)
{
  IpatchDLS2Sample *sample;

  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  sample = region->sample;
  if (sample) g_object_ref (sample);
  IPATCH_ITEM_RUNLOCK (region);

  return (sample);
}

/**
 * ipatch_dls2_region_peek_sample: (skip)
 * @region: Region to get referenced sample from
 *
 * Like ipatch_dls2_region_get_sample() but does not add a reference to
 * the returned item. This function should only be used if a reference
 * of the returned item is ensured or only the pointer value is of
 * interest.
 *
 * Returns: (transfer none): Region's referenced sample or %NULL if not set yet.
 * Remember that the item has NOT been referenced.
 */
IpatchDLS2Sample *
ipatch_dls2_region_peek_sample (IpatchDLS2Region *region)
{
  IpatchDLS2Sample *sample;

  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  sample = region->sample;
  IPATCH_ITEM_RUNLOCK (region);

  return (sample);
}

/**
 * ipatch_dls2_region_set_note_range:
 * @region: Region to set note range of
 * @low: Low value of range (MIDI note # between 0 and 127)
 * @high: High value of range (MIDI note # between 0 and 127)
 *
 * Set the MIDI note range that a region is active on.
 */
void
ipatch_dls2_region_set_note_range (IpatchDLS2Region *region, int low, int high)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (low >= 0 && low <= 127);
  g_return_if_fail (high >= 0 && high <= 127);

  if (low > high)		/* swap if backwards */
    {
      int temp = low;
      low = high;
      high = temp;
    }

  IPATCH_ITEM_WLOCK (region);
  region->note_range_low = low;
  region->note_range_high = high;
  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_dls2_region_set_velocity_range:
 * @region: Region to set velocity range of
 * @low: Low value of range (MIDI velocity # between 0 and 127)
 * @high: High value of range (MIDI velocity # between 0 and 127)
 *
 * Set the MIDI velocity range that a region is active on.
 */
void
ipatch_dls2_region_set_velocity_range (IpatchDLS2Region *region,
				       int low, int high)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (low >= 0 && low <= 127);
  g_return_if_fail (high >= 0 && high <= 127);

  if (low > high)		/* swap if backwards */
    {
      int temp = low;
      low = high;
      high = temp;
    }

  IPATCH_ITEM_WLOCK (region);
  region->velocity_range_low = low;
  region->velocity_range_high = high;
  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_dls2_region_in_range:
 * @region: Region to check if in range
 * @note: MIDI note number or -1 for wildcard
 * @velocity: MIDI velocity or -1 for wildcard
 *
 * Check if a note and velocity falls in a region's ranges
 *
 * Returns: %TRUE if region is in note and velocity range, %FALSE otherwise
 */
gboolean
ipatch_dls2_region_in_range (IpatchDLS2Region *region, int note, int velocity)
{
  gboolean in_range;

  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), FALSE);

  IPATCH_ITEM_RLOCK (region);

  in_range = (note == -1 || (note >= region->note_range_low
			     && note <= region->note_range_high))
    && (velocity == -1 || (velocity >= region->velocity_range_low
			   && velocity <= region->velocity_range_high));

  IPATCH_ITEM_RUNLOCK (region);

  return (in_range);
}

/**
 * ipatch_dls2_region_set_param:
 * @region: Region to set parameter of
 * @param: Parameter to set
 * @val: Value for parameter
 *
 * Sets an effect parameter of a DLS2 Region.  DLS2 defines a standard set
 * of connections (effect parameters).  Any non-standard connections can be
 * manipulated with the connection related functions.
 */
void
ipatch_dls2_region_set_param (IpatchDLS2Region *region,
			      IpatchDLS2Param param, gint32 val)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (param < IPATCH_DLS2_PARAM_COUNT);

  /* no need to lock, since write of 32 bit int is atomic */
  region->params.values[param] = val;
}

/**
 * ipatch_dls2_region_set_param_array:
 * @region: Region to set parameter of
 * @array: Array of parameter values to copy to region
 *
 * Sets all effect parameters of a DLS2 Region.
 */
void
ipatch_dls2_region_set_param_array (IpatchDLS2Region *region,
				    IpatchDLS2ParamArray *array)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (array != NULL);
  int i;

  /* Write of each parameter is atomic. */
  for (i = 0; i < IPATCH_DLS2_PARAM_COUNT; i++)
    region->params.values[i] = array->values[i];
}

/**
 * ipatch_dls2_region_get_conns:
 * @region: Region to get connections from
 *
 * Gets a list of connections from a DLS region. List should be freed with
 * ipatch_dls2_conn_list_free() (free_conns set to %TRUE) when finished
 * with it.
 *
 * Returns: (element-type IpatchDLS2Conn) (transfer full): New list of connections
 *   (#IpatchDLS2Conn) in @region or %NULL if no connections. Remember to free
 *   it when finished.
 */
GSList *
ipatch_dls2_region_get_conns (IpatchDLS2Region *region)
{
  GSList *newlist;

  g_return_val_if_fail (IPATCH_IS_DLS2_REGION (region), NULL);

  IPATCH_ITEM_RLOCK (region);
  newlist = ipatch_dls2_conn_list_duplicate (region->conns);
  IPATCH_ITEM_RUNLOCK (region);

  return (newlist);
}

/**
 * ipatch_dls2_region_set_conn:
 * @region: DLS region
 * @conn: Connection
 *
 * Set a DLS connection in a region. See ipatch_dls2_conn_list_set() for
 * more details.
 */
void
ipatch_dls2_region_set_conn (IpatchDLS2Region *region,
			     const IpatchDLS2Conn *conn)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (conn != NULL);

  IPATCH_ITEM_WLOCK (region);
  ipatch_dls2_conn_list_set (&region->conns, conn);
  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_dls2_region_unset_conn:
 * @region: DLS region
 * @conn: Connection
 *
 * Remove a DLS connection from a region. See ipatch_dls2_conn_list_unset()
 * for more details.
 */
void
ipatch_dls2_region_unset_conn (IpatchDLS2Region *region,
			       const IpatchDLS2Conn *conn)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));
  g_return_if_fail (conn != NULL);

  IPATCH_ITEM_WLOCK (region);
  ipatch_dls2_conn_list_unset (&region->conns, conn);
  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_dls2_region_unset_all_conns:
 * @region: DLS region
 *
 * Remove all connections in a region.
 */
void
ipatch_dls2_region_unset_all_conns (IpatchDLS2Region *region)
{
  g_return_if_fail (IPATCH_IS_DLS2_REGION (region));

  IPATCH_ITEM_WLOCK (region);
  ipatch_dls2_conn_list_free (region->conns, TRUE);
  region->conns = NULL;
  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_dls2_region_conn_count:
 * @region: Region to count connections in
 *
 * Count number of connections in a region
 *
 * Returns: Count of connections
 */
guint
ipatch_dls2_region_conn_count (IpatchDLS2Region *region)
{
  guint i;

  IPATCH_ITEM_RLOCK (region);
  i = g_slist_length (region->conns);
  IPATCH_ITEM_RUNLOCK (region);

  return (i);
}

/**
 * ipatch_dls2_region_channel_map_stereo:
 * @chan: Channel steering enum
 *
 * Map a DLS2 channel steering enumeration (surround sound capable) to stereo
 * steering.
 *
 * Returns: -1 = left, 0 = center, 1 = right
 */
int
ipatch_dls2_region_channel_map_stereo (IpatchDLS2RegionChannelType chan)
{
  switch (chan)
    {
    case IPATCH_DLS2_REGION_CHANNEL_LEFT:
    case IPATCH_DLS2_REGION_CHANNEL_SURROUND_LEFT:
    case IPATCH_DLS2_REGION_CHANNEL_LEFT_OF_CENTER:
    case IPATCH_DLS2_REGION_CHANNEL_SIDE_LEFT:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_LEFT:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_LEFT:
      return (-1);

    case IPATCH_DLS2_REGION_CHANNEL_RIGHT:
    case IPATCH_DLS2_REGION_CHANNEL_SURROUND_RIGHT:
    case IPATCH_DLS2_REGION_CHANNEL_RIGHT_OF_CENTER:
    case IPATCH_DLS2_REGION_CHANNEL_SIDE_RIGHT:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_RIGHT:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_RIGHT:
      return (1);

    case IPATCH_DLS2_REGION_CHANNEL_CENTER:
    case IPATCH_DLS2_REGION_CHANNEL_LOW_FREQ:
    case IPATCH_DLS2_REGION_CHANNEL_SURROUND_CENTER:
    case IPATCH_DLS2_REGION_CHANNEL_TOP:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_FRONT_CENTER:
    case IPATCH_DLS2_REGION_CHANNEL_TOP_REAR_CENTER:
      return (0);

    default:
      return (0);
    }
}
