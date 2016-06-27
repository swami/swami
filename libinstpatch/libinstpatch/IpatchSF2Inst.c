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
 * SECTION: IpatchSF2Inst
 * @short_description: SoundFont instrument object
 * @see_also: #IpatchSF2, #IpatchSF2PZone
 * @stability: Stable
 *
 * SoundFont instruments are children of #IpatchSF2 objects and are referenced
 * by #IpatchSF2PZone objects.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2Inst.h"
#include "IpatchSF2IZone.h"
#include "IpatchSF2GenItem.h"
#include "IpatchSF2ModItem.h"
#include "IpatchSF2.h"
#include "IpatchSF2File.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

/* properties */
enum {
  /* generator IDs are used for lower numbers */
  PROP_NAME = IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID,
  PROP_MODULATORS
};

static void ipatch_sf2_inst_class_init (IpatchSF2InstClass *klass);
static void
ipatch_sf2_inst_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface);
static void
ipatch_sf2_inst_mod_item_iface_init (IpatchSF2ModItemIface *moditem_iface);
static void ipatch_sf2_inst_init (IpatchSF2Inst *inst);
static void ipatch_sf2_inst_finalize (GObject *gobject);
static void ipatch_sf2_inst_set_property (GObject *object,
					  guint property_id,
					  const GValue *value,
					  GParamSpec *pspec);
static void ipatch_sf2_inst_get_property (GObject *object,
					  guint property_id,
					  GValue *value,
					  GParamSpec *pspec);
static void ipatch_sf2_inst_item_copy (IpatchItem *dest, IpatchItem *src,
				       IpatchItemCopyLinkFunc link_func,
				       gpointer user_data);
static void ipatch_sf2_inst_item_remove_full (IpatchItem *item, gboolean full);

static const GType *ipatch_sf2_inst_container_child_types (void);
static gboolean
ipatch_sf2_inst_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type);

static void ipatch_sf2_inst_real_set_name (IpatchSF2Inst *inst, const char *name,
					   gboolean name_notify);

static gpointer parent_class = NULL;
static GType inst_child_types[2] = { 0 };
static GParamSpec *name_pspec;

/* For passing data from class init to gen item interface init */
static GParamSpec **gen_item_specs = NULL;
static GParamSpec **gen_item_setspecs = NULL;

/* For passing between class init and mod item interface init */
static GParamSpec *modulators_spec = NULL;


GType
ipatch_sf2_inst_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchSF2InstClass), NULL, NULL,
      (GClassInitFunc)ipatch_sf2_inst_class_init, NULL, NULL,
      sizeof (IpatchSF2Inst), 0,
      (GInstanceInitFunc)ipatch_sf2_inst_init,
    };
    static const GInterfaceInfo genitem_iface = {
      (GInterfaceInitFunc) ipatch_sf2_inst_gen_item_iface_init, NULL, NULL };
    static const GInterfaceInfo moditem_iface = {
      (GInterfaceInitFunc) ipatch_sf2_inst_mod_item_iface_init, NULL, NULL };

    item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					"IpatchSF2Inst", &item_info, 0);
    g_type_add_interface_static (item_type, IPATCH_TYPE_SF2_GEN_ITEM, &genitem_iface);
    g_type_add_interface_static (item_type, IPATCH_TYPE_SF2_MOD_ITEM, &moditem_iface);
  }

  return (item_type);
}

static void
ipatch_sf2_inst_class_init (IpatchSF2InstClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_ref (IPATCH_TYPE_CONTAINER);

  obj_class->finalize = ipatch_sf2_inst_finalize;
  obj_class->get_property = ipatch_sf2_inst_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_sf2_inst_set_property;
  item_class->copy = ipatch_sf2_inst_item_copy;
  item_class->remove_full = ipatch_sf2_inst_item_remove_full;

  container_class->child_types = ipatch_sf2_inst_container_child_types;
  container_class->init_iter = ipatch_sf2_inst_container_init_iter;

  g_object_class_override_property (obj_class, PROP_NAME, "title");

  name_pspec = ipatch_param_set (g_param_spec_string ("name", "Name", "Name",
	  NULL, G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE),
	  "string-max-length", IPATCH_SFONT_NAME_SIZE, /* max string length */
	  NULL);
  g_object_class_install_property (obj_class, PROP_NAME, name_pspec);

  g_object_class_override_property (obj_class, PROP_MODULATORS, "modulators");
  modulators_spec = g_object_class_find_property (obj_class, "modulators");

  inst_child_types[0] = IPATCH_TYPE_SF2_IZONE;

  /* install generator properties */
  ipatch_sf2_gen_item_iface_install_properties (obj_class,
                                                IPATCH_SF2_GEN_PROPS_INST_GLOBAL,
                                                &gen_item_specs, &gen_item_setspecs);
}

/* gen item interface initialization */
static void
ipatch_sf2_inst_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface)
{
  genitem_iface->genarray_ofs = G_STRUCT_OFFSET (IpatchSF2Inst, genarray);
  genitem_iface->propstype = IPATCH_SF2_GEN_PROPS_INST_GLOBAL;

  g_return_if_fail (gen_item_specs != NULL);
  g_return_if_fail (gen_item_setspecs != NULL);

  memcpy (&genitem_iface->specs, gen_item_specs, sizeof (genitem_iface->specs));
  memcpy (&genitem_iface->setspecs, gen_item_setspecs, sizeof (genitem_iface->setspecs));
  g_free (gen_item_specs);
  g_free (gen_item_setspecs);
}

/* mod item interface initialization */
static void
ipatch_sf2_inst_mod_item_iface_init (IpatchSF2ModItemIface *moditem_iface)
{
  moditem_iface->modlist_ofs = G_STRUCT_OFFSET (IpatchSF2Inst, mods);

  /* cache the modulators property for fast notifications */
  moditem_iface->mod_pspec = modulators_spec;
}

static void
ipatch_sf2_inst_init (IpatchSF2Inst *inst)
{
  inst->name = NULL;
  inst->zones = NULL;
  ipatch_sf2_gen_array_init (&inst->genarray, FALSE, FALSE);
}

static void
ipatch_sf2_inst_finalize (GObject *gobject)
{
  IpatchSF2Inst *inst = IPATCH_SF2_INST (gobject);

  IPATCH_ITEM_WLOCK (inst);

  g_free (inst->name);
  inst->name = NULL;

  ipatch_sf2_mod_list_free (inst->mods, TRUE);
  inst->mods = NULL;

  IPATCH_ITEM_WUNLOCK (inst);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_sf2_inst_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  IpatchSF2Inst *inst = IPATCH_SF2_INST (object);
  IpatchSF2ModList *list;

  /* generator property? */
  if (ipatch_sf2_gen_item_iface_set_property ((IpatchSF2GenItem *)inst,
					      property_id, value))
    return;

  switch (property_id)
    {
    case PROP_NAME:
      ipatch_sf2_inst_real_set_name (inst, g_value_get_string (value), FALSE);
      break;
    case PROP_MODULATORS:
      list = (IpatchSF2ModList *)g_value_get_boxed (value);
      ipatch_sf2_mod_item_set_mods (IPATCH_SF2_MOD_ITEM (inst), list,
				    IPATCH_SF2_MOD_NO_NOTIFY);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
ipatch_sf2_inst_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  IpatchSF2Inst *inst = IPATCH_SF2_INST (object);
  IpatchSF2ModList *list;

  /* generator property? */
  if (ipatch_sf2_gen_item_iface_get_property ((IpatchSF2GenItem *)inst,
					      property_id, value))
    return;

  switch (property_id)
    {
    case PROP_NAME:
      g_value_take_string (value, ipatch_sf2_inst_get_name (inst));
      break;
    case PROP_MODULATORS:
      list = ipatch_sf2_mod_item_get_mods (IPATCH_SF2_MOD_ITEM (inst));
      g_value_take_boxed (value, list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sf2_inst_item_copy (IpatchItem *dest, IpatchItem *src,
			   IpatchItemCopyLinkFunc link_func,
			   gpointer user_data)
{
  IpatchSF2Inst *src_inst, *dest_inst;
  IpatchItem *zitem;
  GSList *p;

  src_inst = IPATCH_SF2_INST (src);
  dest_inst = IPATCH_SF2_INST (dest);

  IPATCH_ITEM_RLOCK (src_inst);

  dest_inst->name = g_strdup (src_inst->name);

  dest_inst->genarray = src_inst->genarray;
  dest_inst->mods = ipatch_sf2_mod_list_duplicate (src_inst->mods);

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

static void
ipatch_sf2_inst_item_remove_full (IpatchItem *item, gboolean full)
{
  IpatchItem *zitem, *temp;
  IpatchList *list;
  IpatchIter iter;

  list = ipatch_sf2_get_zone_references (item);	/* ++ ref zone list */

  ipatch_list_init_iter (list, &iter);
  zitem = ipatch_item_first (&iter);

  while (zitem)
  {
    temp = zitem;
    zitem = ipatch_item_next (&iter);
    ipatch_item_remove (temp);
  }

  g_object_unref (list);	/* -- unref list */

  if (IPATCH_ITEM_CLASS (parent_class)->remove_full)
    IPATCH_ITEM_CLASS (parent_class)->remove_full (item, full);
}

static const GType *
ipatch_sf2_inst_container_child_types (void)
{
  return (inst_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_sf2_inst_container_init_iter (IpatchContainer *container,
				     IpatchIter *iter, GType type)
{
  IpatchSF2Inst *inst = IPATCH_SF2_INST (container);

  if (!g_type_is_a (type, IPATCH_TYPE_SF2_IZONE))
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &inst->zones);
  return (TRUE);
}

/**
 * ipatch_sf2_inst_new:
 *
 * Create a new SoundFont instrument object.
 *
 * Returns: New SoundFont instrument with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSF2Inst *
ipatch_sf2_inst_new (void)
{
  return (IPATCH_SF2_INST (g_object_new (IPATCH_TYPE_SF2_INST, NULL)));
}

/**
 * ipatch_sf2_inst_first: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Inst items
 *
 * Gets the first item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first instrument in @iter or %NULL if empty.
 */
IpatchSF2Inst *
ipatch_sf2_inst_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SF2_INST (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_inst_next: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Inst items
 *
 * Gets the next item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next instrument in @iter or %NULL if at the end of the list.
 */
IpatchSF2Inst *
ipatch_sf2_inst_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SF2_INST (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_inst_new_zone:
 * @inst: SoundFont instrument
 * @sample: Referenced sample for new zone
 *
 * A convenience function for quickly creating a new instrument zone, adding it
 * to @inst and setting the zone's referenced sample to @sample.
 */
void
ipatch_sf2_inst_new_zone (IpatchSF2Inst *inst, IpatchSF2Sample *sample)
{
  IpatchSF2IZone *izone;

  g_return_if_fail (IPATCH_IS_SF2_INST (inst));
  g_return_if_fail (IPATCH_IS_SF2_SAMPLE (sample));

  izone = ipatch_sf2_izone_new (); /* ++ ref new zone */
  ipatch_sf2_zone_set_link_item (IPATCH_SF2_ZONE (izone), IPATCH_ITEM (sample));

  ipatch_container_append (IPATCH_CONTAINER (inst), IPATCH_ITEM (izone));

  g_object_unref (izone);	/* -- unref zone */
}

/**
 * ipatch_sf2_inst_set_name:
 * @inst: Instrument to set name of
 * @name: Value to set name to
 *
 * Sets the name of a SoundFont instrument.
 */
void
ipatch_sf2_inst_set_name (IpatchSF2Inst *inst, const char *name)
{
  g_return_if_fail (IPATCH_IS_SF2_INST (inst));

  ipatch_sf2_inst_real_set_name (inst, name, TRUE);
}

/* the real inst name set routine */
static void
ipatch_sf2_inst_real_set_name (IpatchSF2Inst *inst, const char *name,
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
 * ipatch_sf2_inst_get_name:
 * @inst: Instrument to get name of
 *
 * Gets the name of a SoundFont instrument.
 *
 * Returns: Name of instrument or %NULL if not set. String value should be
 * freed when finished with it.
 */
char *
ipatch_sf2_inst_get_name (IpatchSF2Inst *inst)
{
  char *name = NULL;

  g_return_val_if_fail (IPATCH_IS_SF2_INST (inst), NULL);

  IPATCH_ITEM_RLOCK (inst);
  if (inst->name) name = g_strdup (inst->name);
  IPATCH_ITEM_RUNLOCK (inst);

  return (name);
}
