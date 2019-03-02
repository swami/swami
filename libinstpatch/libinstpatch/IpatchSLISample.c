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
 * SECTION: IpatchSLISample
 * @short_description: Spectralis audio sample
 * @see_also: #IpatchSLI, #IpatchSLIZone
 *
 * Spectralis samples are children of #IpatchSLI objects and are referenced
 * by #IpatchSLIZone objects.  They define the audio which is synthesized.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSLISample.h"
#include "IpatchSample.h"
#include "IpatchSampleData.h"
#include "IpatchSLI.h"
#include "IpatchSLIFile.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "IpatchSF2VoiceCache.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"

/* properties */
enum {
  PROP_0,
  PROP_NAME,
  PROP_SAMPLE_SIZE,
  PROP_SAMPLE_FORMAT,
  PROP_SAMPLE_RATE,
  PROP_LOOP_TYPE,
  PROP_LOOP_START,
  PROP_LOOP_END,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE,
  PROP_SAMPLE_DATA,
};

static void ipatch_sli_sample_iface_init (IpatchSampleIface *sample_iface);
static gboolean ipatch_sli_sample_iface_open (IpatchSampleHandle *handle,
                                              GError **err);

static void ipatch_sli_sample_finalize (GObject *gobject);
static void ipatch_sli_sample_set_property (GObject *object,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_sli_sample_get_property (GObject *object,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void ipatch_sli_sample_item_copy (IpatchItem *dest, IpatchItem *src,
					 IpatchItemCopyLinkFunc link_func,
					 gpointer user_data);
static void ipatch_sli_sample_item_remove_full (IpatchItem *item, gboolean full);
static int
ipatch_sli_sample_voice_cache_update_handler (IpatchSF2VoiceCache *cache,
					      int *select_values,
					      GObject *cache_item,
					      GObject *item, GParamSpec *pspec,
					      const GValue *value,
					      IpatchSF2VoiceUpdate *updates,
					      guint max_updates);
static void ipatch_sli_sample_real_set_name (IpatchSLISample *sample,
                                             const char *name,
                                             gboolean name_notify);
static void ipatch_sli_sample_real_set_data (IpatchSLISample *sample,
                                             IpatchSampleData *sampledata,
                                             gboolean data_notify);

/* cached parameter spec values for speed */
static GParamSpec *name_pspec;
static GParamSpec *sample_data_pspec;

G_DEFINE_TYPE_WITH_CODE (IpatchSLISample, ipatch_sli_sample, IPATCH_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sli_sample_iface_init))

/* sample interface initialization */
static void
ipatch_sli_sample_iface_init (IpatchSampleIface *sample_iface)
{
  sample_iface->open = ipatch_sli_sample_iface_open;
}

static gboolean
ipatch_sli_sample_iface_open (IpatchSampleHandle *handle, GError **err)
{
  IpatchSLISample *sample = IPATCH_SLI_SAMPLE (handle->sample);
  g_return_val_if_fail (sample->sample_data != NULL, FALSE);
  return (ipatch_sample_handle_cascade_open
          (handle, (IpatchSample *)(sample->sample_data), err));
}

static void
ipatch_sli_sample_class_init (IpatchSLISampleClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  GParamSpec *pspec;

  obj_class->finalize = ipatch_sli_sample_finalize;
  obj_class->get_property = ipatch_sli_sample_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_sli_sample_set_property;
  item_class->copy = ipatch_sli_sample_item_copy;
  item_class->remove_full = ipatch_sli_sample_item_remove_full;

  /* "name" property is used as the title */
  g_object_class_override_property (obj_class, PROP_NAME, "title");

  name_pspec = g_param_spec_string ("name", _("Name"), _("Name"), NULL,
                                    G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE);
  ipatch_param_set (name_pspec,
		    "string-max-length", IPATCH_SLI_NAME_SIZE, /* max length */
		    NULL);
  g_object_class_install_property (obj_class, PROP_NAME, name_pspec);

  /* properties defined by IpatchSample interface */
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_SIZE,
                                           "sample-size");
  ipatch_sample_install_property_readonly (obj_class, PROP_SAMPLE_FORMAT,
                                           "sample-format");
  pspec = ipatch_sample_install_property (obj_class, PROP_SAMPLE_RATE,
                                           "sample-rate");
  pspec->flags |= IPATCH_PARAM_SYNTH;

  /* IpatchSLISample objects don't have a loop type field really */
  ipatch_sample_install_property_readonly (obj_class, PROP_LOOP_TYPE, "loop-type");

  pspec = ipatch_sample_install_property (obj_class, PROP_LOOP_START, "loop-start");
  pspec->flags |= IPATCH_PARAM_SYNTH | IPATCH_PARAM_SYNTH_REALTIME;

  pspec = ipatch_sample_install_property (obj_class, PROP_LOOP_END, "loop-end");
  pspec->flags |= IPATCH_PARAM_SYNTH | IPATCH_PARAM_SYNTH_REALTIME;

  pspec = ipatch_sample_install_property (obj_class, PROP_ROOT_NOTE, "root-note");
  pspec->flags |= IPATCH_PARAM_SYNTH;

  pspec = ipatch_sample_install_property (obj_class, PROP_FINE_TUNE, "fine-tune");
  pspec->flags |= IPATCH_PARAM_SYNTH | IPATCH_PARAM_SYNTH_REALTIME;

  sample_data_pspec = ipatch_sample_install_property (obj_class,
                                             PROP_SAMPLE_DATA, "sample-data");

  /* install IpatchSF2VoiceCache update handler for real time effects */
  ipatch_type_set (IPATCH_TYPE_SLI_SAMPLE, "sf2voice-update-func",
		   ipatch_sli_sample_voice_cache_update_handler, NULL);
}

static void
ipatch_sli_sample_init (IpatchSLISample *sample)
{
  ipatch_sli_sample_set_blank (sample);
  sample->rate = IPATCH_SAMPLE_RATE_DEFAULT;
}

static void
ipatch_sli_sample_finalize (GObject *gobject)
{
  IpatchSLISample *sample = IPATCH_SLI_SAMPLE (gobject);

  /* nothing should reference the sample after this, but we set
     pointers to NULL to help catch invalid references. Locking of
     sample is required since in reality all its children do
     still hold references */

  ipatch_sli_sample_real_set_data (sample, NULL, FALSE);

  IPATCH_ITEM_WLOCK (sample);

  g_free (sample->name);
  sample->name = NULL;

  IPATCH_ITEM_WUNLOCK (sample);

  if (G_OBJECT_CLASS (ipatch_sli_sample_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sli_sample_parent_class)->finalize (gobject);
}

static void
ipatch_sli_sample_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  IpatchSLISample *sample;

  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (object));
  sample = IPATCH_SLI_SAMPLE (object);

  switch (property_id)
  {
    case PROP_NAME:
      ipatch_sli_sample_real_set_name (sample, g_value_get_string (value),
				       FALSE);	/* don't do name notify */
      break;
    case PROP_SAMPLE_RATE:
      IPATCH_ITEM_WLOCK (sample);
      sample->rate = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_LOOP_START:
      IPATCH_ITEM_WLOCK (sample);
      sample->loop_start = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_LOOP_END:
      IPATCH_ITEM_WLOCK (sample);
      sample->loop_end = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_ROOT_NOTE:
      IPATCH_ITEM_WLOCK (sample);
      sample->root_note = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_FINE_TUNE:
      IPATCH_ITEM_WLOCK (sample);
      sample->fine_tune = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (sample);
      break;
    case PROP_SAMPLE_DATA:
      ipatch_sli_sample_real_set_data (sample, (IpatchSampleData *)
				       g_value_get_object (value), FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sli_sample_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchSLISample *sample;

  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (object));
  sample = IPATCH_SLI_SAMPLE (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_take_string (value, ipatch_sli_sample_get_name (sample));
      break;
    case PROP_SAMPLE_SIZE:
      g_return_if_fail (sample->sample_data != NULL);
      g_value_set_uint (value, ipatch_sample_get_size ((IpatchSample *)(sample->sample_data), NULL));
      break;
    case PROP_SAMPLE_FORMAT:
      g_return_if_fail (sample->sample_data != NULL);
      g_value_set_int (value, ipatch_sample_get_format ((IpatchSample *)(sample->sample_data)));
      break;
    case PROP_SAMPLE_RATE:
      IPATCH_ITEM_RLOCK (sample);
      g_value_set_int (value, sample->rate);
      IPATCH_ITEM_RUNLOCK (sample);
      break;
    case PROP_LOOP_TYPE:  /* IpatchSLISample objects don't have loop type, just use normal loop */
      g_value_set_enum (value, IPATCH_SAMPLE_LOOP_STANDARD);
      break;
    case PROP_LOOP_START:
      IPATCH_ITEM_RLOCK (sample);
      g_value_set_uint (value, sample->loop_start);
      IPATCH_ITEM_RUNLOCK (sample);
      break;
    case PROP_LOOP_END:
      IPATCH_ITEM_RLOCK (sample);
      g_value_set_uint (value, sample->loop_end);
      IPATCH_ITEM_RUNLOCK (sample);
      break;
    case PROP_ROOT_NOTE:
      g_value_set_int (value, sample->root_note);
      break;
    case PROP_FINE_TUNE:
      g_value_set_int (value, sample->fine_tune);
      break;
    case PROP_SAMPLE_DATA:
      g_value_take_object (value, ipatch_sli_sample_get_data (sample));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sli_sample_item_copy (IpatchItem *dest, IpatchItem *src,
			     IpatchItemCopyLinkFunc link_func,
			     gpointer user_data)
{
  IpatchSLISample *src_sam, *dest_sam;

  src_sam = IPATCH_SLI_SAMPLE (src);
  dest_sam = IPATCH_SLI_SAMPLE (dest);

  IPATCH_ITEM_RLOCK (src_sam);

  ipatch_sli_sample_set_data (dest_sam, src_sam->sample_data);

  dest_sam->name = g_strdup (src_sam->name);
  dest_sam->rate = src_sam->rate;
  dest_sam->loop_start = src_sam->loop_start;
  dest_sam->loop_end = src_sam->loop_end;
  dest_sam->root_note = src_sam->root_note;
  dest_sam->fine_tune = src_sam->fine_tune;

  IPATCH_ITEM_RUNLOCK (src_sam);
}

static void
ipatch_sli_sample_item_remove_full (IpatchItem *item, gboolean full)
{
  IpatchList *list;
  IpatchItem *zone, *temp;
  IpatchIter iter;

  /* ++ ref zone list */
  list = ipatch_sli_get_zone_references (IPATCH_SLI_SAMPLE (item));
  ipatch_list_init_iter (list, &iter);
  zone = ipatch_item_first (&iter);

  while (zone)
  {
    temp = zone;
    zone = ipatch_item_next (&iter);
    ipatch_item_remove (temp);
  }

  g_object_unref (list);	/* -- unref list */

  if (full)
    ipatch_sli_sample_set_data (IPATCH_SLI_SAMPLE (item), NULL);

  if (IPATCH_ITEM_CLASS (ipatch_sli_sample_parent_class)->remove_full)
    IPATCH_ITEM_CLASS (ipatch_sli_sample_parent_class)->remove_full (item, full);
}

/* IpatchSF2VoiceCache update function for realtime effects */
static int
ipatch_sli_sample_voice_cache_update_handler (IpatchSF2VoiceCache *cache,
					      int *select_values,
					      GObject *cache_item,
					      GObject *item, GParamSpec *pspec,
					      const GValue *value,
					      IpatchSF2VoiceUpdate *updates,
					      guint max_updates)
{
  IpatchSF2Voice *voice;
  guint8 genid, genid2 = 255;
  gint16 val = 0, val2 = 0;
  int ival;

  g_return_val_if_fail (cache->voices->len > 0, 0);

  voice = IPATCH_SF2_VOICE_CACHE_GET_VOICE (cache, 0);

  switch (IPATCH_PARAM_SPEC_ID (pspec))
  {
    case PROP_LOOP_START:
      genid = IPATCH_SF2_GEN_SAMPLE_LOOP_START;
      genid2 = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START;
      ival = (int)g_value_get_uint (value) - voice->loop_start;
      val = ival % 32768;
      val2 = ival / 32768;
      break;
    case PROP_LOOP_END:
      genid = IPATCH_SF2_GEN_SAMPLE_LOOP_END;
      genid2 = IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END;
      ival = (int)g_value_get_uint (value) - voice->loop_end;
      val = ival % 32768;
      val2 = ival / 32768;
      break;
    case PROP_FINE_TUNE:
      genid = IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE;
      ival = g_value_get_int (value);
      break;
    default:
      return (0);
  }

  updates->voice = 0;
  updates->genid = genid;
  updates->ival = val;

  if (genid2 != 255 && max_updates >= 2)
  {
    updates[1].voice = 0;
    updates[1].genid = genid2;
    updates[1].ival = val2;
    return (2);
  }
  else return (1);
}

/**
 * ipatch_sli_sample_new:
 *
 * Create a new Spectralis sample object.
 *
 * Returns: New Spectralis sample with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSLISample *
ipatch_sli_sample_new (void)
{
  return (IPATCH_SLI_SAMPLE (g_object_new (IPATCH_TYPE_SLI_SAMPLE, NULL)));
}

/**
 * ipatch_sli_sample_first: (skip)
 * @iter: Patch item iterator containing #IpatchSLISample items
 *
 * Gets the first item in a sample iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first sample in @iter or %NULL if empty.
 */
IpatchSLISample *
ipatch_sli_sample_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SLI_SAMPLE (obj));
  else return (NULL);
}

/**
 * ipatch_sli_sample_next: (skip)
 * @iter: Patch item iterator containing #IpatchSLISample items
 *
 * Gets the next item in a sample iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next sample in @iter or %NULL if at the end of the list.
 */
IpatchSLISample *
ipatch_sli_sample_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SLI_SAMPLE (obj));
  else return (NULL);
}

/**
 * ipatch_sli_sample_set_name:
 * @sample: Sample to set name of
 * @name: (nullable): Value to set name to
 *
 * Sets the name of a Spectralis sample.
 */
void
ipatch_sli_sample_set_name (IpatchSLISample *sample, const char *name)
{
  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (sample));
  ipatch_sli_sample_real_set_name (sample, name, TRUE);
}

/* also called from item_set_property method so "name_notify" can be used to
   stop double emission of name notify */
static void
ipatch_sli_sample_real_set_name (IpatchSLISample *sample, const char *name,
				 gboolean name_notify)
{
  GValue oldval = { 0 }, newval = { 0 };
  char *newname;

  g_value_init (&oldval, G_TYPE_STRING);

  newname = g_strdup (name);

  IPATCH_ITEM_WLOCK (sample);
  g_value_take_string (&oldval, sample->name);
  sample->name = newname;
  IPATCH_ITEM_WUNLOCK (sample);

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, name);

  ipatch_item_prop_notify ((IpatchItem *)sample, ipatch_item_pspec_title,
			   &newval, &oldval);

  if (name_notify)
    ipatch_item_prop_notify ((IpatchItem *)sample, name_pspec, &newval, &oldval);

  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_sli_sample_get_name:
 * @sample: Sample to get name of
 *
 * Gets the name of a Spectralis sample.
 *
 * Returns: Name of sample or %NULL if not set. String value should be freed
 * when finished with it.
 */
char *
ipatch_sli_sample_get_name (IpatchSLISample *sample)
{
  char *name = NULL;

  g_return_val_if_fail (IPATCH_IS_SLI_SAMPLE (sample), NULL);

  IPATCH_ITEM_RLOCK (sample);
  if (sample->name) name = g_strdup (sample->name);
  IPATCH_ITEM_RUNLOCK (sample);

  return (name);
}

/**
 * ipatch_sli_sample_set_data:
 * @sample: Sample to set sample data of
 * @sampledata: Sample data to set sample to
 *
 * Set a sample's sample data object.
 */
void
ipatch_sli_sample_set_data (IpatchSLISample *sample,
			    IpatchSampleData *sampledata)
{
  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (sample));
  g_return_if_fail (IPATCH_IS_SAMPLE_DATA (sampledata));

  ipatch_sli_sample_real_set_data (sample, sampledata, TRUE);
}

/* the actual setting of sample data, user routine does a g_object_notify */
static void
ipatch_sli_sample_real_set_data (IpatchSLISample *sample,
				 IpatchSampleData *sampledata,
				 gboolean data_notify)
{
  GValue oldval = { 0 }, newval = { 0 };
  IpatchSampleData *old_sampledata;

  if (sampledata)
  {
      g_object_ref (sampledata);
      ipatch_sample_data_used (sampledata);   /* ++ inc use count */
  }

  IPATCH_ITEM_WLOCK (sample);
  old_sampledata = sample->sample_data;
  sample->sample_data = sampledata;
  IPATCH_ITEM_WUNLOCK (sample);

  if (old_sampledata)
    ipatch_sample_data_unused (old_sampledata);     // -- dec use count

  if (data_notify)
  {
    g_value_init (&newval, IPATCH_TYPE_SAMPLE_DATA);
    g_value_set_object (&newval, sampledata);

    g_value_init (&oldval, IPATCH_TYPE_SAMPLE_DATA);
    g_value_take_object (&oldval, old_sampledata);

    ipatch_item_prop_notify ((IpatchItem *)sample, sample_data_pspec, &newval, &oldval);
    g_value_unset (&newval);
    g_value_unset (&oldval);
  }
  else if (old_sampledata)
    g_object_unref (old_sampledata);
}

/**
 * ipatch_sli_sample_get_data:
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
ipatch_sli_sample_get_data (IpatchSLISample *sample)
{
  IpatchSampleData *sampledata;

  g_return_val_if_fail (IPATCH_IS_SLI_SAMPLE (sample), NULL);

  IPATCH_ITEM_RLOCK (sample);
  sampledata = sample->sample_data;
  if (sampledata) g_object_ref (sampledata); /* ++ ref sampledata */
  IPATCH_ITEM_RUNLOCK (sample);

  return (sampledata); /* !! ref passed to caller */
}

/**
 * ipatch_sli_sample_peek_data: (skip)
 * @sample: Sample to get sample data from
 *
 * Get the #IpatchSampleData item of a sample. Like
 * ipatch_sli_sample_get_data() but sample data object is not referenced.
 * This function should only be used if a reference of the sample data object
 * is ensured or only the pointer value is of importance.
 *
 * Returns: Sample data object of sample or %NULL if none. Remember that a
 * reference is NOT added.
 */
IpatchSampleData *
ipatch_sli_sample_peek_data (IpatchSLISample *sample)
{
  IpatchSampleData *sampledata;

  g_return_val_if_fail (IPATCH_IS_SLI_SAMPLE (sample), NULL);

  IPATCH_ITEM_RLOCK (sample);
  sampledata = sample->sample_data;
  IPATCH_ITEM_RUNLOCK (sample);

  return (sampledata);
}

/**
 * ipatch_sli_sample_set_blank:
 * @sample: Sample to set to blank sample data
 *
 * Set the sample data of a sample item to blank data.
 */
void
ipatch_sli_sample_set_blank (IpatchSLISample *sample)
{
  IpatchSampleData *sampledata;

  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (sample));

  sampledata = ipatch_sample_data_get_blank ();
  ipatch_item_set_atomic (sample,
			  "sample-data", sampledata,
			  "loop-start", 8,
			  "loop-end", 40,
			  "root-note", 60,
			  "fine-tune", 0,
			  NULL);
  g_object_unref (sampledata);
}
