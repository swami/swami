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
 * SECTION: IpatchSF2
 * @short_description: SoundFont instrument file object
 * @see_also: #IpatchSF2Preset, #IpatchSF2Inst, #IpatchSF2Sample
 * @stability: Stable
 *
 * SoundFont version 2 instrument file object.  Parent to #IpatchSF2Preset,
 * #IpatchSF2Inst and #IpatchSF2Sample objects.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchParamProp.h"
#include "IpatchSF2.h"
#include "IpatchSF2File.h"
#include "IpatchSF2Preset.h"
#include "IpatchSF2Zone.h"
#include "IpatchTypeProp.h"
#include "IpatchVirtualContainer_types.h"
#include "ipatch_priv.h"
#include "version.h"

/* the Info properties below shouldn't conflict, since they are composed of
   their 4 byte RIFF chunk ID */
enum
{
  PROP_0,
  PROP_SAMPLES_24BIT	/* indicates that samples should be saved in 24 bit */
};

/* !! Keep in order with IpatchSF2InfoType in IpatchSF2.h */
static guint info_ids[] =
{
  IPATCH_SF2_VERSION, IPATCH_SF2_ENGINE, IPATCH_SF2_NAME,
  IPATCH_SF2_ROM_NAME, IPATCH_SF2_ROM_VERSION, IPATCH_SF2_DATE,
  IPATCH_SF2_AUTHOR, IPATCH_SF2_PRODUCT, IPATCH_SF2_COPYRIGHT,
  IPATCH_SF2_COMMENT, IPATCH_SF2_SOFTWARE
};

/* parameter specs for each info property */
static GParamSpec *info_prop_pspecs[IPATCH_SF2_INFO_COUNT];


#define ERR_MSG_INVALID_INFO_ID_1 "Invalid SoundFont info ID (%d)"

static void ipatch_sf2_class_init (IpatchSF2Class *klass);
static void ipatch_sf2_init (IpatchSF2 *sfont);
static void ipatch_sf2_finalize (GObject *gobject);
static void ipatch_sf2_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec);
static void ipatch_sf2_get_property (GObject *object, guint property_id,
				     GValue *value, GParamSpec *pspec);
static void ipatch_sf2_item_copy (IpatchItem *dest, IpatchItem *src,
				  IpatchItemCopyLinkFunc link_func,
				  gpointer user_data);
static void ipatch_sf2_info_hash_foreach (gpointer key, gpointer value,
					  gpointer user_data);

static const GType *ipatch_sf2_container_child_types (void);
static const GType *ipatch_sf2_container_virtual_types (void);
static gboolean ipatch_sf2_container_init_iter (IpatchContainer *container,
						IpatchIter *iter, GType type);
static void ipatch_sf2_container_make_unique (IpatchContainer *container,
					      IpatchItem *item);
static void ipatch_sf2_base_find_unused_locale (IpatchBase *base, int *bank,
						int *program,
						const IpatchItem *exclude,
						gboolean percussion);
static int locale_gcompare_func (gconstpointer a, gconstpointer b);
static IpatchItem *
ipatch_sf2_base_find_item_by_locale (IpatchBase *base, int bank, int program);
static void ipatch_sf2_real_set_info (IpatchSF2 *sf, IpatchSF2InfoType id,
				      const char *val);
static void ipatch_sf2_foreach_info_GHFunc (gpointer key, gpointer value,
					    gpointer data);
static int ipatch_sf2_info_array_qsort (const void *a, const void *b);

static gpointer parent_class = NULL;
static IpatchItemClass *base_item_class;

static GType sf2_child_types[4] = { 0 };
static GType sf2_virt_types[6] = { 0 };


/* SoundFont item type creation function */
GType
ipatch_sf2_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchSF2Class), NULL, NULL,
      (GClassInitFunc)ipatch_sf2_class_init, NULL, NULL,
      sizeof (IpatchSF2), 0,
      (GInstanceInitFunc)ipatch_sf2_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_BASE, "IpatchSF2",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_sf2_class_init (IpatchSF2Class *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);
  IpatchBaseClass *base_class = IPATCH_BASE_CLASS (klass);
  GParamSpec **sp = &info_prop_pspecs[0];

  /* save original base class for chaining file-name property */
  base_item_class = IPATCH_ITEM_CLASS (g_type_class_ref (IPATCH_TYPE_BASE));

  parent_class = g_type_class_ref (IPATCH_TYPE_BASE);

  obj_class->finalize = ipatch_sf2_finalize;
  obj_class->get_property = ipatch_sf2_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_sf2_set_property;
  item_class->copy = ipatch_sf2_item_copy;

  container_class->child_types = ipatch_sf2_container_child_types;
  container_class->virtual_types = ipatch_sf2_container_virtual_types;
  container_class->init_iter = ipatch_sf2_container_init_iter;
  container_class->make_unique = ipatch_sf2_container_make_unique;

  base_class->find_unused_locale = ipatch_sf2_base_find_unused_locale;
  base_class->find_item_by_locale = ipatch_sf2_base_find_item_by_locale;

  g_object_class_install_property (obj_class, PROP_SAMPLES_24BIT,
      g_param_spec_boolean ("samples-24bit", _("Samples 24bit"),
	    _("Enable 24 bit samples"), FALSE, G_PARAM_READWRITE));

  g_object_class_override_property (obj_class, IPATCH_SF2_NAME, "title");

  *sp = g_param_spec_string ("version", _("Version"),
	  _("SoundFont version (\"major.minor\")"), "2.01", G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, IPATCH_SF2_VERSION, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("engine", _("Engine"),
	  _("Sound synthesis engine identifier"), "EMU8000", G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_ENGINE, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("name", _("Name"),
			  _("SoundFont name"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_NAME, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("rom-name", _("ROM name"),
			  _("ROM name identifier"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_ROM_NAME, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("rom-version", _("ROM version"),
		_("ROM version \"major.minor\""), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_ROM_VERSION, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("date", _("Date"),
			  _("Creation date"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_DATE, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("author", _("Author"),
		_("Author of SoundFont"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_AUTHOR, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("product", _("Product"),
	      _("Product SoundFont is intended for"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_PRODUCT, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("copyright", _("Copyright"),
			  _("Copyright"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_COPYRIGHT, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("comment", _("Comments"),
			  _("Comments"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 65535, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_COMMENT, *sp++);

  *sp = ipatch_param_set (g_param_spec_string ("software", _("Software"),
	    _("Software 'created by:modified by'"), NULL, G_PARAM_READWRITE),
			  "string-max-length", 255, NULL);
  g_object_class_install_property (obj_class, IPATCH_SF2_SOFTWARE, *sp);

  sf2_child_types[0] = IPATCH_TYPE_SF2_PRESET;
  sf2_child_types[1] = IPATCH_TYPE_SF2_INST;
  sf2_child_types[2] = IPATCH_TYPE_SF2_SAMPLE;

  sf2_virt_types[0] = IPATCH_TYPE_VIRTUAL_SF2_MELODIC;
  sf2_virt_types[1] = IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION;
  sf2_virt_types[2] = IPATCH_TYPE_VIRTUAL_SF2_INST;
  sf2_virt_types[3] = IPATCH_TYPE_VIRTUAL_SF2_SAMPLES;
  sf2_virt_types[4] = IPATCH_TYPE_VIRTUAL_SF2_ROM;
}

static void
ipatch_sf2_init (IpatchSF2 *sfont)
{
  sfont->ver_major = 2;
  sfont->ver_minor = 1;

  /* we convert 4 char info IDs to INTs, also set up hash table to free
     allocated string values */
  sfont->info = g_hash_table_new_full (NULL, NULL, NULL,
				       (GDestroyNotify)g_free);

  /* set required SoundFont info to default values */
  ipatch_sf2_set_info (sfont, IPATCH_SF2_NAME, _(IPATCH_BASE_DEFAULT_NAME));
  ipatch_sf2_set_info (sfont, IPATCH_SF2_ENGINE, IPATCH_SF2_DEFAULT_ENGINE);
  ipatch_sf2_set_info (sfont, IPATCH_SF2_SOFTWARE,
		       "libInstPatch v" IPATCH_VERSION ":");

  ipatch_item_clear_flags (IPATCH_ITEM (sfont), IPATCH_BASE_CHANGED);
}

/* function called when SoundFont is being destroyed */
static void
ipatch_sf2_finalize (GObject *gobject)
{
  IpatchSF2 *sf = IPATCH_SF2 (gobject);

  IPATCH_ITEM_WLOCK (sf);

  g_hash_table_destroy (sf->info);	/* destroy SoundFont info */
  sf->info = NULL;

  IPATCH_ITEM_WUNLOCK (sf);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_sf2_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
  IpatchSF2 *sf;
  gboolean b;

  g_return_if_fail (IPATCH_IS_SF2 (object));
  sf = IPATCH_SF2 (object);

  if (property_id == PROP_SAMPLES_24BIT)
    {
      b = g_value_get_boolean (value);

      if (b) ipatch_item_set_flags (IPATCH_ITEM (sf), IPATCH_SF2_SAMPLES_24BIT);
      else ipatch_item_clear_flags (IPATCH_ITEM (sf), IPATCH_SF2_SAMPLES_24BIT);
    }
  else if (property_id == IPATCH_SF2_VERSION
	   || property_id == IPATCH_SF2_ROM_VERSION)
    {
      guint major, minor;

      if (sscanf (g_value_get_string (value), "%u.%u",
		  &major, &minor) != 2)
	{
	  g_critical ("SoundFont version property parse error");
	  return;
	}

      if (property_id == IPATCH_SF2_VERSION)
	{
	  IPATCH_ITEM_WLOCK (sf);
	  sf->ver_major = major;
	  sf->ver_minor = minor;
	  IPATCH_ITEM_WUNLOCK (sf);
	}
      else
	{
	  IPATCH_ITEM_WLOCK (sf);
	  sf->romver_major = major;
	  sf->romver_minor = minor;
	  IPATCH_ITEM_WUNLOCK (sf);
	}
    }
  else if (ipatch_sf2_info_id_is_valid (property_id))
    {
      ipatch_sf2_real_set_info (sf, property_id, g_value_get_string (value));

      /* need to do a title property notify? */
      if (property_id == IPATCH_SF2_NAME)
	ipatch_item_prop_notify ((IpatchItem *)sf, ipatch_item_pspec_title,
				 value, NULL);
    }
  else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
ipatch_sf2_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
  IpatchSF2 *sf;
  char *s;

  g_return_if_fail (IPATCH_IS_SF2 (object));
  sf = IPATCH_SF2 (object);

  if (property_id == PROP_SAMPLES_24BIT)
    {
      g_value_set_boolean (value, (ipatch_item_get_flags (IPATCH_ITEM (sf))
				   & IPATCH_SF2_SAMPLES_24BIT) != 0);
    }
  else if (property_id == IPATCH_SF2_VERSION
      || property_id == IPATCH_SF2_ROM_VERSION)
    {
      int major, minor;

      IPATCH_ITEM_RLOCK (sf);
      if (property_id == IPATCH_SF2_VERSION)
	{
	  major = sf->ver_major;
	  minor = sf->ver_minor;
	}
      else
	{
	  major = sf->romver_major;
	  minor = sf->romver_minor;
	}
      IPATCH_ITEM_RUNLOCK (sf);

      s = g_strdup_printf ("%d.%d", major, minor);
      g_value_take_string (value, s);
    }
  else if (ipatch_sf2_info_id_is_valid (property_id))
    g_value_take_string (value, ipatch_sf2_get_info (sf, property_id));
  else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

/* item copy function, note that this is an #IpatchBase derived object, so
   link_func is not used */
static void
ipatch_sf2_item_copy (IpatchItem *dest, IpatchItem *src,
		      IpatchItemCopyLinkFunc link_func,
		      gpointer user_data)
{
  IpatchSF2 *src_sf, *dest_sf;
  IpatchItem *newitem;
  GHashTable *repl_samples, *repl_insts;
  gboolean has_linked = FALSE;
  GSList *p;

  src_sf = IPATCH_SF2 (src);
  dest_sf = IPATCH_SF2 (dest);

  /* create item replacement hash */
  repl_samples = g_hash_table_new (NULL, NULL);
  repl_insts = g_hash_table_new (NULL, NULL);

  IPATCH_ITEM_RLOCK (src_sf);

  if (ipatch_item_get_flags (src) & IPATCH_SF2_SAMPLES_24BIT)
    ipatch_item_set_flags (dest, IPATCH_SF2_SAMPLES_24BIT);
  else ipatch_item_clear_flags (dest, IPATCH_SF2_SAMPLES_24BIT);

  dest_sf->ver_major = src_sf->ver_major;
  dest_sf->ver_minor = src_sf->ver_minor;
  dest_sf->romver_major = src_sf->romver_major;
  dest_sf->romver_minor = src_sf->romver_minor;

  if (IPATCH_BASE (src_sf)->file)
    ipatch_base_set_file (IPATCH_BASE (dest_sf), IPATCH_BASE (src_sf)->file);

  /* duplicate the info variables */
  g_hash_table_foreach (src_sf->info, ipatch_sf2_info_hash_foreach, dest_sf);

  p = src_sf->samples;
  while (p)			/* duplicate samples */
    { /* ++ ref new duplicate sample, !! sample list takes it over */
      newitem = ipatch_item_duplicate ((IpatchItem *)(p->data));
      dest_sf->samples = g_slist_prepend (dest_sf->samples, newitem);
      ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_sf));

      /* add to sample pointer replacement hash */
      g_hash_table_insert (repl_samples, p->data, newitem);

      /* check if sample is stereo linked */
      if (ipatch_sf2_sample_peek_linked ((IpatchSF2Sample *)newitem))
        has_linked = TRUE;

      p = g_slist_next (p);
    }

  /* if any linked samples exist, we must replace old linked pointers,
     duplicate "replace" hash wont work since samples reference items in the
     same list */
  if (has_linked)
    {
      IpatchSF2Sample *linked;

      p = dest_sf->samples;
      while (p)
	{
	  IpatchSF2Sample *sample = (IpatchSF2Sample *)(p->data);

	  linked = ipatch_sf2_sample_peek_linked (sample);
	  if (linked)
	    {
	      linked = g_hash_table_lookup (repl_samples, linked);
	      ipatch_sf2_sample_set_linked (sample, linked);
	    }

	  p = g_slist_next (p);
	}
    }

  p = src_sf->insts;
  while (p)			/* duplicate instruments */
    { /* ++ ref new duplicate instrument, !! instrument list takes it over
       * duplicate instrument and replace referenced sample pointers. */
      newitem = ipatch_item_duplicate_replace ((IpatchItem *)(p->data),
					       repl_samples);
      dest_sf->insts = g_slist_prepend (dest_sf->insts, newitem);
      ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_sf));

      /* add to instrument pointer replacement hash */
      g_hash_table_insert (repl_insts, p->data, newitem);

      p = g_slist_next (p);
    }

  p = src_sf->presets;
  while (p)			/* duplicate presets */
    { /* ++ ref new duplicate preset, !! preset list takes it over
       * duplicate preset and replace referenced instrument pointers. */
      newitem = ipatch_item_duplicate_replace ((IpatchItem *)(p->data),
					       repl_insts);
      dest_sf->presets = g_slist_prepend (dest_sf->presets, newitem);
      ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_sf));

      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (src_sf);

  dest_sf->presets = g_slist_reverse (dest_sf->presets);
  dest_sf->insts = g_slist_reverse (dest_sf->insts);
  dest_sf->samples = g_slist_reverse (dest_sf->samples);

  g_hash_table_destroy (repl_samples);
  g_hash_table_destroy (repl_insts);
}

static void
ipatch_sf2_info_hash_foreach (gpointer key, gpointer value,
			      gpointer user_data)
{
  char *val = (char *)value;
  IpatchSF2 *dup = (IpatchSF2 *)user_data;

  ipatch_sf2_set_info (dup, GPOINTER_TO_UINT (key), val);
}

static const GType *
ipatch_sf2_container_child_types (void)
{
  return (sf2_child_types);
}

static const GType *
ipatch_sf2_container_virtual_types (void)
{
  return (sf2_virt_types);
}

/* container is locked by caller */
static gboolean
ipatch_sf2_container_init_iter (IpatchContainer *container,
				IpatchIter *iter, GType type)
{
  IpatchSF2 *sfont = IPATCH_SF2 (container);

  if (g_type_is_a (type, IPATCH_TYPE_SF2_PRESET))
    ipatch_iter_GSList_init (iter, &sfont->presets);
  else if (g_type_is_a (type, IPATCH_TYPE_SF2_INST))
    ipatch_iter_GSList_init (iter, &sfont->insts);
  else if (g_type_is_a (type, IPATCH_TYPE_SF2_SAMPLE))
    ipatch_iter_GSList_init (iter, &sfont->samples);
  else
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  return (TRUE);
}

static void
ipatch_sf2_container_make_unique (IpatchContainer *container, IpatchItem *item)
{
  IpatchSF2 *sfont = IPATCH_SF2 (container);
  char *name, *newname;

  IPATCH_ITEM_WLOCK (sfont);

  if (IPATCH_IS_SF2_PRESET (item))
    {
      int bank, newbank, program, newprogram;
      ipatch_sf2_preset_get_midi_locale (IPATCH_SF2_PRESET (item),
					 &bank, &program);
      newbank = bank;
      newprogram = program;

      ipatch_base_find_unused_midi_locale (IPATCH_BASE (sfont),
					   &newbank, &newprogram, item,
					   newbank == 128);

      if (bank != newbank || program != newprogram)
	ipatch_sf2_preset_set_midi_locale (IPATCH_SF2_PRESET (item),
					   newbank, newprogram);
    }
  else if (!IPATCH_IS_SF2_INST (item) && !IPATCH_IS_SF2_SAMPLE (item))
    {
      g_critical ("Invalid child type '%s' for IpatchSF2 object",
		  g_type_name (G_TYPE_FROM_INSTANCE (item)));
      return;
    }

  g_object_get (item, "name", &name, NULL);
  newname = ipatch_sf2_make_unique_name (sfont, G_TYPE_FROM_INSTANCE (item),
					 name, NULL);
  if (!name || strcmp (name, newname) != 0)
    g_object_set (item, "name", newname, NULL);

  IPATCH_ITEM_WUNLOCK (sfont);

  g_free (name);
  g_free (newname);
}

/* base method to find an unused MIDI bank:program locale */
static void
ipatch_sf2_base_find_unused_locale (IpatchBase *base, int *bank,
				    int *program, const IpatchItem *exclude,
				    gboolean percussion)
{
  IpatchSF2 *sf = IPATCH_SF2 (base);
  GSList *locale_list = NULL;
  IpatchSF2Preset *pset;
  GSList *p;
  int b, n;			/* Stores current bank and program number */
  guint lbank, lprogram;

  if (percussion) *bank = 128;

  /* fill array with bank and program numbers */
  IPATCH_ITEM_RLOCK (sf);
  p = sf->presets;
  while (p)
    {
      pset = (IpatchSF2Preset *)(p->data);

      /* only add to locale list if not the exclude item */
      if ((gpointer)pset != (gpointer)exclude)
	locale_list = g_slist_prepend (locale_list, GUINT_TO_POINTER
				       (((guint32)pset->bank << 16)
					| pset->program));
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sf);

  if (!locale_list) return;

  locale_list = g_slist_sort (locale_list, (GCompareFunc)locale_gcompare_func);

  b = *bank;
  n = *program;

  /* loop through sorted list of bank:programs */
  p = locale_list;
  while (p)
    {
      lprogram = GPOINTER_TO_UINT (p->data);
      lbank = lprogram >> 16;
      lprogram &= 0xFFFF;

      if (lbank > b || (lbank == b && lprogram > n)) break;
      if (lbank >= b)
	{
	  if (++n > 127)
	    {
	      n = 0;
	      b++;
	    }
	}
      p = g_slist_delete_link (p, p); /* delete and advance */
    }
  *bank = b;
  *program = n;

  if (p) g_slist_free (p);	/* free remainder of list */
}

/* function used to do a temporary sort on preset list for
   ipatch_sf2_base_find_unused_locale */
static int
locale_gcompare_func (gconstpointer a, gconstpointer b)
{
  return (GPOINTER_TO_UINT (a) - GPOINTER_TO_UINT (b));
}

static IpatchItem *
ipatch_sf2_base_find_item_by_locale (IpatchBase *base, int bank, int program)
{
  IpatchSF2Preset *preset;

  preset = ipatch_sf2_find_preset (IPATCH_SF2 (base), NULL, bank, program, NULL);
  return ((IpatchItem *)preset);
}

/**
 * ipatch_sf2_new:
 *
 * Create a new SoundFont base object.
 *
 * Returns: New SoundFont base object with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item.
 */
IpatchSF2 *
ipatch_sf2_new (void)
{
  return (IPATCH_SF2 (g_object_new (IPATCH_TYPE_SF2, NULL)));
}

/**
 * ipatch_sf2_set_file:
 * @sf: SoundFont to set file object of
 * @file: File object to use with the SoundFont.
 *
 * Sets the file object of a SoundFont. SoundFont files are kept open
 * for sample data that references the file. This function sets a
 * SoundFonts authoritive file object. A convenience function, as
 * ipatch_base_set_file() does the same thing (albeit without more specific
 * type casting).
 */
void
ipatch_sf2_set_file (IpatchSF2 *sf, IpatchSF2File *file)
{
  g_return_if_fail (IPATCH_IS_SF2 (sf));
  g_return_if_fail (IPATCH_IS_SF2_FILE (file));

  ipatch_base_set_file (IPATCH_BASE (sf), IPATCH_FILE (file));
}

/**
 * ipatch_sf2_get_file:
 * @sf: SoundFont to get file object of
 *
 * Gets the file object of a SoundFont. The returned SoundFont file object's
 * reference count has incremented. The caller owns the reference and is
 * responsible for removing it with <function>g_object_unref()</function>.
 * A convenience function as ipatch_base_get_file() does the same thing
 * (albeit without more specific type casting).
 *
 * Returns: (transfer full): The SoundFont file object or %NULL if @sf is not open. Remember
 * to unref the file object with <function>g_object_unref()</function> when
 * done with it.
 */
IpatchSF2File *
ipatch_sf2_get_file (IpatchSF2 *sf)
{
  IpatchFile *file;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);

  file = ipatch_base_get_file (IPATCH_BASE (sf));
  if (file) return (IPATCH_SF2_FILE (file));
  else return (NULL);
}

/**
 * ipatch_sf2_get_info:
 * @sf: SoundFont to get info from
 * @id: RIFF FOURCC id
 *
 * Get a SoundFont info string by RIFF FOURCC ID.
 *
 * Returns: New allocated info string value or %NULL if no info with the
 * given @id. String should be freed when finished with it.
 */
char *
ipatch_sf2_get_info (IpatchSF2 *sf, IpatchSF2InfoType id)
{
  char *val;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);

  IPATCH_ITEM_RLOCK (sf);
  val = g_hash_table_lookup (sf->info, GUINT_TO_POINTER (id));
  if (val) val = g_strdup (val);
  IPATCH_ITEM_RUNLOCK (sf);

  return (val);
}

/**
 * ipatch_sf2_set_info:
 * @sf: SoundFont to set info of
 * @id: RIFF FOURCC id
 * @val: Value to set info to or %NULL to unset (clear) info.
 *
 * Set SoundFont info.  Validates @id and ensures @val does not exceed
 * the maximum allowed length for the given info type.
 *
 * Emits changed signal on SoundFont.
 */
void
ipatch_sf2_set_info (IpatchSF2 *sf, IpatchSF2InfoType id, const char *val)
{
  GParamSpec *pspec;
  GValue oldval = { 0 }, newval = { 0 };
  int i;

  g_return_if_fail (IPATCH_IS_SF2 (sf));

  for (i = 0; i < G_N_ELEMENTS (info_ids); i++)
    if (info_ids[i] == id) break;

  if (i == G_N_ELEMENTS (info_ids))
    {
      g_return_if_fail (ipatch_sf2_info_id_is_valid (id));  /* for debugging */
      return;
    }

  pspec = info_prop_pspecs[i];

  g_value_init (&oldval, G_TYPE_STRING);
  g_value_take_string (&oldval, ipatch_sf2_get_info (sf, id));

  ipatch_sf2_real_set_info (sf, id, val);

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, val);

  ipatch_item_prop_notify ((IpatchItem *)sf, pspec, &newval, &oldval);

  /* need to do a title property notify? */
  if (id == IPATCH_SF2_NAME)
    ipatch_item_prop_notify ((IpatchItem *)sf, ipatch_item_pspec_title,
			     &newval, &oldval);
  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/* the real set info by id routine, user routine does a property notify */
static void
ipatch_sf2_real_set_info (IpatchSF2 *sf, IpatchSF2InfoType id,
			  const char *val)
{
  char *newval = NULL;
  int maxlen;

  maxlen = ipatch_sf2_get_info_max_size (id);

  /* value exceeds max length? */
  if (maxlen > 0 && val && strlen (val) > maxlen - 1)
    {
      g_warning ("IpatchSF2Info string with id '%.4s' truncated",
		 (char *)&id);
      newval = g_strndup (val, maxlen - 1);
    }
  else if (val) newval = g_strdup (val);

  /* we set up the hash table to free old string values */
  IPATCH_ITEM_WLOCK (sf);
  if (newval) g_hash_table_insert (sf->info, GINT_TO_POINTER (id), newval);
  else g_hash_table_remove (sf->info, GINT_TO_POINTER (id));
  IPATCH_ITEM_WUNLOCK (sf);

  /* newval has been taken over by hash table */
}

/* a bag for ipatch_sf2_get_info_array() */
typedef struct
{
  int count;
  IpatchSF2Info *info;
} InfoArrayBag;

/**
 * ipatch_sf2_get_info_array: (skip)
 * @sf: SoundFont to get all info strings from
 *
 * Get all string info (not IPATCH_SF2_VERSION or IPATCH_SF2_ROM_VERSION)
 * from a SoundFont object. The array is sorted in the order recommended
 * by the SoundFont standard for saving.
 *
 * Returns: A newly allocated array of info structures terminated by
 * an array member with 0 valued <structfield>id</structfield>
 * field. Remember to free the array with ipatch_sf2_free_info_array()
 * when finished with it.
 */
IpatchSF2Info *
ipatch_sf2_get_info_array (IpatchSF2 *sf)
{
  IpatchSF2Info *array;
  InfoArrayBag bag;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);

  /* allocate max expected info elements + 1 for terminator */
  array = g_malloc ((IPATCH_SF2_INFO_COUNT + 1) * sizeof (IpatchSF2Info));

  bag.count = 0;
  bag.info = array;

  IPATCH_ITEM_RLOCK (sf);
  g_hash_table_foreach (sf->info, ipatch_sf2_foreach_info_GHFunc, &bag);
  IPATCH_ITEM_RUNLOCK (sf);

  qsort (array, bag.count, sizeof (IpatchSF2Info),
	 ipatch_sf2_info_array_qsort);

  /* terminate array */
  array[bag.count].id = 0;
  array[bag.count].val = NULL;

  /* re-allocate for actual size */
  return (g_realloc (array, (bag.count + 1) * sizeof (IpatchSF2Info)));
}

static void
ipatch_sf2_foreach_info_GHFunc (gpointer key, gpointer value, gpointer data)
{
  InfoArrayBag *bag = (InfoArrayBag *)data;

  /* shouldn't happen, but just in case */
  if (bag->count >= IPATCH_SF2_INFO_COUNT) return;

  /* store the info ID and string in the info array */
  bag->info[bag->count].id = GPOINTER_TO_UINT (key);
  bag->info[bag->count].val = g_strdup ((char *)value);
  bag->count++;			/* advance to next index */
}

/* sorts an info array according to recommended SoundFont order */
static int
ipatch_sf2_info_array_qsort (const void *a, const void *b)
{
  IpatchSF2Info *ainfo = (IpatchSF2Info *)a;
  IpatchSF2Info *binfo = (IpatchSF2Info *)b;
  int andx, bndx;

  /* find index in info ID array */
  for (andx = 0; andx < G_N_ELEMENTS (info_ids); andx++)
    if (info_ids[andx] == ainfo->id) break;
  for (bndx = 0; bndx < G_N_ELEMENTS (info_ids); bndx++)
    if (info_ids[bndx] == binfo->id) break;

  return (andx - bndx);
}

/**
 * ipatch_sf2_free_info_array: (skip)
 * @array: SoundFont info array
 *
 * Frees an info array returned by ipatch_sf2_get_info_array().
 */
void
ipatch_sf2_free_info_array (IpatchSF2Info *array)
{
  int i;
  g_return_if_fail (array != NULL);

  for (i = 0; array[i].id; i++)
    g_free (array[i].val);

  g_free (array);
}

/**
 * ipatch_sf2_info_id_is_valid: (skip)
 * @id: RIFF FOURCC id (see #IpatchSF2InfoType)
 *
 * Check if a given RIFF FOURCC id is a valid SoundFont info id.
 *
 * Returns: %TRUE if @id is a valid info id, %FALSE otherwise
 */
gboolean
ipatch_sf2_info_id_is_valid (guint32 id)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (info_ids) ; i++)
    if (info_ids[i] == id) return (TRUE);

  return (FALSE);
}

/**
 * ipatch_sf2_get_info_max_size: (skip)
 * @infotype: An info enumeration
 *
 * Get maximum chunk size for info chunks.
 *
 * NOTE: Max size includes terminating NULL character so subtract one from
 * returned value to get max allowed string length.
 *
 * Returns: Maximum info chunk size or 0 if invalid @infotype. Subtract one
 * to get max allowed string length (if returned value was not 0).
 */
int
ipatch_sf2_get_info_max_size (IpatchSF2InfoType infotype)
{
  if (!ipatch_sf2_info_id_is_valid (infotype))
    return (0);

  if (infotype == IPATCH_SF2_COMMENT) /* comments can have up to 64k bytes */
    return (65536);

  if (infotype == IPATCH_SF2_VERSION /* versions take up 4 bytes */
      || infotype == IPATCH_SF2_ROM_VERSION)
    return (4);

  return (256);	  /* all other valid info types allow 256 bytes max */
}

/**
 * ipatch_sf2_find_preset:
 * @sf: SoundFont to search in
 * @name: (nullable): Name of preset to find or %NULL to match any name
 * @bank: MIDI bank number of preset to search for or -1 to not search by
 *   MIDI bank:program numbers
 * @program: MIDI program number of preset to search for, only used
 *   if @bank is 0-128
 * @exclude: (nullable): A preset to exclude from the search or %NULL
 *
 * Find a preset by name or bank:preset MIDI numbers. If preset @name
 * and @bank:@program are specified then match for either condition.
 * If a preset is found its reference count is incremented before it
 * is returned. The caller is responsible for removing the reference
 * with g_object_unref() when finished with it.
 *
 * Returns: (transfer full): The matching preset or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchSF2Preset *
ipatch_sf2_find_preset (IpatchSF2 *sf, const char *name, int bank,
			int program, const IpatchSF2Preset *exclude)
{
  IpatchSF2Preset *pset;
  gboolean bynum = FALSE;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);

  /* if bank and program are valid, then search by number */
  if (bank >= 0 && bank <= 128 && program >= 0 && program < 128)
    bynum = TRUE;

  IPATCH_ITEM_RLOCK (sf);
  p = sf->presets;
  while (p)
    {
      pset = (IpatchSF2Preset *)(p->data);
      IPATCH_ITEM_RLOCK (pset);	/* MT - Recursive LOCK */

      if (pset != exclude	/* if exclude is NULL it will never == pset */
	  && ((bynum && pset->bank == bank && pset->program == program)
	      || (name && strcmp (pset->name, name) == 0)))
	{
	  g_object_ref (pset);
	  IPATCH_ITEM_RUNLOCK (pset);
	  IPATCH_ITEM_RUNLOCK (sf);
	  return (pset);
	}
      IPATCH_ITEM_RUNLOCK (pset);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sf);

  return (NULL);
}

/**
 * ipatch_sf2_find_inst:
 * @sf: SoundFont to search in
 * @name: Name of Instrument to find
 * @exclude: (nullable): An instrument to exclude from the search or %NULL
 *
 * Find an instrument by @name in a SoundFont. If a matching instrument
 * is found, its reference count is incremented before it is returned.
 * The caller is responsible for removing the reference with g_object_unref()
 * when finished with it.
 *
 * Returns: (transfer full): The matching instrument or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchSF2Inst *
ipatch_sf2_find_inst (IpatchSF2 *sf, const char *name,
		      const IpatchSF2Inst *exclude)
{
  IpatchSF2Inst *inst;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  IPATCH_ITEM_RLOCK (sf);
  p = sf->insts;
  while (p)
    {
      inst = (IpatchSF2Inst *)(p->data);
      IPATCH_ITEM_RLOCK (inst);	/* MT - Recursive LOCK */

      if (inst != exclude && strcmp (inst->name, name) == 0)
	{
	  g_object_ref (inst);
	  IPATCH_ITEM_RUNLOCK (inst);
	  IPATCH_ITEM_RUNLOCK (sf);
	  return (inst);
	}
      IPATCH_ITEM_RUNLOCK (inst);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sf);

  return (NULL);
}

/**
 * ipatch_sf2_find_sample:
 * @sf: SoundFont to search in
 * @name: Name of sample to find
 * @exclude: (nullable): A sample to exclude from the search or %NULL
 *
 * Find a sample by @name in a SoundFont. If a sample is found its
 * reference count is incremented before it is returned. The caller
 * is responsible for removing the reference with g_object_unref()
 * when finished with it.
 *
 * Returns: (transfer full): The matching sample or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchSF2Sample *
ipatch_sf2_find_sample (IpatchSF2 *sf, const char *name,
			const IpatchSF2Sample *exclude)
{
  IpatchSF2Sample *sample;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SF2 (sf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  IPATCH_ITEM_RLOCK (sf);
  p = sf->samples;
  while (p)
    {
      sample = (IpatchSF2Sample *)(p->data);
      IPATCH_ITEM_RLOCK (sample); /* MT - Recursive LOCK */

      if (sample != exclude && strcmp (sample->name, name) == 0)
	{
	  g_object_ref (sample);
	  IPATCH_ITEM_RUNLOCK (sample);
	  IPATCH_ITEM_RUNLOCK (sf);
	  return (sample);
	}
      IPATCH_ITEM_RUNLOCK (sample);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sf);

  return (NULL);
}

/**
 * ipatch_sf2_get_zone_references:
 * @item: Item to locate referencing zones of, must be of type #IpatchSF2Inst
 *   or #IpatchSF2Sample and be parented to an #IpatchSF2 object.
 *
 * Get list of zones referencing an IpatchSF2Inst or IpatchSF2Sample.
 *
 * Returns: (transfer full): New object list containing #IpatchSF2Zone objects that
 *   refer to @item. The new list object has a reference count of 1
 *   which the caller owns, unreference to free the list.
 */
IpatchList *
ipatch_sf2_get_zone_references (IpatchItem *item)
{
  IpatchList *reflist, *itemlist, *zonelist;
  IpatchSF2 *sf;
  IpatchSF2Zone *zone;
  IpatchIter iter, zone_iter;
  IpatchItem *pitem;

  g_return_val_if_fail (IPATCH_IS_SF2_INST (item) || IPATCH_IS_SF2_SAMPLE (item), NULL);

  pitem = ipatch_item_get_parent (item);
  g_return_val_if_fail (IPATCH_IS_SF2 (pitem), NULL);
  sf = IPATCH_SF2 (pitem);

  /* ++ ref item list */
  if (IPATCH_IS_SF2_INST (item)) /* is an instrument? */
    itemlist = ipatch_sf2_get_presets (sf);
  else itemlist = ipatch_sf2_get_insts (sf); /* its a sample */

  reflist = ipatch_list_new ();	/* ++ ref new list */

  ipatch_list_init_iter (itemlist, &iter);
  pitem = ipatch_item_first (&iter);
  while (pitem)      /* loop on item list  */
    { /* ++ ref new zone list */
      zonelist = ipatch_container_get_children ((IpatchContainer *)(pitem),
						IPATCH_TYPE_SF2_ZONE);
      ipatch_list_init_iter (zonelist, &zone_iter);

      zone = ipatch_sf2_zone_first (&zone_iter);
      while (zone)
	{
	  if (ipatch_sf2_zone_peek_link_item (zone) == item)
	    {
	      g_object_ref (zone); /* ++ ref zone for new list */
	      reflist->items = g_list_prepend (reflist->items, zone);
	    }
	  zone = ipatch_sf2_zone_next (&zone_iter);
	}
      g_object_unref (zonelist); /* -- unref zone list */
      pitem = ipatch_item_next (&iter);
    }

  g_object_unref (itemlist);	/* -- unref item list */

  return (reflist);		/* !! caller takes over reference */
}

/* In theory there is still a chance of duplicates if another item's name is
   set to the generated unique one (by another thread) while in this routine */

/**
 * ipatch_sf2_make_unique_name:
 * @sfont: SoundFont item
 * @child_type: A child type of @sfont to search for a unique name in
 * @name: (nullable): An initial name to use or %NULL
 * @exclude: (nullable): An item to exclude from search or %NULL
 *
 * Generates a unique name for the given @child_type in @sfont. The @name
 * parameter is used as a base and is modified, by appending a number, to
 * make it unique (if necessary). The @exclude parameter is used to exclude
 * an existing @sfont child item from the search.
 *
 * MT-Note: To ensure that an item is actually unique before being
 * added to a SoundFont object, ipatch_container_add_unique() should be
 * used.
 *
 * Returns: A new unique name which should be freed when finished with it.
 */
char *
ipatch_sf2_make_unique_name (IpatchSF2 *sfont, GType child_type,
			     const char *name, const IpatchItem *exclude)
{
  GSList **list, *p;
  char curname[IPATCH_SFONT_NAME_SIZE + 1];
  int name_ofs;
  int count = 2;

  g_return_val_if_fail (IPATCH_IS_SF2 (sfont), NULL);

  if (child_type == IPATCH_TYPE_SF2_PRESET)
    {
      list = &sfont->presets;
      name_ofs = G_STRUCT_OFFSET (IpatchSF2Preset, name);
      if (!name) name = _("New Preset");
    }
  else if (child_type == IPATCH_TYPE_SF2_INST)
    {
      list = &sfont->insts;
      name_ofs = G_STRUCT_OFFSET (IpatchSF2Inst, name);
      if (!name) name = _("New Instrument");
    }
  else if (child_type == IPATCH_TYPE_SF2_SAMPLE)
    {
      list = &sfont->samples;
      name_ofs = G_STRUCT_OFFSET (IpatchSF2Sample, name);
      if (!name) name = _("New Sample");
    }
  else
    {
      g_critical (IPATCH_CONTAINER_ERRMSG_INVALID_CHILD_2,
		  g_type_name (child_type), g_type_name (IPATCH_TYPE_SF2));
      return (NULL);
    }

  g_strlcpy (curname, name, sizeof (curname));

  IPATCH_ITEM_RLOCK (sfont);

  p = *list;
  while (p)	/* check for duplicate */
    {
      IPATCH_ITEM_RLOCK (p->data); /* MT - Recursive LOCK */

      if (p->data != exclude
	  && strcmp (G_STRUCT_MEMBER (char *, p->data, name_ofs),
		     curname) == 0)
	{			/* duplicate name */
	  IPATCH_ITEM_RUNLOCK (p->data);

	  ipatch_strconcat_num (name, count++, curname, sizeof (curname));

	  p = *list;		/* start over */
	  continue;
	}

      IPATCH_ITEM_RUNLOCK (p->data);
      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (sfont);

  return (g_strdup (curname));
}
