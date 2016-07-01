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
 * SECTION: IpatchSLIInst
 * @short_description: Spectralis instrument object
 * @see_also: #IpatchSLI, #IpatchSLIZone
 *
 * Spectralis instruments are children of #IpatchSLI objects and are referenced
 * by #IpatchSLIZone objects.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSLIInst.h"
#include "IpatchSLIZone.h"
#include "IpatchSLI.h"
#include "IpatchSLIFile.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

/* properties */
enum {
  PROP_0,
  PROP_NAME,
  PROP_SOUND_ID,
  PROP_CATEGORY
};

static void ipatch_sli_inst_class_init (IpatchSLIInstClass *klass);
static void ipatch_sli_inst_init (IpatchSLIInst *inst);
static void ipatch_sli_inst_finalize (GObject *gobject);
static void ipatch_sli_inst_set_property (GObject *object,
					  guint property_id,
					  const GValue *value,
					  GParamSpec *pspec);
static void ipatch_sli_inst_get_property (GObject *object,
					  guint property_id,
					  GValue *value,
					  GParamSpec *pspec);
static void ipatch_sli_inst_item_copy (IpatchItem *dest, IpatchItem *src,
				       IpatchItemCopyLinkFunc link_func,
				       gpointer user_data);

static const GType *ipatch_sli_inst_container_child_types (void);
static gboolean ipatch_sli_inst_container_init_iter (IpatchContainer *container,
                                                  IpatchIter *iter, GType type);

static void ipatch_sli_inst_real_set_name (IpatchSLIInst *inst,
                                           const char *name,
                                           gboolean name_notify);

static gpointer parent_class = NULL;
static GType inst_child_types[2] = { 0 };
static GParamSpec *name_pspec;


GType
ipatch_sli_inst_get_type (void)
{
  static GType item_type = 0;

  if (!item_type)
  {
    static const GTypeInfo item_info = {
      sizeof (IpatchSLIInstClass), NULL, NULL,
      (GClassInitFunc)ipatch_sli_inst_class_init, NULL, NULL,
      sizeof (IpatchSLIInst), 0,
      (GInstanceInitFunc)ipatch_sli_inst_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					"IpatchSLIInst", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_sli_inst_class_init (IpatchSLIInstClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_ref (IPATCH_TYPE_CONTAINER);

  obj_class->finalize = ipatch_sli_inst_finalize;
  obj_class->get_property = ipatch_sli_inst_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_sli_inst_set_property;
  item_class->copy = ipatch_sli_inst_item_copy;

  container_class->child_types = ipatch_sli_inst_container_child_types;
  container_class->init_iter = ipatch_sli_inst_container_init_iter;

  g_object_class_override_property (obj_class, PROP_NAME, "title");

  name_pspec = ipatch_param_set (g_param_spec_string ("name", "Name", "Name",
	  NULL, G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE),
	  "string-max-length", IPATCH_SLI_NAME_SIZE, /* max string length */
	  NULL);
  g_object_class_install_property (obj_class, PROP_NAME, name_pspec);

  g_object_class_install_property (obj_class, PROP_SOUND_ID,
          g_param_spec_uint ("sound-id", "SoundID", "SoundID", 0, G_MAXUINT,
                             0, G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, PROP_CATEGORY,
          g_param_spec_uint ("category", "Category", "Category", 0, G_MAXUINT,
                             0, G_PARAM_READWRITE));

  inst_child_types[0] = IPATCH_TYPE_SLI_ZONE;
}

static void
ipatch_sli_inst_init (IpatchSLIInst *inst)
{
  inst->name = NULL;
  inst->zones = NULL;
  inst->sound_id = 0;
  inst->category = 0x4040;
}

static void
ipatch_sli_inst_finalize (GObject *gobject)
{
  IpatchSLIInst *inst = IPATCH_SLI_INST (gobject);

  IPATCH_ITEM_WLOCK (inst);

  g_free (inst->name);
  inst->name = NULL;

  IPATCH_ITEM_WUNLOCK (inst);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_sli_inst_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  IpatchSLIInst *inst = IPATCH_SLI_INST (object);

  switch (property_id)
    {
    case PROP_NAME:
      ipatch_sli_inst_real_set_name (inst, g_value_get_string (value), FALSE);
      break;
    case PROP_SOUND_ID:
      IPATCH_ITEM_WLOCK (inst);
      inst->sound_id = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (inst);
      break;
    case PROP_CATEGORY:
      IPATCH_ITEM_WLOCK (inst);
      inst->category = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (inst);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sli_inst_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  IpatchSLIInst *inst = IPATCH_SLI_INST (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_take_string (value, ipatch_sli_inst_get_name (inst));
      break;
    case PROP_SOUND_ID:
      g_value_set_uint (value, inst->sound_id);
      break;
    case PROP_CATEGORY:
      g_value_set_uint (value, inst->category);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sli_inst_item_copy (IpatchItem *dest, IpatchItem *src,
			   IpatchItemCopyLinkFunc link_func,
			   gpointer user_data)
{
  IpatchSLIInst *src_inst, *dest_inst;
  IpatchItem *zitem;
  GSList *p;

  src_inst = IPATCH_SLI_INST (src);
  dest_inst = IPATCH_SLI_INST (dest);

  IPATCH_ITEM_RLOCK (src_inst);

  dest_inst->name = g_strdup (src_inst->name);
  dest_inst->sound_id = src_inst->sound_id;
  dest_inst->category = src_inst->category;

  p = src_inst->zones;
  while (p)			/* duplicate zones */
  { /* ++ ref new instrument zone, !! zone list takes it over */
    zitem = ipatch_item_duplicate_link_func (IPATCH_ITEM (p->data),
                                             link_func, user_data);
    dest_inst->zones = g_slist_prepend (dest_inst->zones, zitem);
    ipatch_item_set_parent (zitem, IPATCH_ITEM (dest_inst));

    p = g_slist_next (p);
  }

  IPATCH_ITEM_RUNLOCK (src_inst);

  dest_inst->zones = g_slist_reverse (dest_inst->zones);
}

static const GType *
ipatch_sli_inst_container_child_types (void)
{
  return (inst_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_sli_inst_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type)
{
  IpatchSLIInst *inst = IPATCH_SLI_INST (container);

  if (!g_type_is_a (type, IPATCH_TYPE_SLI_ZONE))
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &inst->zones);
  return (TRUE);
}

/**
 * ipatch_sli_inst_new:
 *
 * Create a new Spectralis instrument object.
 *
 * Returns: New Spectralis instrument with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSLIInst *
ipatch_sli_inst_new (void)
{
  return (IPATCH_SLI_INST (g_object_new (IPATCH_TYPE_SLI_INST, NULL)));
}

/**
 * ipatch_sli_inst_first: (skip)
 * @iter: Patch item iterator containing #IpatchSLIInst items
 *
 * Gets the first item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first instrument in @iter or %NULL if empty.
 */
IpatchSLIInst *
ipatch_sli_inst_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SLI_INST (obj));
  else return (NULL);
}

/**
 * ipatch_sli_inst_next: (skip)
 * @iter: Patch item iterator containing #IpatchSLIInst items
 *
 * Gets the next item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next instrument in @iter or %NULL if at the end of the list.
 */
IpatchSLIInst *
ipatch_sli_inst_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SLI_INST (obj));
  else return (NULL);
}

/**
 * ipatch_sli_inst_new_zone:
 * @inst: Spectralis instrument
 * @sample: Referenced sample for new zone
 *
 * A convenience function for quickly creating a new instrument zone, adding it
 * to @inst and setting the zone's referenced sample to @sample.
 */
void
ipatch_sli_inst_new_zone (IpatchSLIInst *inst, IpatchSLISample *sample)
{
  IpatchSLIZone *zone;

  g_return_if_fail (IPATCH_IS_SLI_INST (inst));
  g_return_if_fail (IPATCH_IS_SLI_SAMPLE (sample));

  zone = ipatch_sli_zone_new (); /* ++ ref new zone */
  ipatch_sli_zone_set_sample (zone, sample);

  ipatch_container_append (IPATCH_CONTAINER (inst), IPATCH_ITEM (zone));

  g_object_unref (zone);	/* -- unref zone */
}

/**
 * ipatch_sli_inst_set_name:
 * @inst: Instrument to set name of
 * @name: (nullable): Value to set name to
 *
 * Sets the name of a Spectralis instrument.
 */
void
ipatch_sli_inst_set_name (IpatchSLIInst *inst, const char *name)
{
  g_return_if_fail (IPATCH_IS_SLI_INST (inst));

  ipatch_sli_inst_real_set_name (inst, name, TRUE);
}

/* the real inst name set routine */
static void
ipatch_sli_inst_real_set_name (IpatchSLIInst *inst, const char *name,
			       gboolean name_notify)
{
  GValue oldval = { 0 }, newval = { 0 };
  char *newname, *oldname;

  newname = g_strdup (name);

  IPATCH_ITEM_WLOCK (inst);
  oldname = inst->name;
  inst->name = newname;
  IPATCH_ITEM_WUNLOCK (inst);

  g_value_init (&oldval, G_TYPE_STRING);
  g_value_take_string (&oldval, oldname);

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, name);

  if (name_notify)
    ipatch_item_prop_notify ((IpatchItem *)inst, name_pspec, &newval, &oldval);

  ipatch_item_prop_notify ((IpatchItem *)inst, ipatch_item_pspec_title,
			   &newval, &oldval);

  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_sli_inst_get_name:
 * @inst: Instrument to get name of
 *
 * Gets the name of a Spectralis instrument.
 *
 * Returns: Name of instrument or %NULL if not set. String value should be
 * freed when finished with it.
 */
char *
ipatch_sli_inst_get_name (IpatchSLIInst *inst)
{
  char *name = NULL;

  g_return_val_if_fail (IPATCH_IS_SLI_INST (inst), NULL);

  IPATCH_ITEM_RLOCK (inst);
  if (inst->name) name = g_strdup (inst->name);
  IPATCH_ITEM_RUNLOCK (inst);

  return (name);
}

/**
 * ipatch_sli_inst_get_category_as_path:
 * @inst: Instrument to get category of
 *
 * Gets the category of a Spectralis instrument as a string of colon separated
 * indexes into ipatch_sli_inst_cat_map[].
 *
 * Returns: Category of instrument or %NULL if not set. String value should be
 * g_freed when finished with it.
 */
char *
ipatch_sli_inst_get_category_as_path (IpatchSLIInst *inst)
{
  guint cat, i;
  GString *catstr;
  const IpatchSLIInstCatMapEntry *catmap = ipatch_sli_inst_cat_map;

  g_return_val_if_fail (IPATCH_IS_SLI_INST (inst), NULL);
  g_return_val_if_fail (inst->category != 0, NULL);

  catstr = g_string_sized_new (6); /* ++ new str */
  for(cat = GUINT16_SWAP_LE_BE(inst->category); cat; cat >>= 8)
  {
    if (cat == 0x40) /* Other subcats are treated as no subcat for UI */
      break;
    if (catstr->str[0] != '\0')
      g_string_append_c (catstr, ':');
    for (i=0; catmap[i].code != '@'; i++)
      if ((cat & 0xff) == catmap[i].code)
        break; /* from inner loop */
    g_string_append_printf (catstr, "%d", i);
    if (catmap[i].submap == NULL)
      break;
    else
      catmap = catmap[i].submap;
  }
  return g_string_free (catstr, FALSE); /* !! ref to str passed to caller */
}

