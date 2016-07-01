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
 * SECTION: IpatchVBankInst
 * @short_description: VBank instrument item
 * @see_also: #IpatchVBank
 * @stability: Stable
 *
 * VBank instruments are children of #IpatchVBank objects and define individual
 * instruments mapped to MIDI bank/program numbers and which reference items
 * in other instrument files.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchVBankInst.h"
#include "IpatchVBankRegion.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

/* properties */
enum
{
  PROP_0,
  PROP_TITLE,
  PROP_NAME,
  PROP_BANK,
  PROP_PROGRAM
};

static void ipatch_vbank_inst_finalize (GObject *gobject);
static void ipatch_vbank_inst_set_property (GObject *object,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void ipatch_vbank_inst_get_property (GObject *object,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void ipatch_vbank_inst_item_copy (IpatchItem *dest, IpatchItem *src,
					 IpatchItemCopyLinkFunc link_func,
					 gpointer user_data);
static const GType *ipatch_vbank_inst_container_child_types (void);
static gboolean
ipatch_vbank_inst_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type);


G_DEFINE_TYPE (IpatchVBankInst, ipatch_vbank_inst, IPATCH_TYPE_CONTAINER);

static GType inst_child_types[2] = { 0 };
static GParamSpec *name_pspec, *bank_pspec, *program_pspec;



static void
ipatch_vbank_inst_class_init (IpatchVBankInstClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  obj_class->finalize = ipatch_vbank_inst_finalize;
  obj_class->get_property = ipatch_vbank_inst_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_vbank_inst_set_property;
  item_class->copy = ipatch_vbank_inst_item_copy;

  container_class->child_types = ipatch_vbank_inst_container_child_types;
  container_class->init_iter = ipatch_vbank_inst_container_init_iter;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  name_pspec =
    ipatch_param_set (g_param_spec_string ("name", _("Name"), _("Name"),
		      NULL, G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE),
		      "string-max-length", IPATCH_VBANK_INST_NAME_SIZE,
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

  inst_child_types[0] = IPATCH_TYPE_VBANK_REGION;
}

static void
ipatch_vbank_inst_init (IpatchVBankInst *inst)
{
}

static void
ipatch_vbank_inst_finalize (GObject *gobject)
{
  IpatchVBankInst *inst = IPATCH_VBANK_INST (gobject);

  IPATCH_ITEM_WLOCK (inst);

  g_free (inst->name);
  inst->name = NULL;

  IPATCH_ITEM_WUNLOCK (inst);

  if (G_OBJECT_CLASS (ipatch_vbank_inst_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_vbank_inst_parent_class)->finalize (gobject);
}

static void
ipatch_vbank_inst_get_title (IpatchVBankInst *inst, GValue *value)
{
  int bank, program;
  char *name, *s;

  g_object_get (inst,
		"bank", &bank,
		"program", &program,
		"name", &name,
		NULL);
  s = g_strdup_printf ("%03d-%03d %s", bank, program, name);
  g_free (name);

  g_value_take_string (value, s);
}

static void
ipatch_vbank_inst_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  IpatchVBankInst *inst = IPATCH_VBANK_INST (object);

  switch (property_id)
  {
    case PROP_NAME:
      IPATCH_ITEM_WLOCK (inst);
      g_free (inst->name);
      inst->name = g_value_dup_string (value);
      IPATCH_ITEM_WUNLOCK (inst);
      break;
    case PROP_BANK:
      inst->bank = g_value_get_int (value);
      break;
    case PROP_PROGRAM:
      inst->program = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
  }

  /* need to do title notify? */
  if (property_id == PROP_NAME || property_id == PROP_BANK
      || property_id == PROP_PROGRAM)
  {
    GValue titleval = { 0 };

    g_value_init (&titleval, G_TYPE_STRING);
    ipatch_vbank_inst_get_title (inst, &titleval);
    ipatch_item_prop_notify ((IpatchItem *)inst, ipatch_item_pspec_title,
			     &titleval, NULL);
    g_value_unset (&titleval);
  }
}

static void
ipatch_vbank_inst_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  IpatchVBankInst *inst = IPATCH_VBANK_INST (object);

  switch (property_id)
  {
    case PROP_TITLE:
      ipatch_vbank_inst_get_title (inst, value);
      break;
    case PROP_NAME:
      IPATCH_ITEM_RLOCK (inst);
      g_value_set_string (value, inst->name);
      IPATCH_ITEM_RUNLOCK (inst);
      break;
    case PROP_BANK:
      g_value_set_int (value, inst->bank);
      break;
    case PROP_PROGRAM:
      g_value_set_int (value, inst->program);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ipatch_vbank_inst_item_copy (IpatchItem *dest, IpatchItem *src,
			     IpatchItemCopyLinkFunc link_func,
			     gpointer user_data)
{
  IpatchVBankInst *src_inst, *dest_inst;
  IpatchItem *region;
  GSList *p;

  src_inst = IPATCH_VBANK_INST (src);
  dest_inst = IPATCH_VBANK_INST (dest);

  IPATCH_ITEM_RLOCK (src_inst);

  dest_inst->name = g_strdup (src_inst->name);
  dest_inst->program = src_inst->program;
  dest_inst->bank = src_inst->bank;

  for (p = src_inst->regions; p; p = p->next)
  {
    region = ipatch_item_duplicate (IPATCH_ITEM (p->data));
    dest_inst->regions = g_slist_prepend (dest_inst->regions, region);
    ipatch_item_set_parent (region, IPATCH_ITEM (dest_inst));
  }

  IPATCH_ITEM_RUNLOCK (src_inst);

  dest_inst->regions = g_slist_reverse (dest_inst->regions);
}

static const GType *
ipatch_vbank_inst_container_child_types (void)
{
  return (inst_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_vbank_inst_container_init_iter (IpatchContainer *container,
				       IpatchIter *iter, GType type)
{
  IpatchVBankInst *inst = IPATCH_VBANK_INST (container);

  if (!g_type_is_a (type, IPATCH_TYPE_VBANK_REGION))
  {
    g_critical ("Invalid child type '%s' for parent of type '%s'",
		g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
    return (FALSE);
  }

  ipatch_iter_GSList_init (iter, &inst->regions);
  return (TRUE);
}

/**
 * ipatch_vbank_inst_new:
 *
 * Create a new virtual bank instrument object.
 *
 * Returns: New VBank instrument with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item.
 */
IpatchVBankInst *
ipatch_vbank_inst_new (void)
{
  return (IPATCH_VBANK_INST (g_object_new (IPATCH_TYPE_VBANK_INST, NULL)));
}

/**
 * ipatch_vbank_inst_first: (skip)
 * @iter: Patch item iterator containing #IpatchVBankInst items
 *
 * Gets the first item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first instrument in @iter or %NULL if empty.
 */
IpatchVBankInst *
ipatch_vbank_inst_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_VBANK_INST (obj));
  else return (NULL);
}

/**
 * ipatch_vbank_inst_next: (skip)
 * @iter: Patch item iterator containing #IpatchVBankInst items
 *
 * Gets the next item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next instrument in @iter or %NULL if at the end of the list.
 */
IpatchVBankInst *
ipatch_vbank_inst_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_VBANK_INST (obj));
  else return (NULL);
}

/**
 * ipatch_vbank_inst_new_region:
 * @inst: VBank instrument
 * @item: Referenced item for new region
 *
 * A convenience function for creating a new virtual bank region, adding it
 * to @inst and setting the region's referenced item to @item.
 */
void
ipatch_vbank_inst_new_region (IpatchVBankInst *inst, IpatchItem *item)
{
  IpatchVBankRegion *region;

  g_return_if_fail (IPATCH_IS_VBANK_INST (inst));
  g_return_if_fail (IPATCH_IS_ITEM (item));

  region = ipatch_vbank_region_new (); /* ++ ref new region */
  g_object_set (region, "link-item", item, NULL);
  ipatch_container_append (IPATCH_CONTAINER (inst), IPATCH_ITEM (region));

  g_object_unref (region);	/* -- unref vbank region */
}

/**
 * ipatch_vbank_inst_set_midi_locale:
 * @inst: Virtual bank instrument to set MIDI locale of
 * @bank: MIDI bank number to assign to instrument
 * @program: MIDI program number to assign to instrument
 *
 * Sets the MIDI locale of an instrument (bank and program numbers).
 */
void
ipatch_vbank_inst_set_midi_locale (IpatchVBankInst *inst,
				   int bank, int program)
{
  g_object_set (inst, "bank", bank, "program", program, NULL);
}

/**
 * ipatch_vbank_inst_get_midi_locale:
 * @inst: Virtual bank instrument to get MIDI locale from
 * @bank: (out) (optional): Location to store instrument's
 *   MIDI bank number or %NULL
 * @program: (out) (optional): Location to store instrument's
 *   MIDI program number or %NULL
 *
 * Gets the MIDI locale of a virtual bank instrument (bank and program numbers).
 */
void
ipatch_vbank_inst_get_midi_locale (IpatchVBankInst *inst,
				   int *bank, int *program)
{
  g_return_if_fail (IPATCH_IS_VBANK_INST (inst));

  IPATCH_ITEM_RLOCK (inst);

  if (bank) *bank = inst->bank;
  if (program) *program = inst->program;

  IPATCH_ITEM_RUNLOCK (inst);
}

/**
 * ipatch_vbank_inst_compare:
 * @p1: First instrument in comparison
 * @p2: Second instrument in comparison
 *
 * Virtual bank instrument comparison function for sorting. Compare two
 * instruments by their MIDI bank:program numbers. Note that this function is
 * compatible with GCompareFunc and can therefore be used with g_list_sort, etc.
 *
 * Returns: Comparison result that is less than, equal to, or greater than zero
 * if @p1 is found, respectively, to be less than, to match, or be greater
 * than @p2.
 */
int
ipatch_vbank_inst_compare (const IpatchVBankInst *p1,
			   const IpatchVBankInst *p2)
{
  gint32 aval, bval;

  aval = ((gint32)(p1->bank) << 16) | p1->program;
  bval = ((gint32)(p2->bank) << 16) | p2->program;

  return (aval - bval);
}
