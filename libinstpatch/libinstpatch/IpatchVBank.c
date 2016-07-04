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
 * SECTION: IpatchVBank
 * @short_description: Virtual bank object
 * @see_also:
 * @stability: Stable
 *
 * Virtual banks provide the capability of creating new instrument MIDI
 * maps from components from other files of possibly different types.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchVBank.h"
#include "IpatchParamProp.h"
#include "i18n.h"
#include "misc.h"

/* !! Keep synchronized with IPATCH_VBANK_INFO_COUNT constant in IpatchVBank.h */
enum
{
  PROP_0,
  PROP_PARSER_VERSION,	/* IVBank version of parser which wrote the file */
  PROP_REQUIRE_VERSION,	/* IVBank parser version required */
  PROP_ENGINE,		/* Sound engine "FluidSynth 1.0.x" for example */
  PROP_NAME,		/* Descriptive name of bank */
  PROP_DATE,		/* Creation date */
  PROP_AUTHOR,		/* Author */
  PROP_COMMENT		/* Comments */
};

#define IPATCH_VBANK_PARSER_VERSION	"1.0"           // Current IVBank parser version.

static void ipatch_vbank_finalize (GObject *gobject);
static void ipatch_vbank_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec);
static void ipatch_vbank_get_property (GObject *object, guint property_id,
				     GValue *value, GParamSpec *pspec);
static void ipatch_vbank_item_copy (IpatchItem *dest, IpatchItem *src,
				  IpatchItemCopyLinkFunc link_func,
				  gpointer user_data);

static const GType *ipatch_vbank_container_child_types (void);
static gboolean ipatch_vbank_container_init_iter (IpatchContainer *container,
						IpatchIter *iter, GType type);
static void ipatch_vbank_container_make_unique (IpatchContainer *container,
					      IpatchItem *item);
static void ipatch_vbank_base_find_unused_locale (IpatchBase *base, int *bank,
						int *program,
						const IpatchItem *exclude,
						gboolean percussion);
static int locale_gcompare_func (gconstpointer a, gconstpointer b);
static IpatchItem *
ipatch_vbank_base_find_item_by_locale (IpatchBase *base, int bank, int program);


G_DEFINE_TYPE (IpatchVBank, ipatch_vbank, IPATCH_TYPE_BASE);

static GType vbank_child_types[2] = { 0 };


static void
ipatch_vbank_class_init (IpatchVBankClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);
  IpatchBaseClass *base_class = IPATCH_BASE_CLASS (klass);

  obj_class->finalize = ipatch_vbank_finalize;
  obj_class->get_property = ipatch_vbank_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_vbank_set_property;
  item_class->copy = ipatch_vbank_item_copy;

  container_class->child_types = ipatch_vbank_container_child_types;
  container_class->init_iter = ipatch_vbank_container_init_iter;
  container_class->make_unique = ipatch_vbank_container_make_unique;

  base_class->find_unused_locale = ipatch_vbank_base_find_unused_locale;
  base_class->find_item_by_locale = ipatch_vbank_base_find_item_by_locale;

  g_object_class_override_property (obj_class, PROP_NAME, "title");

  g_object_class_install_property (obj_class, PROP_PARSER_VERSION,
	    g_param_spec_string ("parser-version", _("Parser version"),
				 _("Parser version"),
				 IPATCH_VBANK_PARSER_VERSION, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_REQUIRE_VERSION,
	    g_param_spec_string ("require-version", _("Require version"),
				 _("Required parser version"),
				 IPATCH_VBANK_PARSER_VERSION, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_ENGINE,
	    ipatch_param_set (g_param_spec_string ("engine", _("Engine"),
	      _("Synthesis engine"), NULL, G_PARAM_READWRITE),
			      "string-max-length", 255, NULL));
  g_object_class_install_property (obj_class, PROP_NAME,
	    ipatch_param_set (g_param_spec_string ("name", _("Name"),
	      _("Descriptive name"), NULL, G_PARAM_READWRITE),
			      "string-max-length", 255, NULL));
  g_object_class_install_property (obj_class, PROP_DATE,
	    ipatch_param_set (g_param_spec_string ("date", _("Date"),
	      _("Creation date"), NULL, G_PARAM_READWRITE),
			      "string-max-length", 255, NULL));
  g_object_class_install_property (obj_class, PROP_AUTHOR,
	    ipatch_param_set (g_param_spec_string ("author", _("Author"),
	      _("Author of file"), NULL, G_PARAM_READWRITE),
			      "string-max-length", 255, NULL));
  g_object_class_install_property (obj_class, PROP_COMMENT,
	    ipatch_param_set (g_param_spec_string ("comment", _("Comments"),
	      _("Comments"), NULL, G_PARAM_READWRITE),
			      "string-max-length", 65535, NULL));

  vbank_child_types[0] = IPATCH_TYPE_VBANK_INST;
}

static void
ipatch_vbank_init (IpatchVBank *vbank)
{
  g_object_set (vbank, "name", _(IPATCH_BASE_DEFAULT_NAME), NULL);
  ipatch_item_clear_flags (IPATCH_ITEM (vbank), IPATCH_BASE_CHANGED);
}

/* function called when VBank is being destroyed */
static void
ipatch_vbank_finalize (GObject *gobject)
{
  IpatchVBank *vbank = IPATCH_VBANK (gobject);
  int i;

  IPATCH_ITEM_WLOCK (vbank);

  for (i = 0; i < IPATCH_VBANK_INFO_COUNT; i++)
    g_free (vbank->info[i]);

  IPATCH_ITEM_WUNLOCK (vbank);

  if (G_OBJECT_CLASS (ipatch_vbank_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_vbank_parent_class)->finalize (gobject);
}

static void
ipatch_vbank_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
  IpatchVBank *vbank = IPATCH_VBANK (object);

  if (property_id > PROP_0 && property_id <= IPATCH_VBANK_INFO_COUNT)
  {
    g_free (vbank->info[property_id - 1]);
    vbank->info[property_id - 1] = g_value_dup_string (value);

    /* need to do a title property notify? */
    if (property_id == PROP_NAME)
      ipatch_item_prop_notify ((IpatchItem *)vbank, ipatch_item_pspec_title,
			       value, NULL);
  }
  else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
ipatch_vbank_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
  IpatchVBank *vbank = IPATCH_VBANK (object);

  if (property_id > PROP_0 && property_id <= IPATCH_VBANK_INFO_COUNT)
    g_value_set_string (value, vbank->info[property_id - 1]);
  else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

/* item copy function, note that this is an #IpatchBase derived object, so
   link_func is not used */
static void
ipatch_vbank_item_copy (IpatchItem *dest, IpatchItem *src,
		      IpatchItemCopyLinkFunc link_func,
		      gpointer user_data)
{
  IpatchVBank *src_vbank, *dest_vbank;
  IpatchItem *newitem;
  GSList *p;
  int i;

  src_vbank = IPATCH_VBANK (src);
  dest_vbank = IPATCH_VBANK (dest);

  IPATCH_ITEM_RLOCK (src_vbank);

  if (IPATCH_BASE (src_vbank)->file)
    ipatch_base_set_file (IPATCH_BASE (dest_vbank),
			  IPATCH_BASE (src_vbank)->file);

  /* duplicate the info variables */
  for (i = 0; i < IPATCH_VBANK_INFO_COUNT; i++)
    dest_vbank->info[i] = g_strdup (src_vbank->info[i]);

  /* duplicate instruments */
  for (p = src_vbank->insts; p; p = p->next)
  { /* ++ ref new duplicate instrument, !! inst list takes it over */
    newitem = ipatch_item_duplicate ((IpatchItem *)(p->data));
    dest_vbank->insts = g_slist_prepend (dest_vbank->insts, newitem);
    ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_vbank));
  }

  IPATCH_ITEM_RUNLOCK (src_vbank);

  dest_vbank->insts = g_slist_reverse (dest_vbank->insts);
}

static const GType *
ipatch_vbank_container_child_types (void)
{
  return (vbank_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_vbank_container_init_iter (IpatchContainer *container,
				IpatchIter *iter, GType type)
{
  IpatchVBank *vbank = IPATCH_VBANK (container);

  if (g_type_is_a (type, IPATCH_TYPE_VBANK_INST))
    ipatch_iter_GSList_init (iter, &vbank->insts);
  else
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  return (TRUE);
}

static void
ipatch_vbank_container_make_unique (IpatchContainer *container, IpatchItem *item)
{
  IpatchVBank *vbank = IPATCH_VBANK (container);
  char *name, *newname;

  IPATCH_ITEM_WLOCK (vbank);

  if (IPATCH_IS_VBANK_INST (item))
  {
    int bank, newbank, program, newprogram;
    ipatch_vbank_inst_get_midi_locale (IPATCH_VBANK_INST (item),
				       &bank, &program);
    newbank = bank;
    newprogram = program;

    ipatch_base_find_unused_midi_locale (IPATCH_BASE (vbank),
					 &newbank, &newprogram, item, FALSE);

    if (bank != newbank || program != newprogram)
      ipatch_vbank_inst_set_midi_locale (IPATCH_VBANK_INST (item),
					 newbank, newprogram);
  }
  else
  {
    g_critical ("Invalid child type '%s' for IpatchVBank object",
		g_type_name (G_TYPE_FROM_INSTANCE (item)));
    return;
  }

  g_object_get (item, "name", &name, NULL);
  newname = ipatch_vbank_make_unique_name (vbank, name, NULL);

  if (!name || strcmp (name, newname) != 0)
    g_object_set (item, "name", newname, NULL);

  IPATCH_ITEM_WUNLOCK (vbank);

  g_free (name);
  g_free (newname);
}

/* base method to find an unused MIDI bank:program locale */
static void
ipatch_vbank_base_find_unused_locale (IpatchBase *base, int *bank,
				    int *program, const IpatchItem *exclude,
				    gboolean percussion)
{
  IpatchVBank *vbank = IPATCH_VBANK (base);
  GSList *locale_list = NULL;
  IpatchVBankInst *inst;
  GSList *p;
  int b, n;			/* Stores current bank and program number */
  guint lbank, lprogram;

  /* fill array with bank and program numbers */
  IPATCH_ITEM_RLOCK (vbank);

  for (p = vbank->insts; p; p = p->next)
  {
    inst = (IpatchVBankInst *)(p->data);

    /* only add to locale list if not the exclude item */
    if ((gpointer)inst != (gpointer)exclude)
      locale_list = g_slist_prepend (locale_list, GUINT_TO_POINTER
				     (((guint32)inst->bank << 16)
				      | inst->program));
  }

  IPATCH_ITEM_RUNLOCK (vbank);

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
   ipatch_vbank_base_find_unused_locale */
static int
locale_gcompare_func (gconstpointer a, gconstpointer b)
{
  return (GPOINTER_TO_UINT (a) - GPOINTER_TO_UINT (b));
}

static IpatchItem *
ipatch_vbank_base_find_item_by_locale (IpatchBase *base, int bank, int program)
{
  IpatchVBankInst *inst;

  inst = ipatch_vbank_find_inst (IPATCH_VBANK (base), NULL, bank, program, NULL);
  return ((IpatchItem *)inst);
}

/**
 * ipatch_vbank_new:
 *
 * Create a new virtual bank base object.
 *
 * Returns: New IVBank base object with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item.
 */
IpatchVBank *
ipatch_vbank_new (void)
{
  return (IPATCH_VBANK (g_object_new (IPATCH_TYPE_VBANK, NULL)));
}

/**
 * ipatch_vbank_find_inst:
 * @vbank: VBank to search in
 * @name: (nullable): Name of instrument to find or %NULL to match any name
 * @bank: MIDI bank number of instrument to search for or -1 to not search by
 *   MIDI bank:program numbers
 * @program: MIDI program number of instrument to search for, only used
 *   if @bank is 0-128
 * @exclude: (nullable): An instrument to exclude from the search or %NULL
 *
 * Find an instrument by name or bank:preset MIDI numbers. If instrument @name
 * and @bank:@program are specified then match for either condition.
 * If an instrument is found its reference count is incremented before it
 * is returned. The caller is responsible for removing the reference
 * with g_object_unref() when finished with it.
 *
 * Returns: (transfer full): The matching instrument or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchVBankInst *
ipatch_vbank_find_inst (IpatchVBank *vbank, const char *name, int bank,
			int program, const IpatchVBankInst *exclude)
{
  IpatchVBankInst *inst;
  gboolean bynum = FALSE;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_VBANK (vbank), NULL);

  /* if bank and program are valid, then search by number */
  if (bank >= 0 && bank <= 128 && program >= 0 && program < 128)
    bynum = TRUE;

  IPATCH_ITEM_RLOCK (vbank);

  for (p = vbank->insts; p; p = p->next)
  {
    inst = (IpatchVBankInst *)(p->data);

    IPATCH_ITEM_RLOCK (inst);	/* MT - Recursive LOCK */

    if (inst != exclude	/* if exclude is NULL it will never == pset */
	&& ((bynum && inst->bank == bank && inst->program == program)
	    || (name && strcmp (inst->name, name) == 0)))
      {
	g_object_ref (inst);
	IPATCH_ITEM_RUNLOCK (inst);
	IPATCH_ITEM_RUNLOCK (vbank);
	return (inst);
      }

    IPATCH_ITEM_RUNLOCK (inst);
  }

  IPATCH_ITEM_RUNLOCK (vbank);

  return (NULL);
}

/* In theory there is still a chance of duplicates if another item's name is
   set to the generated unique one (by another thread) while in this routine */

/**
 * ipatch_vbank_make_unique_name:
 * @vbank: VBank item
 * @name: (nullable): An initial name to use or %NULL
 * @exclude: (nullable): An item to exclude from search or %NULL
 *
 * Generates a unique instrument name for @vbank. The @name
 * parameter is used as a base and is modified, by appending a number, to
 * make it unique (if necessary). The @exclude parameter is used to exclude
 * an existing @vbank instrument from the search.
 *
 * MT-Note: To ensure that an item is actually unique before being
 * added to a VBank object, ipatch_container_add_unique() should be
 * used.
 *
 * Returns: A new unique name which should be freed when finished with it.
 */
char *
ipatch_vbank_make_unique_name (IpatchVBank *vbank, const char *name,
			       const IpatchVBankInst *exclude)
{
  char curname[IPATCH_VBANK_INST_NAME_SIZE + 1];
  IpatchVBankInst *inst;
  int count = 2;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_VBANK (vbank), NULL);

  if (!name) name = _("New Instrument");
  g_strlcpy (curname, name, sizeof (curname));

  IPATCH_ITEM_RLOCK (vbank);

  /* check for duplicate */
  for (p = vbank->insts; p; p = p->next)
  {
    inst = (IpatchVBankInst *)(p->data);

    IPATCH_ITEM_RLOCK (inst); /* MT - Recursive LOCK */

    if (p->data != exclude && strcmp (inst->name, curname) == 0)
      {			/* duplicate name */
	IPATCH_ITEM_RUNLOCK (inst);

	ipatch_strconcat_num (name, count++, curname, sizeof (curname));

	p = vbank->insts;		/* start over */
	continue;
      }

    IPATCH_ITEM_RUNLOCK (inst);
  }

  IPATCH_ITEM_RUNLOCK (vbank);

  return (g_strdup (curname));
}
