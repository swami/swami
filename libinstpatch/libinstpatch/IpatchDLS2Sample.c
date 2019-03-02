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
 * SECTION: IpatchDLS2Sample
 * @short_description: DLS audio sample object
 * @see_also: #IpatchDLS, #IpatchDLSRegion
 * @stability: Stable
 *
 * Object which defines a DLS audio sample.  These objects are contained in
 * #IpatchDLS objects and linked (referenced) from #IpatchDLSRegion objects.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2Sample.h"
#include "IpatchDLS2.h"
#include "IpatchDLSFile.h"
#include "IpatchDLSFile_priv.h"
#include "IpatchSample.h"
#include "IpatchSampleStoreRam.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"

/* properties */
enum {
  PROP_0,
  PROP_SAMPLE_SIZE,		/* read only convenience property */
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_SAMPLE_DATA
};


/* sample info property enums, used by regions as well, so we define these
   in a non-conflicting range
   !! Keep order synchronized with IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT */
enum
{
  PROP_FLAGS      = IPATCH_DLS2_SAMPLE_INFO_FIRST_PROPERTY_ID,
  PROP_LOOP_TYPE,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE,
  PROP_GAIN,
  PROP_LOOP_START,
  PROP_LOOP_END
};

/* for caching sample info GParamSpec objects for an object class */
typedef struct
{
  GObjectClass *klass;		/* object class owning these properties */
  GParamSpec *pspecs[IPATCH_DLS2_SAMPLE_INFO_PROPERTY_COUNT];
} ClassPropBag;

static void ipatch_dls2_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean
ipatch_dls2_sample_iface_open (IpatchSampleHandle *handle, GError **err);

static void ipatch_dls2_sample_finalize (GObject *gobject);
static void ipatch_dls2_sample_set_property (GObject *object,
					     guint property_id,
					     const GValue *value,
					     GParamSpec *pspec);
static void ipatch_dls2_sample_get_property (GObject *object,
					     guint property_id,
					     GValue *value,
					     GParamSpec *pspec);
static void ipatch_dls2_sample_item_copy (IpatchItem *dest, IpatchItem *src,
					  IpatchItemCopyLinkFunc link_func,
					  gpointer user_data);
static void ipatch_dls2_sample_item_remove_full (IpatchItem *item, gboolean full);
static gboolean ipatch_dls2_sample_real_set_data (IpatchDLS2Sample *sample,
						  IpatchSampleData *sampledata);


/* list of ClassPropBag to speed up info property notifies */
static GSList *info_pspec_list = NULL;


G_DEFINE_TYPE_WITH_CODE (IpatchDLS2Sample, ipatch_dls2_sample,
                         IPATCH_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_dls2_sample_iface_init))

/* sample interface initialization */
static void
ipatch_dls2_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_dls2_sample_iface_open;
  sample_iface->loop_types = ipatch_sample_loop_types_standard_release;
}

static gboolean
ipatch_dls2_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchDLS2Sample *sample = IPATCH_DLS2_SAMPLE (handle->sample);
  g_return_val_if_fail (sample->sample_data != NULL, FALSE);
  return (ipatch_sample_handle_cascade_open
          (handle, (IpatchSample *)(sample->sample_data), err));
}

static void
ipatch_dls2_sample_class_init (IpatchDLS2SampleClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->finalize = ipatch_dls2_sample_finalize;
  obj_class->get_property = ipatch_dls2_sample_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_dls2_sample_set_property;
  item_class->copy = ipatch_dls2_sample_item_copy;
  item_class->remove_full = ipatch_dls2_sample_item_remove_full;

  g_object_class_override_property (obj_class, IPATCH_DLS2_NAME, "title");

  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE, "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT, "sample-format");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_RATE, "sample-rate");
  ipatch_sample_install_property (obj_class, PROP_SAMPLE_DATA, "sample-data");

  ipatch_dls2_sample_info_install_class_properties (obj_class);
  ipatch_dls2_info_install_class_properties (obj_class);
}

static void
ipatch_dls2_sample_init (IpatchDLS2Sample *sample)
{
  ipatch_dls2_sample_set_blank (sample);
  sample->rate = IPATCH_SAMPLE_RATE_DEFAULT;
}

static void
ipatch_dls2_sample_finalize (GObject *gobject)
{
  IpatchDLS2Sample *sample = IPATCH_DLS2_SAMPLE (gobject);

  /* nothing should reference the sample after this, but we set
     pointers to NULL to help catch invalid references. Locking of
     sample is required since in reality all its children do
     still hold references */

  IPATCH_ITEM_WLOCK (sample);
  
  if (sample->sample_data)
  {
    ipatch_sample_data_unused (sample->sample_data);  // -- dec use count
    g_object_unref (sample->sample_data);             // -- dec reference count
  }

  if (sample->sample_info) ipatch_dls2_sample_info_free (sample->sample_info);
  ipatch_dls2_info_free (sample->info);

  g_free (sample->dlid);

  IPATCH_ITEM_WUNLOCK (sample);

  if (G_OBJECT_CLASS (ipatch_dls2_sample_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_dls2_sample_parent_class)->finalize (gobject);
}

static void
ipatch_dls2_sample_set_property (GObject *object, guint property_id,
				 const GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Sample *sample = IPATCH_DLS2_SAMPLE (object);
  gboolean retval;
  
  switch (property_id)
    {
    case PROP_SAMPLE_RATE:
      IPATCH_ITEM_WLOCK (sample);
      sample->rate = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_SAMPLE_DATA:
      ipatch_dls2_sample_real_set_data (sample, (IpatchSampleData *)
					(g_value_get_object (value)));
      break;
    default:
      IPATCH_ITEM_WLOCK (sample);

      retval = ipatch_dls2_sample_info_set_property (&sample->sample_info,
						     property_id, value);
      if (!retval) retval = ipatch_dls2_info_set_property (&sample->info,
							   property_id, value);
      IPATCH_ITEM_WUNLOCK (sample);

      /* check if "title" property needs to be notified */
      if (property_id == IPATCH_DLS2_NAME)
	ipatch_item_prop_notify ((IpatchItem *)sample, ipatch_item_pspec_title,
				 value, NULL);

      if (!retval)
	{
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
      break;
    }
}

static void
ipatch_dls2_sample_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Sample *sample = IPATCH_DLS2_SAMPLE (object);
  gboolean retval;

  switch (property_id)
    {
    case PROP_SAMPLE_SIZE:
      g_return_if_fail (sample->sample_data != NULL);
      g_object_get_property ((GObject *)(sample->sample_data), "sample-size", value);
      break;
    case PROP_SAMPLE_FORMAT:
      g_return_if_fail (sample->sample_data != NULL);
      g_object_get_property ((GObject *)(sample->sample_data), "sample-format", value);
      break;
    case PROP_SAMPLE_RATE:
      IPATCH_ITEM_RLOCK (sample);
      g_value_set_int (value, sample->rate);
      IPATCH_ITEM_RUNLOCK (sample);
      break;
    case PROP_SAMPLE_DATA:
      g_value_take_object (value, ipatch_dls2_sample_get_data (sample));
      break;
    default:
      IPATCH_ITEM_RLOCK (sample);
      retval = ipatch_dls2_sample_info_get_property (sample->sample_info,
						     property_id, value);
      if (!retval) retval = ipatch_dls2_info_get_property (sample->info,
							   property_id, value);
      IPATCH_ITEM_RUNLOCK (sample);

      if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_dls2_sample_item_copy (IpatchItem *dest, IpatchItem *src,
			      IpatchItemCopyLinkFunc link_func,
			      gpointer user_data)
{
  IpatchDLS2Sample *src_sam, *dest_sam;

  src_sam = IPATCH_DLS2_SAMPLE (src);
  dest_sam = IPATCH_DLS2_SAMPLE (dest);

  IPATCH_ITEM_RLOCK (src_sam);

  dest_sam->sample_info = src_sam->sample_info ?
    ipatch_dls2_sample_info_duplicate (src_sam->sample_info) : NULL;
  dest_sam->info = ipatch_dls2_info_duplicate (src_sam->info);
  ipatch_dls2_sample_set_data (dest_sam, src_sam->sample_data);

  if (src_sam->dlid)
    dest_sam->dlid = g_memdup (src_sam->dlid, IPATCH_DLS_DLID_SIZE);

  IPATCH_ITEM_RUNLOCK (src_sam);
}

static void
ipatch_dls2_sample_item_remove_full (IpatchItem *item, gboolean full)
{
  IpatchList *list;
  IpatchIter iter;

  /* ++ ref new list */
  list = ipatch_dls2_get_region_references (IPATCH_DLS2_SAMPLE (item));
  ipatch_list_init_iter (list, &iter);

  item = ipatch_item_first (&iter);

  while (item)
  {
    ipatch_item_remove (item);
    item = ipatch_item_next (&iter);
  }

  g_object_unref (list);	/* -- unref list */

  if (full)
    ipatch_dls2_sample_set_data (IPATCH_DLS2_SAMPLE (item), NULL);

  if (IPATCH_ITEM_CLASS (ipatch_dls2_sample_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_dls2_sample_parent_class)->remove_full (item, full);
}

/**
 * ipatch_dls2_sample_new:
 *
 * Create a new DLS sample object.
 *
 * Returns: New DLS sample with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchDLS2Sample *
ipatch_dls2_sample_new (void)
{
  return (IPATCH_DLS2_SAMPLE (g_object_new (IPATCH_TYPE_DLS2_SAMPLE, NULL)));
}

/**
 * ipatch_dls2_sample_first: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Sample items
 *
 * Gets the first item in a sample iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first sample in @iter or %NULL if empty.
 */
IpatchDLS2Sample *
ipatch_dls2_sample_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_DLS2_SAMPLE (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_sample_next: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Sample items
 *
 * Gets the next item in a sample iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next sample in @iter or %NULL if at the end of the list.
 */
IpatchDLS2Sample *
ipatch_dls2_sample_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_DLS2_SAMPLE (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_sample_set_data:
 * @sample: Sample to set sample data of
 * @sampledata: Sample data to set sample to
 *
 * Set a sample's sample data object.
 */
void
ipatch_dls2_sample_set_data (IpatchDLS2Sample *sample, IpatchSampleData *sampledata)
{
  if (ipatch_dls2_sample_real_set_data (sample, sampledata))
    g_object_notify (G_OBJECT (sample), "sample-data");
}

/* the actual setting of sample data, user routine does a g_object_notify */
static gboolean
ipatch_dls2_sample_real_set_data (IpatchDLS2Sample *sample,
				 IpatchSampleData *sampledata)
{
  IpatchSampleData *old_sampledata;

  g_return_val_if_fail (IPATCH_IS_DLS2_SAMPLE (sample), FALSE);
  g_return_val_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata), FALSE);

  g_object_ref (sampledata);	/* ++ ref for sample */
  ipatch_sample_data_used (sampledata);   /* ++ inc use count */

  IPATCH_ITEM_WLOCK (sample);
  old_sampledata = sample->sample_data;
  sample->sample_data = sampledata;	/* !! takes over ref */
  IPATCH_ITEM_WUNLOCK (sample);

  if (old_sampledata)
  {
    ipatch_sample_data_unused (old_sampledata);  // -- dec use count
    g_object_unref (old_sampledata);             // -- dec reference count
  }

  return (TRUE);
}

/**
 * ipatch_dls2_sample_get_data:
 * @sample: Sample to get sample data from
 *
 * Get the #IpatchSampleData item of a sample. Sample data item is referenced
 * before returning and caller is responsible for unreferencing it with
 * g_object_unref() when finished with it.
 *
 * Returns: (transfer full): Sample data object of sample or %NULL if none. Remember to
 * unreference with g_object_unref() when finished with it.
 */
IpatchSampleData *
ipatch_dls2_sample_get_data (IpatchDLS2Sample *sample)
{
  IpatchSampleData *sampledata;

  g_return_val_if_fail (IPATCH_IS_DLS2_SAMPLE (sample), NULL);

  IPATCH_ITEM_RLOCK (sample);
  sampledata = sample->sample_data;
  if (sampledata) g_object_ref (sampledata);	/* ++ ref */
  IPATCH_ITEM_RUNLOCK (sample);

  return (sampledata);	/* !! caller takes over ref */
}

/**
 * ipatch_dls2_sample_peek_data: (skip)
 * @sample: Sample to get sample data from
 *
 * Get the #IpatchSampleData item of a sample. Like
 * ipatch_dls2_sample_get_data() but sample data object is not referenced.
 * This function should only be used if a reference of the sample data object
 * is ensured or only the pointer value is of importance.
 *
 * Returns: (transfer none): Sample data object of sample or %NULL if none.
 * Remember that a reference is NOT added.
 */
IpatchSampleData *
ipatch_dls2_sample_peek_data (IpatchDLS2Sample *sample)
{
  IpatchSampleData *sampledata;

  g_return_val_if_fail (IPATCH_IS_DLS2_SAMPLE (sample), NULL);

  IPATCH_ITEM_RLOCK (sample);
  sampledata = sample->sample_data;
  IPATCH_ITEM_RUNLOCK (sample);

  return (sampledata);
}

/**
 * ipatch_dls2_sample_set_blank:
 * @sample: Sample to set to blank sample data
 *
 * Set the sample data of a sample item to blank data.
 */
void
ipatch_dls2_sample_set_blank (IpatchDLS2Sample *sample)
{
  IpatchSampleData *sampledata;

  g_return_if_fail (IPATCH_IS_DLS2_SAMPLE (sample));

  sampledata = ipatch_sample_data_get_blank ();

  IPATCH_ITEM_WLOCK (sample);
  if (sample->sample_info)	/* reset sample info to defaults */
    {
      ipatch_dls2_sample_info_free (sample->sample_info);
      sample->sample_info = NULL;
    }
  g_object_set (sample,
		"sample-data", sampledata,
		"sample-rate", 44100,
		NULL);
  IPATCH_ITEM_WUNLOCK (sample);

  g_object_unref (sampledata);
}

GType
ipatch_dls2_sample_info_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchDLS2SampleInfo",
				(GBoxedCopyFunc)ipatch_dls2_sample_info_duplicate,
				(GBoxedFreeFunc)ipatch_dls2_sample_info_free);

  return (type);
}

/**
 * ipatch_dls2_sample_info_new:
 *
 * Allocates a new sample info structure.
 *
 * Returns: (transfer full): New sample info structure, free it with
 * ipatch_dls2_sample_info_free() when finished.
 */
IpatchDLS2SampleInfo *
ipatch_dls2_sample_info_new (void)
{
  IpatchDLS2SampleInfo *sample_info;

  sample_info = g_slice_new0 (IpatchDLS2SampleInfo);
  sample_info->root_note = 60;

  return (sample_info);
}

/**
 * ipatch_dls2_sample_info_free:
 * @sample_info: Sample info structure
 *
 * Free a sample info structure allocated with ipatch_dls2_sample_info_new().
 */
void
ipatch_dls2_sample_info_free (IpatchDLS2SampleInfo *sample_info)
{
  g_slice_free (IpatchDLS2SampleInfo, sample_info);
}

/**
 * ipatch_dls2_sample_info_duplicate:
 * @sample_info: Sample info structure to duplicate
 *
 * Duplicate a sample info structure.
 *
 * Returns: Newly allocated sample info structure which should be freed
 * with ipatch_dls2_sample_info_free() when done with it.
 */
IpatchDLS2SampleInfo *
ipatch_dls2_sample_info_duplicate (IpatchDLS2SampleInfo *sample_info)
{
  IpatchDLS2SampleInfo *newinfo;

  g_return_val_if_fail (sample_info != NULL, NULL);

  newinfo = ipatch_dls2_sample_info_new ();
  *newinfo = *sample_info;

  return (newinfo);
}

/**
 * ipatch_dls2_sample_info_init:
 * @sample_info: Sample info structure to initialize
 *
 * Initialize a sample info structure to defaults.
 */
void
ipatch_dls2_sample_info_init (IpatchDLS2SampleInfo *sample_info)
{
  g_return_if_fail (sample_info != NULL);

  memset (sample_info, 0, sizeof (IpatchDLS2SampleInfo));
  sample_info->root_note = 60;
}

/**
 * ipatch_dls2_sample_info_install_class_properties: (skip)
 * @obj_class: GObjectClass to install properties for
 *
 * Installs sample info properties for the given @obj_class. Useful for
 * objects that implement #IpatchDLS2SampleInfo properties.
 */
void
ipatch_dls2_sample_info_install_class_properties (GObjectClass *obj_class)
{
  ClassPropBag *bag;

  /* add new bag to cache pspecs for this class */
  bag = g_new (ClassPropBag, 1);
  bag->klass = obj_class;
  info_pspec_list = g_slist_append (info_pspec_list, bag);

  /* properties defined by IpatchSample interface */
  bag->pspecs[0]
    = ipatch_sample_install_property (obj_class, PROP_LOOP_TYPE, "loop-type");
  bag->pspecs[1]
    = ipatch_sample_install_property (obj_class, PROP_LOOP_START, "loop-start");
  bag->pspecs[2]
    = ipatch_sample_install_property (obj_class, PROP_LOOP_END, "loop-end");
  bag->pspecs[3]
    = ipatch_sample_install_property (obj_class, PROP_ROOT_NOTE, "root-note");
  bag->pspecs[4]
    = ipatch_sample_install_property (obj_class, PROP_FINE_TUNE, "fine-tune");

  bag->pspecs[5]
    = g_param_spec_flags ("flags", _("Sample flags"),
			  _("Sample flags"),
			  IPATCH_TYPE_DLS2_SAMPLE_FLAGS,
			  0,
			  G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FLAGS, bag->pspecs[5]);

  bag->pspecs[6]
    = g_param_spec_int ("gain", _("Gain"),
			_("Gain in DLS relative gain units"),
			G_MININT, G_MAXINT, 0,
			G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_GAIN, bag->pspecs[6]);
}

/**
 * ipatch_dls2_sample_info_is_property_id_valid: (skip)
 * @property_id: Property ID to test
 *
 * Check if a property ID is a valid sample info property ID.
 *
 * Returns: %TRUE if property_id is a sample info property ID, %FALSE otherwise.
 */
gboolean
ipatch_dls2_sample_info_is_property_id_valid (guint property_id)
{
  return (property_id == PROP_FLAGS || property_id == PROP_LOOP_TYPE
	  || property_id == PROP_ROOT_NOTE || property_id == PROP_FINE_TUNE
	  || property_id == PROP_GAIN || property_id == PROP_LOOP_START
	  || property_id == PROP_LOOP_END);
}

/**
 * ipatch_dls2_sample_info_set_property: (skip)
 * @sample_info: Pointer to pointer to sample info
 * @property_id: Property ID
 * @value: Value for property
 *
 * A function used by set_property methods that implement #IpatchDLS2SampleInfo
 * properties.
 *
 * Returns: %TRUE if property_id was handled, %FALSE otherwise
 */
gboolean
ipatch_dls2_sample_info_set_property (IpatchDLS2SampleInfo **sample_info,
				      guint property_id, const GValue *value)
{
  IpatchDLS2SampleInfo *saminfo;

  if (!*sample_info)
    {
      if (property_id != PROP_FLAGS && property_id != PROP_LOOP_TYPE
	  && property_id != PROP_ROOT_NOTE && property_id != PROP_FINE_TUNE
	  && property_id != PROP_GAIN && property_id != PROP_LOOP_START
	  && property_id != PROP_LOOP_END)
	return (FALSE);

      *sample_info = ipatch_dls2_sample_info_new ();
    }

  saminfo = *sample_info;

  switch (property_id)
    {
    case PROP_FLAGS:
      saminfo->options &= ~IPATCH_DLS2_SAMPLE_FLAGS_MASK;
      saminfo->options |= g_value_get_flags (value)
	& IPATCH_DLS2_SAMPLE_FLAGS_MASK;
      break;
    case PROP_LOOP_TYPE:
      saminfo->options &= ~IPATCH_DLS2_SAMPLE_LOOP_MASK;
      saminfo->options |= g_value_get_enum (value)
	& IPATCH_DLS2_SAMPLE_LOOP_MASK;
      break;
    case PROP_ROOT_NOTE:
      saminfo->root_note = g_value_get_int (value);
      break;
    case PROP_FINE_TUNE:
      saminfo->fine_tune = g_value_get_int (value);
      break;
    case PROP_GAIN:
      saminfo->gain = g_value_get_int (value);
      break;
    case PROP_LOOP_START:
      saminfo->loop_start = g_value_get_uint (value);
      break;
    case PROP_LOOP_END:
      saminfo->loop_end = g_value_get_uint (value);
      break;
    default:
      return (FALSE);
    }

  return (TRUE);
}

/**
 * ipatch_dls2_sample_info_get_property: (skip)
 * @sample_info: Pointer to sample info
 * @property_id: Property ID
 * @value: Value to set
 *
 * A function used by get_property methods that implement #IpatchDLS2SampleInfo
 * properties.
 *
 * Returns: %TRUE if property_id was handled, %FALSE otherwise
 */
gboolean
ipatch_dls2_sample_info_get_property (IpatchDLS2SampleInfo *sample_info,
				      guint property_id, GValue *value)
{
  switch (property_id)
    {
    case PROP_FLAGS:
      g_value_set_flags (value, sample_info ?
			 (sample_info->options
			  & IPATCH_DLS2_SAMPLE_FLAGS_MASK) : 0);
      break;
    case PROP_LOOP_TYPE:
      g_value_set_enum (value, sample_info ?
			(sample_info->options
			 & IPATCH_DLS2_SAMPLE_LOOP_MASK)
			: IPATCH_SAMPLE_LOOP_NONE);
      break;
    case PROP_ROOT_NOTE:
      g_value_set_int (value, sample_info ? sample_info->root_note : 60);
      break;
    case PROP_FINE_TUNE:
      g_value_set_int (value, sample_info ? sample_info->fine_tune : 0);
      break;
    case PROP_GAIN:
      g_value_set_int (value, sample_info ? sample_info->gain : 0);
      break;
    case PROP_LOOP_START:
      g_value_set_uint (value, sample_info ? sample_info->loop_start : 0);
      break;
    case PROP_LOOP_END:
      g_value_set_uint (value, sample_info ? sample_info->loop_end : 0);
      break;
    default:
      return (FALSE);
    }

  return (TRUE);
}

/**
 * ipatch_dls2_sample_info_notify_changes: (skip)
 * @item: Item to send #IpatchItem property notifies on
 * @newinfo: New sample info values
 * @oldinfo: Old sample info values
 *
 * Sends #IpatchItem property notifies for changed sample info parameters.
 */
void
ipatch_dls2_sample_info_notify_changes (IpatchItem *item,
					IpatchDLS2SampleInfo *newinfo,
					IpatchDLS2SampleInfo *oldinfo)
{
  GParamSpec **found_pspec_cache = NULL;
  GObjectClass *klass;
  GValue newval = { 0 }, oldval = { 0 };
  GSList *p;

  g_return_if_fail (IPATCH_IS_ITEM (item));

  klass = G_OBJECT_GET_CLASS (item);

  /* search for param spec cache for object's class */
  for (p = info_pspec_list; p; p = p->next)
    {
      if (((ClassPropBag *)(p->data))->klass == klass)
	{
	  found_pspec_cache = ((ClassPropBag *)(p->data))->pspecs;
	  break;
	}
    }

  g_return_if_fail (found_pspec_cache);

  if ((oldinfo->options & IPATCH_DLS2_SAMPLE_LOOP_MASK)
      != (newinfo->options & IPATCH_DLS2_SAMPLE_LOOP_MASK))
    {
      g_value_init (&newval, IPATCH_TYPE_SAMPLE_LOOP_TYPE);
      g_value_init (&oldval, IPATCH_TYPE_SAMPLE_LOOP_TYPE);
      g_value_set_enum (&newval, newinfo->options & IPATCH_DLS2_SAMPLE_LOOP_MASK);
      g_value_set_enum (&oldval, oldinfo->options & IPATCH_DLS2_SAMPLE_LOOP_MASK);
      ipatch_item_prop_notify (item, found_pspec_cache[0], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if ((oldinfo->options & IPATCH_DLS2_SAMPLE_FLAGS_MASK)
      != (newinfo->options & IPATCH_DLS2_SAMPLE_FLAGS_MASK))
    {
      g_value_init (&newval, IPATCH_TYPE_DLS2_SAMPLE_FLAGS);
      g_value_init (&oldval, IPATCH_TYPE_DLS2_SAMPLE_FLAGS);
      g_value_set_flags (&newval, newinfo->options & IPATCH_DLS2_SAMPLE_FLAGS_MASK);
      g_value_set_flags (&oldval, oldinfo->options & IPATCH_DLS2_SAMPLE_FLAGS_MASK);
      ipatch_item_prop_notify (item, found_pspec_cache[1], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldinfo->root_note != newinfo->root_note)
    {
      g_value_init (&newval, G_TYPE_INT);
      g_value_init (&oldval, G_TYPE_INT);
      g_value_set_int (&newval, newinfo->root_note);
      g_value_set_int (&oldval, oldinfo->root_note);
      ipatch_item_prop_notify (item, found_pspec_cache[2], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldinfo->fine_tune != newinfo->fine_tune)
    {
      g_value_init (&newval, G_TYPE_INT);
      g_value_init (&oldval, G_TYPE_INT);
      g_value_set_int (&newval, newinfo->fine_tune);
      g_value_set_int (&oldval, oldinfo->fine_tune);
      ipatch_item_prop_notify (item, found_pspec_cache[3], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldinfo->gain != newinfo->gain)
    {
      g_value_init (&newval, G_TYPE_INT);
      g_value_init (&oldval, G_TYPE_INT);
      g_value_set_int (&newval, newinfo->gain);
      g_value_set_int (&oldval, oldinfo->gain);
      ipatch_item_prop_notify (item, found_pspec_cache[4], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldinfo->loop_start != newinfo->loop_start)
    {
      g_value_init (&newval, G_TYPE_UINT);
      g_value_init (&oldval, G_TYPE_UINT);
      g_value_set_uint (&newval, newinfo->loop_start);
      g_value_set_uint (&oldval, oldinfo->loop_start);
      ipatch_item_prop_notify (item, found_pspec_cache[5], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }

  if (oldinfo->loop_end != newinfo->loop_end)
    {
      g_value_init (&newval, G_TYPE_UINT);
      g_value_init (&oldval, G_TYPE_UINT);
      g_value_set_uint (&newval, newinfo->loop_end);
      g_value_set_uint (&oldval, oldinfo->loop_end);
      ipatch_item_prop_notify (item, found_pspec_cache[6], &newval, &oldval);
      g_value_unset (&newval);
      g_value_unset (&oldval);
    }
}
