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
 * SECTION: IpatchSF2Preset
 * @short_description: SoundFont preset object
 * @see_also: #IpatchSF2
 * @stability: Stable
 *
 * SoundFont presets are children of #IpatchSF2 objects and define individual
 * instruments mapped to MIDI bank/program numbers.
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2Preset.h"
#include "IpatchSF2PZone.h"
#include "IpatchSF2File.h"
#include "IpatchSF2GenItem.h"
#include "IpatchSF2ModItem.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

/* properties */
enum {
  /* generator IDs are used for lower numbers */
  PROP_TITLE = IPATCH_SF2_GEN_ITEM_FIRST_PROP_USER_ID,
  PROP_NAME,
  PROP_BANK,
  PROP_PROGRAM,
  PROP_PERCUSSION,
  PROP_MODULATORS,
  PROP_LIBRARY,
  PROP_GENRE,
  PROP_MORPHOLOGY
};

static void ipatch_sf2_preset_class_init (IpatchSF2PresetClass *klass);
static void
ipatch_sf2_preset_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface);
static void
ipatch_sf2_preset_mod_item_iface_init (IpatchSF2ModItemIface *moditem_iface);
static void ipatch_sf2_preset_init (IpatchSF2Preset *preset);
static void ipatch_sf2_preset_finalize (GObject *gobject);
static void ipatch_sf2_preset_set_property (GObject *object,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_sf2_preset_get_property (GObject *object,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void ipatch_sf2_preset_item_copy (IpatchItem *dest, IpatchItem *src,
					 IpatchItemCopyLinkFunc link_func,
					 gpointer user_data);
static const GType *ipatch_sf2_preset_container_child_types (void);
static gboolean
ipatch_sf2_preset_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type);

static void ipatch_sf2_preset_real_set_name (IpatchSF2Preset *preset,
					     const char *name,
					     gboolean name_notify);

static gpointer parent_class = NULL;
static GType preset_child_types[2] = { 0 };
static GParamSpec *name_pspec, *bank_pspec, *program_pspec, *percuss_pspec;

/* For passing data from class init to gen item interface init */
static GParamSpec **gen_item_specs = NULL;
static GParamSpec **gen_item_setspecs = NULL;

/* For passing between class init and mod item interface init */
static GParamSpec *modulators_spec = NULL;


GType
ipatch_sf2_preset_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchSF2PresetClass), NULL, NULL,
      (GClassInitFunc)ipatch_sf2_preset_class_init, NULL, NULL,
      sizeof (IpatchSF2Preset), 0,
      (GInstanceInitFunc)ipatch_sf2_preset_init,
    };
    static const GInterfaceInfo genitem_iface = {
      (GInterfaceInitFunc) ipatch_sf2_preset_gen_item_iface_init, NULL, NULL };
    static const GInterfaceInfo moditem_iface = {
      (GInterfaceInitFunc) ipatch_sf2_preset_mod_item_iface_init, NULL, NULL };

    item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					"IpatchSF2Preset", &item_info, 0);
    g_type_add_interface_static (item_type, IPATCH_TYPE_SF2_GEN_ITEM, &genitem_iface);
    g_type_add_interface_static (item_type, IPATCH_TYPE_SF2_MOD_ITEM, &moditem_iface);
  }

  return (item_type);
}

static void
ipatch_sf2_preset_class_init (IpatchSF2PresetClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  parent_class = g_type_class_ref (IPATCH_TYPE_CONTAINER);

  obj_class->finalize = ipatch_sf2_preset_finalize;
  obj_class->get_property = ipatch_sf2_preset_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_sf2_preset_set_property;
  item_class->copy = ipatch_sf2_preset_item_copy;

  container_class->child_types = ipatch_sf2_preset_container_child_types;
  container_class->init_iter = ipatch_sf2_preset_container_init_iter;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  name_pspec =
    ipatch_param_set (g_param_spec_string ("name", _("Name"), _("Name"),
		      NULL, G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE),
		      "string-max-length", IPATCH_SFONT_NAME_SIZE, /* max len */
		      NULL);

  g_object_class_install_property (obj_class, PROP_NAME, name_pspec);

  /* bank/program are grouped unique (siblings with same bank/program are
     considered conflicting) */

  bank_pspec = g_param_spec_int ("bank", _("Bank"),
				 _("MIDI bank number"),
				 0, 128, 0,
				 G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE);
  ipatch_param_set (bank_pspec, "unique-group-id", 1, NULL);
  g_object_class_install_property (obj_class, PROP_BANK, bank_pspec);

  program_pspec = g_param_spec_int ("program", _("Program"),
				    _("MIDI program number"),
				    0, 127, 0,
				    G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE);
  ipatch_param_set (program_pspec, "unique-group-id", 1, NULL);
  g_object_class_install_property (obj_class, PROP_PROGRAM, program_pspec);

  percuss_pspec =
    g_param_spec_boolean ("percussion", _("Percussion"),
			  _("Percussion preset?"), FALSE,
			  G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_PERCUSSION, percuss_pspec);

  g_object_class_override_property (obj_class, PROP_MODULATORS, "modulators");
  modulators_spec = g_object_class_find_property (obj_class, "modulators");

  g_object_class_install_property (obj_class, PROP_LIBRARY,
		    g_param_spec_uint ("library", _("Library"),
				       _("Library category"),
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_GENRE,
		    g_param_spec_uint ("genre", _("Genre"),
				       _("Genre category"),
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MORPHOLOGY,
		    g_param_spec_uint ("morphology", _("Morphology"),
				       _("Morphology category"),
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));

  preset_child_types[0] = IPATCH_TYPE_SF2_PZONE;

  /* install generator properties */
  ipatch_sf2_gen_item_iface_install_properties (obj_class,
                                                IPATCH_SF2_GEN_PROPS_PRESET_GLOBAL,
                                                &gen_item_specs, &gen_item_setspecs);
}

/* gen item interface initialization */
static void
ipatch_sf2_preset_gen_item_iface_init (IpatchSF2GenItemIface *genitem_iface)
{
  genitem_iface->genarray_ofs = G_STRUCT_OFFSET (IpatchSF2Preset, genarray);
  genitem_iface->propstype = IPATCH_SF2_GEN_PROPS_PRESET_GLOBAL;

  g_return_if_fail (gen_item_specs != NULL);
  g_return_if_fail (gen_item_setspecs != NULL);

  memcpy (&genitem_iface->specs, gen_item_specs, sizeof (genitem_iface->specs));
  memcpy (&genitem_iface->setspecs, gen_item_setspecs, sizeof (genitem_iface->setspecs));
  g_free (gen_item_specs);
  g_free (gen_item_setspecs);
}

/* mod item interface initialization */
static void
ipatch_sf2_preset_mod_item_iface_init (IpatchSF2ModItemIface *moditem_iface)
{
  moditem_iface->modlist_ofs = G_STRUCT_OFFSET (IpatchSF2Preset, mods);

  /* cache the modulators property for fast notifications */
  moditem_iface->mod_pspec = modulators_spec;
}

static void
ipatch_sf2_preset_init (IpatchSF2Preset *preset)
{
  ipatch_sf2_gen_array_init (&preset->genarray, TRUE, FALSE);
}

static void
ipatch_sf2_preset_finalize (GObject *gobject)
{
  IpatchSF2Preset *preset = IPATCH_SF2_PRESET (gobject);

  /* nothing should reference the preset after this, but we set
     pointers to NULL to help catch invalid references. Locking of
     preset is required since in reality all its children do
     still hold references */

  IPATCH_ITEM_WLOCK (preset);

  g_free (preset->name);
  preset->name = NULL;

  ipatch_sf2_mod_list_free (preset->mods, TRUE);
  preset->mods = NULL;

  IPATCH_ITEM_WUNLOCK (preset);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_sf2_preset_get_title (IpatchSF2Preset *preset, GValue *value)
{
  int bank, program;
  char *name, *s;

  g_object_get (preset,
		"bank", &bank,
		"program", &program,
		"name", &name,
		NULL);
  s = g_strdup_printf ("%03d-%03d %s", bank, program, name);
  g_free (name);

  g_value_take_string (value, s);
}

static void
ipatch_sf2_preset_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  IpatchSF2Preset *preset = IPATCH_SF2_PRESET (object);
  IpatchSF2ModList *list;
  GValue oldvalue = { 0 }, newvalue = { 0 };
  int newbank, oldbank;
  gboolean newpercuss, oldpercuss;

  /* generator property? */
  if (ipatch_sf2_gen_item_iface_set_property ((IpatchSF2GenItem *)preset,
					      property_id, value))
    return;

  switch (property_id)
    {
    case PROP_NAME:
      ipatch_sf2_preset_real_set_name (preset, g_value_get_string (value),
				       FALSE);
      break;
    case PROP_BANK:
      newbank = g_value_get_int (value);

      IPATCH_ITEM_WLOCK (preset);
      oldbank = preset->bank;
      preset->bank = newbank;
      IPATCH_ITEM_WUNLOCK (preset);

      /* do "percussion" property notify if necessary */
      if ((newbank == 128) != (oldbank == 128))
      {
	g_value_init (&newvalue, G_TYPE_BOOLEAN);
	g_value_init (&oldvalue, G_TYPE_BOOLEAN);
	g_value_set_boolean (&newvalue, newbank == 128);
	g_value_set_boolean (&oldvalue, oldbank == 128);
	ipatch_item_prop_notify ((IpatchItem *)preset, percuss_pspec,
				 &newvalue, &oldvalue);
	g_value_unset (&newvalue);
	g_value_unset (&oldvalue);
      }
      break;
    case PROP_PROGRAM:
      IPATCH_ITEM_WLOCK (preset);
      preset->program = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (preset);
      break;
    case PROP_PERCUSSION:
      newpercuss = g_value_get_boolean (value);

      IPATCH_ITEM_WLOCK (preset);
      oldbank = preset->bank;
      oldpercuss = (preset->bank == 128);
      if (newpercuss != oldpercuss) preset->bank = newpercuss ? 128 : 0;
      IPATCH_ITEM_WUNLOCK (preset);

      /* do "bank" property notify if necessary */
      if (newpercuss != oldpercuss)
      {
	g_value_init (&newvalue, G_TYPE_INT);
	g_value_init (&oldvalue, G_TYPE_INT);
	g_value_set_int (&newvalue, newpercuss ? 128 : 0);
	g_value_set_int (&oldvalue, oldbank);
	ipatch_item_prop_notify ((IpatchItem *)preset, bank_pspec,
				 &newvalue, &oldvalue);
	g_value_unset (&newvalue);
	g_value_unset (&oldvalue);
      }
      break;
    case PROP_MODULATORS:
      list = (IpatchSF2ModList *)g_value_get_boxed (value);
      ipatch_sf2_mod_item_set_mods (IPATCH_SF2_MOD_ITEM (preset), list,
				    IPATCH_SF2_MOD_NO_NOTIFY);
      break;
    case PROP_LIBRARY:
      IPATCH_ITEM_WLOCK (preset);
      preset->library = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (preset);
      break;
    case PROP_GENRE:
      IPATCH_ITEM_WLOCK (preset);
      preset->genre = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (preset);
      break;
    case PROP_MORPHOLOGY:
      IPATCH_ITEM_WLOCK (preset);
      preset->morphology = g_value_get_uint (value);
      IPATCH_ITEM_WUNLOCK (preset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }

  /* need to do title notify? */
  if (property_id == PROP_NAME || property_id == PROP_BANK
      || property_id == PROP_PROGRAM || property_id == PROP_PERCUSSION)
    {
      GValue titleval = { 0 };

      g_value_init (&titleval, G_TYPE_STRING);
      ipatch_sf2_preset_get_title (preset, &titleval);
      ipatch_item_prop_notify ((IpatchItem *)preset, ipatch_item_pspec_title,
			       &titleval, NULL);
      g_value_unset (&titleval);
    }
}

static void
ipatch_sf2_preset_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchSF2Preset *preset = IPATCH_SF2_PRESET (object);
  IpatchSF2ModList *list;

  /* generator property? */
  if (ipatch_sf2_gen_item_iface_get_property ((IpatchSF2GenItem *)preset,
					      property_id, value))
    return;

  switch (property_id)
    {
    case PROP_TITLE:
      ipatch_sf2_preset_get_title (preset, value);
      break;
    case PROP_NAME:
      g_value_take_string (value, ipatch_sf2_preset_get_name (preset));
      break;
    case PROP_BANK:
      g_value_set_int (value, preset->bank);
      break;
    case PROP_PROGRAM:
      g_value_set_int (value, preset->program);
      break;
    case PROP_PERCUSSION:
      g_value_set_boolean (value, (preset->bank == 128) ? TRUE : FALSE);
      break;
    case PROP_MODULATORS:
      list = ipatch_sf2_mod_item_get_mods (IPATCH_SF2_MOD_ITEM (preset));
      g_value_take_boxed (value, list);
      break;
    case PROP_LIBRARY:
      g_value_set_uint (value, preset->library);
      break;
    case PROP_GENRE:
      g_value_set_uint (value, preset->genre);
      break;
    case PROP_MORPHOLOGY:
      g_value_set_uint (value, preset->morphology);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sf2_preset_item_copy (IpatchItem *dest, IpatchItem *src,
			     IpatchItemCopyLinkFunc link_func,
			     gpointer user_data)
{
  IpatchSF2Preset *src_pset, *dest_pset;
  IpatchItem *zitem;
  GSList *p;

  src_pset = IPATCH_SF2_PRESET (src);
  dest_pset = IPATCH_SF2_PRESET (dest);

  IPATCH_ITEM_RLOCK (src_pset);

  dest_pset->name = g_strdup (src_pset->name);
  dest_pset->program = src_pset->program;
  dest_pset->bank = src_pset->bank;
  dest_pset->library = src_pset->library;
  dest_pset->genre = src_pset->genre;
  dest_pset->morphology = src_pset->morphology;

  dest_pset->genarray = src_pset->genarray;
  dest_pset->mods = ipatch_sf2_mod_list_duplicate (src_pset->mods);

  p = src_pset->zones;
  while (p)
    {
      zitem = ipatch_item_duplicate_link_func (IPATCH_ITEM (p->data),
					       link_func, user_data);
      dest_pset->zones = g_slist_prepend (dest_pset->zones, zitem);
      ipatch_item_set_parent (zitem, IPATCH_ITEM (dest_pset));

      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (src_pset);

  dest_pset->zones = g_slist_reverse (dest_pset->zones);
}

static const GType *
ipatch_sf2_preset_container_child_types (void)
{
  return (preset_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_sf2_preset_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type)
{
  IpatchSF2Preset *preset = IPATCH_SF2_PRESET (container);

  if (!g_type_is_a (type, IPATCH_TYPE_SF2_PZONE))
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &preset->zones);
  return (TRUE);
}

/**
 * ipatch_sf2_preset_new:
 *
 * Create a new SoundFont preset object.
 *
 * Returns: New SoundFont preset with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchSF2Preset *
ipatch_sf2_preset_new (void)
{
  return (IPATCH_SF2_PRESET (g_object_new (IPATCH_TYPE_SF2_PRESET, NULL)));
}

/**
 * ipatch_sf2_preset_first: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Preset items
 *
 * Gets the first item in a preset iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first preset in @iter or %NULL if empty.
 */
IpatchSF2Preset *
ipatch_sf2_preset_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_SF2_PRESET (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_preset_next: (skip)
 * @iter: Patch item iterator containing #IpatchSF2Preset items
 *
 * Gets the next item in a preset iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next preset in @iter or %NULL if at the end of the list.
 */
IpatchSF2Preset *
ipatch_sf2_preset_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_SF2_PRESET (obj));
  else return (NULL);
}

/**
 * ipatch_sf2_preset_new_zone:
 * @preset: SoundFont preset
 * @inst: Referenced instrument for new zone
 *
 * A convenience function for quickly creating a new preset zone, adding it
 * to @preset and setting the zone's referenced instrument to @inst.
 */
void
ipatch_sf2_preset_new_zone (IpatchSF2Preset *preset, IpatchSF2Inst *inst)
{
  IpatchSF2PZone *pzone;

  g_return_if_fail (IPATCH_IS_SF2_PRESET (preset));
  g_return_if_fail (IPATCH_IS_SF2_INST (inst));

  pzone = ipatch_sf2_pzone_new (); /* ++ ref new zone */
  ipatch_sf2_zone_set_link_item (IPATCH_SF2_ZONE (pzone), IPATCH_ITEM (inst));

  ipatch_container_append (IPATCH_CONTAINER (preset), IPATCH_ITEM (pzone));

  g_object_unref (pzone);	/* -- unref preset zone */
}

/**
 * ipatch_sf2_preset_set_name:
 * @preset: Preset to set name of
 * @name: (nullable): Value to set name to
 *
 * Sets the name of a SoundFont preset.
 */
void
ipatch_sf2_preset_set_name (IpatchSF2Preset *preset, const char *name)
{
  g_return_if_fail (IPATCH_IS_SF2_PRESET (preset));
  ipatch_sf2_preset_real_set_name (preset, name, TRUE);
}

/* the real preset name set routine */
static void
ipatch_sf2_preset_real_set_name (IpatchSF2Preset *preset, const char *name,
				 gboolean name_notify)
{
  GValue oldname = { 0 }, newname = { 0 };
  char *newstr, *oldstr;

  newstr = g_strdup (name);

  IPATCH_ITEM_WLOCK (preset);
  oldstr = preset->name;
  preset->name = newstr;
  IPATCH_ITEM_WUNLOCK (preset);

  g_value_init (&oldname, G_TYPE_STRING);
  g_value_take_string (&oldname, oldstr);

  g_value_init (&newname, G_TYPE_STRING);
  g_value_set_static_string (&newname, name);

  if (name_notify)
    ipatch_item_prop_notify ((IpatchItem *)preset, name_pspec,
			     &newname, &oldname);

  ipatch_item_prop_notify ((IpatchItem *)preset, ipatch_item_pspec_title,
			   &newname, &oldname);

  g_value_unset (&newname);
  g_value_unset (&oldname);
}

/**
 * ipatch_sf2_preset_get_name:
 * @preset: Preset to get name of
 *
 * Gets the name of a SoundFont preset.
 *
 * Returns: Name of preset or %NULL if not set. String value should be freed
 * when finished with it.
 */
char *
ipatch_sf2_preset_get_name (IpatchSF2Preset *preset)
{
  char *name = NULL;

  g_return_val_if_fail (IPATCH_IS_SF2_PRESET (preset), NULL);

  IPATCH_ITEM_RLOCK (preset);
  if (preset->name) name = g_strdup (preset->name);
  IPATCH_ITEM_RUNLOCK (preset);

  return (name);
}

/**
 * ipatch_sf2_preset_set_midi_locale:
 * @preset: Preset to set MIDI locale of
 * @bank: MIDI bank number to assign to preset
 * @program: MIDI program number to assign to preset
 *
 * Sets the MIDI locale of a preset (bank and program numbers).
 */
void
ipatch_sf2_preset_set_midi_locale (IpatchSF2Preset *preset,
				   int bank, int program)
{
  g_object_set (preset, "bank", bank, "program", program, NULL);
}

/**
 * ipatch_sf2_preset_get_midi_locale:
 * @preset: Preset to get MIDI locale from
 * @bank: Location to store preset's MIDI bank number or %NULL
 * @program: Location to store preset's MIDI program number or %NULL
 *
 * Gets the MIDI locale of a SoundFont preset (bank and program numbers).
 */
void
ipatch_sf2_preset_get_midi_locale (IpatchSF2Preset *preset,
				   int *bank, int *program)
{
  g_return_if_fail (IPATCH_IS_SF2_PRESET (preset));

  IPATCH_ITEM_RLOCK (preset);

  if (bank) *bank = preset->bank;
  if (program) *program = preset->program;

  IPATCH_ITEM_RUNLOCK (preset);
}

/**
 * ipatch_sf2_preset_compare:
 * @p1: First preset in comparison
 * @p2: Second preset in comparison
 *
 * Preset comparison function for sorting. Compare two presets by their
 * MIDI bank:program numbers. Note that this function is compatible with
 * GCompareFunc and can therefore be used with g_list_sort, etc.
 *
 * Returns: Comparison result that is less than, equal to, or greater than zero
 * if @p1 is found, respectively, to be less than, to match, or be greater
 * than @p2.
 */
int
ipatch_sf2_preset_compare (const IpatchSF2Preset *p1,
			   const IpatchSF2Preset *p2)
{
  gint32 aval, bval;

  aval = ((gint32)(p1->bank) << 16) | p1->program;
  bval = ((gint32)(p2->bank) << 16) | p2->program;

  return (aval - bval);
}
