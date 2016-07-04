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
 * SECTION: IpatchGigRegion
 * @short_description: GigaSampler region object
 * @see_also: #IpatchGigInst, #IpatchGig
 * @stability: Stable
 *
 * GigaSampler region objects are children of #IpatchGigInst objects.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchGigRegion.h"
#include "IpatchGigFile.h"
#include "IpatchGigFile_priv.h"
#include "IpatchGigSample.h"
#include "IpatchRange.h"
#include "IpatchTypeProp.h"
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

  /* IpatchItem flags (no one needs to know that though) */
  PROP_SELF_NON_EXCLUSIVE,
  PROP_PHASE_MASTER,
  PROP_MULTI_CHANNEL
};

static void ipatch_gig_region_class_init (IpatchGigRegionClass *klass);
static void ipatch_gig_region_set_property (GObject *object, guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_gig_region_get_property (GObject *object, guint property_id,
					    GValue *value, GParamSpec *pspec);

static void ipatch_gig_region_init (IpatchGigRegion *gig_region);
static void ipatch_gig_region_finalize (GObject *gobject);
static void ipatch_gig_region_item_copy (IpatchItem *dest, IpatchItem *src,
					 IpatchItemCopyLinkFunc link_func,
					 gpointer user_data);
static const GType *ipatch_gig_region_container_child_types (void);
static gboolean
ipatch_gig_region_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type);


static GObjectClass *parent_class = NULL;
static GType gig_region_child_types[3] = { 0 };


GType
ipatch_gig_region_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchGigRegionClass), NULL, NULL,
      (GClassInitFunc)ipatch_gig_region_class_init, NULL, NULL,
      sizeof (IpatchGigRegion), 0,
      (GInstanceInitFunc)ipatch_gig_region_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					"IpatchGigRegion", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_gig_region_class_init (IpatchGigRegionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_gig_region_finalize;
  obj_class->get_property = ipatch_gig_region_get_property;

  item_class->item_set_property = ipatch_gig_region_set_property;
  item_class->copy = ipatch_gig_region_item_copy;

  container_class->child_types = ipatch_gig_region_container_child_types;
  container_class->init_iter = ipatch_gig_region_container_init_iter;
  container_class->make_unique = NULL;

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

  gig_region_child_types[0] = IPATCH_TYPE_GIG_DIMENSION;
  gig_region_child_types[1] = IPATCH_TYPE_GIG_SUB_REGION;
}

static void
ipatch_gig_region_get_title (IpatchGigRegion *region, GValue *value)
{
  IpatchRange *range = NULL;
  char *s = NULL;

  g_object_get (region, "note-range", &range, NULL);

  if (range)
    {
      if (range->low != range->high)
	s = g_strdup_printf (_("Note %d-%d"), range->low, range->high);
      else s = g_strdup_printf (_("Note %d"), range->low);

      ipatch_range_free (range);
    }

  g_value_take_string (value, s);
}

static void
ipatch_gig_region_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  IpatchGigRegion *region = IPATCH_GIG_REGION (object);
  IpatchRange *range;
  gboolean retval;
  
  switch (property_id)
    {
    case PROP_NOTE_RANGE:
      range = ipatch_value_get_range (value);
      if (range)
	ipatch_gig_region_set_note_range (region, range->low, range->high);
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
    case PROP_SELF_NON_EXCLUSIVE:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE);
      break;
    case PROP_PHASE_MASTER:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_GIG_REGION_PHASE_MASTER);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_GIG_REGION_PHASE_MASTER);
      break;
    case PROP_MULTI_CHANNEL:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags (IPATCH_ITEM (object),
			       IPATCH_GIG_REGION_MULTI_CHANNEL);
      else ipatch_item_clear_flags (IPATCH_ITEM (object),
				    IPATCH_GIG_REGION_MULTI_CHANNEL);
      break;
    default:
      IPATCH_ITEM_WLOCK (region);
      retval = ipatch_dls2_info_set_property (&region->info,
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
ipatch_gig_region_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchGigRegion *region = IPATCH_GIG_REGION (object);
  IpatchRange range;
  gboolean bool, retval;

  switch (property_id)
    {
    case PROP_TITLE:
      ipatch_gig_region_get_title (region, value);
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
    case PROP_SELF_NON_EXCLUSIVE:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	& IPATCH_GIG_REGION_SELF_NON_EXCLUSIVE) > 0;
      g_value_set_boolean (value, bool);
      break;
    case PROP_PHASE_MASTER:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	      & IPATCH_GIG_REGION_PHASE_MASTER) > 0;
      g_value_set_boolean (value, bool);
      break;
    case PROP_MULTI_CHANNEL:
      bool = (ipatch_item_get_flags (IPATCH_ITEM (object))
	      & IPATCH_GIG_REGION_MULTI_CHANNEL) > 0;
      g_value_set_boolean (value, bool);
      break;
    default:
      IPATCH_ITEM_RLOCK (region);
      retval = ipatch_dls2_info_get_property (region->info,
					      property_id, value);
      IPATCH_ITEM_RUNLOCK (region);

      if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_gig_region_init (IpatchGigRegion *region)
{
  int i;

  region->note_range_low = 0;
  region->note_range_high = 127;
  region->velocity_range_low = 0;
  region->velocity_range_high = 127;
  region->key_group = 0;
  region->layer_group = 0;
  region->phase_group = 0;
  region->channel = 0;
  region->info = NULL;

  region->dimension_count = 0;
  region->sub_region_count = 1; /* always at least 1 sub region */
  region->sub_regions[0] = ipatch_gig_sub_region_new ();
  ipatch_item_set_parent (IPATCH_ITEM (region->sub_regions[0]),
			  IPATCH_ITEM (region));

  /* FIXME - What is this for really? */
  for (i = 0; i < IPATCH_GIG_3DDP_SIZE; i++)
    region->chunk_3ddp[i] = 0xFF;
}

static void
ipatch_gig_region_finalize (GObject *gobject)
{
  IpatchGigRegion *gig_region = IPATCH_GIG_REGION (gobject);
  int i;

  IPATCH_ITEM_WLOCK (gig_region);

  /* delete all dimensions */
  for (i = 0; i < gig_region->dimension_count; i++)
    {
      g_object_unref (gig_region->dimensions[i]);
      gig_region->dimensions[i] = NULL;
    }

  /* delete all sub regions */
  for (i = 0; i < gig_region->sub_region_count; i++)
    {
      g_object_unref (gig_region->sub_regions[i]);
      gig_region->sub_regions[i] = NULL;
    }

  IPATCH_ITEM_WUNLOCK (gig_region);

  if (parent_class->finalize)
    parent_class->finalize (gobject);
}

static void
ipatch_gig_region_item_copy (IpatchItem *dest, IpatchItem *src,
			     IpatchItemCopyLinkFunc link_func,
			     gpointer user_data)
{
  IpatchGigRegion *src_reg, *dest_reg;
  IpatchGigDimension *src_dim;
  IpatchGigSubRegion *src_sub;
  IpatchItem *new_dim, *new_sub;
  int i;

  src_reg = IPATCH_GIG_REGION (src);
  dest_reg = IPATCH_GIG_REGION (dest);

  IPATCH_ITEM_RLOCK (src_reg);

  /* duplicate the flags */
  ipatch_item_set_flags (dest_reg, ipatch_item_get_flags (src_reg)
			 & IPATCH_GIG_REGION_FLAG_MASK);

  dest_reg->note_range_low = src_reg->note_range_low;
  dest_reg->note_range_high = src_reg->note_range_high;
  dest_reg->velocity_range_low = src_reg->velocity_range_low;
  dest_reg->velocity_range_high = src_reg->velocity_range_high;
  dest_reg->key_group = src_reg->key_group;
  dest_reg->layer_group = src_reg->layer_group;
  dest_reg->phase_group = src_reg->phase_group;
  dest_reg->channel = src_reg->channel;

  dest_reg->info = ipatch_dls2_info_duplicate (src_reg->info);

  /* copy dimensions */
  for (i = 0; i < src_reg->dimension_count; i++)
    {
      src_dim = src_reg->dimensions[i];
      new_dim = ipatch_item_duplicate_link_func ((IpatchItem *)src_dim,
						 link_func, user_data);
      g_return_if_fail (new_dim != NULL);

      dest_reg->dimensions[i] = IPATCH_GIG_DIMENSION (new_dim);
      dest_reg->dimension_count = i + 1;  /* update count in case of failure */
    }

  /* copy sub regions */
  for (i = 0; i < src_reg->sub_region_count; i++)
    {
      src_sub = src_reg->sub_regions[i];
      new_sub = ipatch_item_duplicate_link_func ((IpatchItem *)src_sub,
						 link_func, user_data);
      g_return_if_fail (new_sub != NULL);

      dest_reg->sub_regions[i] = IPATCH_GIG_SUB_REGION (new_sub);
      dest_reg->sub_region_count = i + 1;  /* update count in case of failure */
    }

  IPATCH_ITEM_RUNLOCK (src_reg);
}

static const GType *
ipatch_gig_region_container_child_types (void)
{
  return (gig_region_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_gig_region_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type)
{
  IpatchGigRegion *region = IPATCH_GIG_REGION (container);

  if (g_type_is_a (type, IPATCH_TYPE_GIG_DIMENSION))
    ipatch_iter_array_init (iter, (gpointer *)(region->dimensions),
			    region->dimension_count);
  else if (g_type_is_a (type, IPATCH_TYPE_GIG_SUB_REGION))
    ipatch_iter_array_init (iter, (gpointer *)(region->sub_regions),
			    region->sub_region_count);
  else
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  return (TRUE);
}

/**
 * ipatch_gig_region_new:
 *
 * Create a new GigaSampler instrument region.
 *
 * Returns: New GigaSampler region with a ref count of 1 which the caller
 * owns.
 */
IpatchGigRegion *
ipatch_gig_region_new (void)
{
  return (IPATCH_GIG_REGION (g_object_new (IPATCH_TYPE_GIG_REGION, NULL)));
}

/**
 * ipatch_gig_region_first: (skip)
 * @iter: Patch item iterator containing #IpatchGigRegion items
 *
 * Gets the first item in a region iterator. A convenience
 * wrapper for ipatch_iter_first().
 *
 * Returns: The first region in @iter or %NULL if empty.
 */
IpatchGigRegion *
ipatch_gig_region_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_GIG_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_gig_region_next: (skip)
 * @iter: Patch item iterator containing #IpatchGigRegion items
 *
 * Gets the next item in a region iterator. A convenience wrapper
 * for ipatch_iter_next().
 *
 * Returns: The next region in @iter or %NULL if at the end of
 *   the list.
 */
IpatchGigRegion *
ipatch_gig_region_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_GIG_REGION (obj));
  else return (NULL);
}

/**
 * ipatch_gig_region_set_note_range:
 * @region: Region to set note range of
 * @low: Low value of range (MIDI note # between 0 and 127)
 * @high: High value of range (MIDI note # between 0 and 127)
 *
 * Set the MIDI note range that a region is active on.
 */
void
ipatch_gig_region_set_note_range (IpatchGigRegion *region, int low, int high)
{
  GValue titleval = { 0 };

  g_return_if_fail (IPATCH_IS_GIG_REGION (region));
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

  /* title property notify */
  g_value_init (&titleval, G_TYPE_STRING);
  ipatch_gig_region_get_title (region, &titleval);
  ipatch_item_prop_notify ((IpatchItem *)region, ipatch_item_pspec_title,
			   &titleval, NULL);
  g_value_unset (&titleval);
}

/**
 * ipatch_gig_region_set_velocity_range:
 * @region: Region to set velocity range of
 * @low: Low value of range (MIDI velocity # between 0 and 127)
 * @high: High value of range (MIDI velocity # between 0 and 127)
 *
 * Set the MIDI velocity range that a region is active on.
 */
void
ipatch_gig_region_set_velocity_range (IpatchGigRegion *region,
				      int low, int high)
{
  g_return_if_fail (IPATCH_IS_GIG_REGION (region));
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
 * ipatch_gig_region_new_dimension:
 * @region: GigaSampler region
 * @type: Type of dimension to add
 * @split_count: Split bit count
 *
 * Adds a new dimension to a GigaSampler region. The dimension is allocated
 * @split_count number of dimension bits which means the total number of
 * sub regions will be multiplied by a factor of 2 to the power of
 * @split_count. There can be a maximum of 32 sub regions for a total of
 * 5 used split bits.
 */
void
ipatch_gig_region_new_dimension (IpatchGigRegion *region,
				 IpatchGigDimensionType type, int split_count)
{
  IpatchGigDimension *dimension;
  int new_sub_region_count;
  int mask, shift;
  int i;

  g_return_if_fail (IPATCH_IS_GIG_REGION (region));
  g_return_if_fail (split_count > 0);

  IPATCH_ITEM_WLOCK (region);

  new_sub_region_count = region->sub_region_count * (1 << split_count);
  if (log_if_fail (new_sub_region_count <= 32))
    {
      IPATCH_ITEM_WUNLOCK (region);
      return;
    }

  /* calculate dimension split bit shift value */
  for (i = region->sub_region_count, shift = 0; !(i & 1); i >>= 1, shift++);

  /* calculate unshifted mask for the split bit count */
  for (i = 0, mask = 0; i < split_count; i++, mask = (mask << 1) | 1);

  dimension = ipatch_gig_dimension_new ();
  dimension->type = type;
  dimension->split_count = split_count;
  dimension->split_mask = mask << shift;
  dimension->split_shift = shift;

  region->dimensions[region->dimension_count] = dimension;
  region->dimension_count++;

  ipatch_item_set_parent (IPATCH_ITEM (dimension), IPATCH_ITEM (region));

  for (i = region->sub_region_count; i < new_sub_region_count; i++)
    {
      region->sub_regions[i] = ipatch_gig_sub_region_new ();
      ipatch_item_set_parent (IPATCH_ITEM (region->sub_regions[i]),
			      IPATCH_ITEM (region));
    }

  region->sub_region_count = new_sub_region_count;

  IPATCH_ITEM_WUNLOCK (region);
}

/**
 * ipatch_gig_region_remove_dimension:
 * @region: GigaSampler region
 * @dim_index: Index of an existing dimension to remove (0-4)
 * @split_index: Split index to use in the dimension to remove
 *
 * Removes a dimension from a GigaSampler region, including all related
 * sub regions (except those that correspond to the @split_index), and
 * re-organizes sub regions for new dimension map.
 */
void
ipatch_gig_region_remove_dimension (IpatchGigRegion *region, int dim_index,
				    int split_index)
{
  IpatchGigSubRegion *new_regions[32] = { NULL };
  int new_region_index = 0;
  guint max_split_index;
  guint8 index[5];		/* current index for each dimension */
  guint8 max[5];		/* max count for each dimension (+1) */
  int sub_index, bit_index;
  int i;

  g_return_if_fail (IPATCH_IS_GIG_REGION (region));

  IPATCH_ITEM_WLOCK (region);

  if (log_if_fail (dim_index >= 0 && dim_index < region->dimension_count))
    {
      IPATCH_ITEM_WUNLOCK (region);
      return;
    }

  max_split_index = 1 << region->dimensions[dim_index]->split_count;
  if (log_if_fail (split_index > 0 && split_index < max_split_index))
    {
      IPATCH_ITEM_WUNLOCK (region);
      return;
    }

  /* initialize dimension index and max arrays */
  for (i = 0; i < region->dimension_count; i++)
    {
      index[i] = 0;
      max[i] = 1 << region->dimensions[i]->split_count;
    }

  index[dim_index] = split_index; /* the split index to use */

  /* move pointers of sub regions we want to keep to new_regions array */
  while (TRUE)
    {
      /* calculate current sub region index */
      sub_index = 0;
      bit_index = 0;
      for (i = 0; i < region->dimension_count; i++)
	{
	  sub_index += index[i] << bit_index;
	  bit_index += region->dimensions[i]->split_count;
	}

      /* move the sub region pointer to new region array */
      new_regions[new_region_index++] = region->sub_regions[sub_index];
      region->sub_regions[sub_index] = NULL; /* clear pointer */

      /* increment dimension indexes in binary fashion */
      i = (dim_index != 0) ? 0 : 1; /* first dimension to increment index of */
      while (i < region->dimension_count)
	{
	  if (++index[i] < max[i]) break; /* carry bit to next dimension? */
	  index[i] = 0;
	  if (++i == dim_index) i++; /* skip remove dimension */
	}

      if (i == region->dimension_count) break; /* are we done yet? */
    }

  /* free unused sub regions */
  for (i = 0; i < region->sub_region_count; i++)
    if (region->sub_regions[i])
      g_object_unref (region->sub_regions[i]);

  /* copy saved sub region pointers back into region */
  for (i = 0; i < new_region_index; i++)
    region->sub_regions[i] = new_regions[i];

  /* shift dimensions down into the deleted slot */
  for (i = dim_index; i < region->dimension_count - 1; i++)
    region->dimensions[i] = region->dimensions[i+1];

  region->sub_region_count = new_region_index;
  region->dimension_count--;

  IPATCH_ITEM_WUNLOCK (region);
}
