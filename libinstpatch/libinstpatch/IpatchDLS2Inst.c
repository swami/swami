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
 * SECTION: IpatchDLS2Inst
 * @short_description: DLS instrument object
 * @see_also: 
 * @stability: Stable
 *
 * Defines a DLS instrument object.  DLS instruments are the toplevel objects
 * in the DLS instrument file tree hierarchy.
 */
#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2Inst.h"
#include "IpatchDLS2Conn.h"
#include "IpatchDLS2Region.h"
#include "IpatchDLSFile.h"
#include "IpatchDLSFile_priv.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "ipatch_priv.h"

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_BANK,
  PROP_PROGRAM,
  PROP_PERCUSSION
};

static void ipatch_dls2_inst_class_init (IpatchDLS2InstClass *klass);
static void ipatch_dls2_inst_finalize (GObject *gobject);
static void get_title (IpatchDLS2Inst *inst, GValue *value);
static void ipatch_dls2_inst_set_property (GObject *object,
					   guint property_id,
					   const GValue *value,
					   GParamSpec *pspec);
static void ipatch_dls2_inst_get_property (GObject *object,
					   guint property_id,
					   GValue *value,
					   GParamSpec *pspec);
static void ipatch_dls2_inst_item_copy (IpatchItem *dest, IpatchItem *src,
					IpatchItemCopyLinkFunc link_func,
					gpointer user_data);

static const GType *ipatch_dls2_inst_container_child_types (void);
static gboolean
ipatch_dls2_inst_container_init_iter (IpatchContainer *container,
				      IpatchIter *iter, GType type);

static gpointer parent_class = NULL;
static GType inst_child_types[2] = { 0 };


GType
ipatch_dls2_inst_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchDLS2InstClass), NULL, NULL,
      (GClassInitFunc)ipatch_dls2_inst_class_init, NULL, NULL,
      sizeof (IpatchDLS2Inst), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_CONTAINER,
					"IpatchDLS2Inst", &item_info, 0);
  }

  return (item_type);
}

static void
ipatch_dls2_inst_class_init (IpatchDLS2InstClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);
  GParamSpec *pspec;

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_dls2_inst_finalize;
  obj_class->get_property = ipatch_dls2_inst_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_dls2_inst_set_property;
  item_class->copy = ipatch_dls2_inst_item_copy;

  container_class->child_types = ipatch_dls2_inst_container_child_types;
  container_class->init_iter = ipatch_dls2_inst_container_init_iter;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  /* bank and program numbers are flagged as UNIQUE properties and are
     grouped together (i.e., siblings with same bank/preset are considered
     conflicting) */

  pspec = g_param_spec_int ("bank", _("Bank"),
			    _("MIDI bank number"),
			    0, IPATCH_DLS2_INST_BANK_MAX, 0,
			    G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE);
  ipatch_param_set (pspec, "unique-group-id", 1, NULL);
  g_object_class_install_property (obj_class, PROP_BANK, pspec);

  pspec = g_param_spec_int ("program", _("Program"),
			    _("MIDI program number"),
			    0, 127, 0,
			    G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE);
  ipatch_param_set (pspec, "unique-group-id", 1, NULL);
  g_object_class_install_property (obj_class, PROP_PROGRAM, pspec);

  g_object_class_install_property (obj_class, PROP_PERCUSSION,
	g_param_spec_boolean ("percussion", _("Percussion"),
			      _("Percussion instrument?"), FALSE,
			      G_PARAM_READWRITE));

  ipatch_dls2_info_install_class_properties (obj_class);

  inst_child_types[0] = IPATCH_TYPE_DLS2_REGION;
}

static void
ipatch_dls2_inst_finalize (GObject *gobject)
{
  IpatchDLS2Inst *inst = IPATCH_DLS2_INST (gobject);
  GSList *p;

  IPATCH_ITEM_WLOCK (inst);

  ipatch_dls2_info_free (inst->info);
  inst->info = NULL;

  p = inst->conns;
  while (p)
    {
      ipatch_dls2_conn_free ((IpatchDLS2Conn *)(p->data));
      p = g_slist_delete_link (p, p);
    }
  inst->conns = NULL;

  g_free (inst->dlid);
  inst->dlid = NULL;

  IPATCH_ITEM_WUNLOCK (inst);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
get_title (IpatchDLS2Inst *inst, GValue *value)
{
  char *name;
  char *s;

  g_object_get (inst, "name", &name, NULL);

  if (name)
    {
      s = g_strdup_printf ("%05d-%03d %s", inst->bank, inst->program, name);
      g_free (name);
    }
  else s = g_strdup_printf ("%05d-%03d %s", inst->bank, inst->program,
			    IPATCH_UNTITLED);

  g_value_take_string (value, s);
}

static void
ipatch_dls2_inst_set_property (GObject *object, guint property_id,
			       const GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Inst *inst = IPATCH_DLS2_INST (object);
  gboolean retval;

  switch (property_id)
    {
    case PROP_BANK:
      IPATCH_ITEM_WLOCK (inst);	/* locked to ensure bank/program atomicity */
      inst->bank = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (inst);
      break;
    case PROP_PROGRAM:
      IPATCH_ITEM_WLOCK (inst);	/* locked to ensure bank/program atomicity */
      inst->program = g_value_get_int (value);
      IPATCH_ITEM_WUNLOCK (inst);
      break;
    case PROP_PERCUSSION:
      if (g_value_get_boolean (value))
	ipatch_item_set_flags ((IpatchItem *)inst, IPATCH_DLS2_INST_PERCUSSION);
      else
	ipatch_item_clear_flags ((IpatchItem *)inst, IPATCH_DLS2_INST_PERCUSSION);
      break;
    default:
      IPATCH_ITEM_WLOCK (inst);
      retval = ipatch_dls2_info_set_property (&inst->info, property_id, value);
      IPATCH_ITEM_WUNLOCK (inst);

      if (!retval)
	{
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
      break;
    }

  /* does title property need to be notified? */
  if (property_id == PROP_BANK || property_id == PROP_PROGRAM
      || property_id == IPATCH_DLS2_NAME)
    {
      GValue newval = { 0 };

      g_value_init (&newval, G_TYPE_STRING);
      get_title (inst, &newval);
      ipatch_item_prop_notify ((IpatchItem *)inst, ipatch_item_pspec_title,
			       &newval, NULL);
      g_value_unset (&newval);
    }
}

static void
ipatch_dls2_inst_get_property (GObject *object, guint property_id,
			       GValue *value, GParamSpec *pspec)
{
  IpatchDLS2Inst *inst = IPATCH_DLS2_INST (object);
  gboolean retval;

  switch (property_id)
    {
    case PROP_TITLE:
      get_title (inst, value);
      break;
    case PROP_BANK:
      g_value_set_int (value, inst->bank);
      break;
    case PROP_PROGRAM:
      g_value_set_int (value, inst->program);
      break;
    case PROP_PERCUSSION:
      g_value_set_boolean (value, (ipatch_item_get_flags ((IpatchItem *)inst)
				   & IPATCH_DLS2_INST_PERCUSSION) != 0);
      break;
    default:
      IPATCH_ITEM_RLOCK (inst);
      retval = ipatch_dls2_info_get_property (inst->info, property_id, value);
      IPATCH_ITEM_RUNLOCK (inst);

      if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_dls2_inst_item_copy (IpatchItem *dest, IpatchItem *src,
			    IpatchItemCopyLinkFunc link_func,
			    gpointer user_data)
{
  IpatchDLS2Inst *src_inst, *dest_inst;
  IpatchItem *ritem;
  GSList *p;

  src_inst = IPATCH_DLS2_INST (src);
  dest_inst = IPATCH_DLS2_INST (dest);

  IPATCH_ITEM_RLOCK (src_inst);

  /* set the percussion flag if set in src */
  if (ipatch_item_get_flags (src_inst) & IPATCH_DLS2_INST_PERCUSSION)
    ipatch_item_set_flags (dest_inst, IPATCH_DLS2_INST_PERCUSSION);

  dest_inst->bank = src_inst->bank;
  dest_inst->program = src_inst->program;

  dest_inst->info = ipatch_dls2_info_duplicate (src_inst->info);

  if (src_inst->dlid)
    dest_inst->dlid = g_memdup (src_inst->dlid, IPATCH_DLS_DLID_SIZE);

  p = src_inst->regions;
  while (p)			/* duplicate regions */
    { /* ++ ref new duplicate item */
      ritem = ipatch_item_duplicate_link_func (IPATCH_ITEM (p->data), link_func,
					       user_data);
      g_return_if_fail (ritem != NULL);
    
      /* !! take over duplicate item reference */
      dest_inst->regions = g_slist_prepend (dest_inst->regions, ritem);
      ipatch_item_set_parent (ritem, IPATCH_ITEM (dest_inst));

      p = g_slist_next (p);
    }

  dest_inst->conns = ipatch_dls2_conn_list_duplicate (src_inst->conns);

  IPATCH_ITEM_RUNLOCK (src_inst);

  dest_inst->regions = g_slist_reverse (dest_inst->regions);
}

static const GType *
ipatch_dls2_inst_container_child_types (void)
{
  return (inst_child_types);
}

/* container is locked by caller */
static gboolean
ipatch_dls2_inst_container_init_iter (IpatchContainer *container,
				      IpatchIter *iter, GType type)
{
  IpatchDLS2Inst *inst = IPATCH_DLS2_INST (container);

  if (!g_type_is_a (type, IPATCH_TYPE_DLS2_REGION))
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  ipatch_iter_GSList_init (iter, &inst->regions);
  return (TRUE);
}

/**
 * ipatch_dls2_inst_new:
 *
 * Create a new DLS instrument object.
 *
 * Returns: New DLS instrument with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item, unless another
 * reference is added (if its parented for example).
 */
IpatchDLS2Inst *
ipatch_dls2_inst_new (void)
{
  return (IPATCH_DLS2_INST (g_object_new (IPATCH_TYPE_DLS2_INST, NULL)));
}

/**
 * ipatch_dls2_inst_first: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Inst items
 *
 * Gets the first item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_first().
 *
 * Returns: The first instrument in @iter or %NULL if empty.
 */
IpatchDLS2Inst *
ipatch_dls2_inst_first (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_first (iter);
  if (obj) return (IPATCH_DLS2_INST (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_inst_next: (skip)
 * @iter: Patch item iterator containing #IpatchDLS2Inst items
 *
 * Gets the next item in an instrument iterator. A convenience wrapper for
 * ipatch_iter_next().
 *
 * Returns: The next instrument in @iter or %NULL if at the end of the list.
 */
IpatchDLS2Inst *
ipatch_dls2_inst_next (IpatchIter *iter)
{
  GObject *obj;
  g_return_val_if_fail (iter != NULL, NULL);

  obj = ipatch_iter_next (iter);
  if (obj) return (IPATCH_DLS2_INST (obj));
  else return (NULL);
}

/**
 * ipatch_dls2_inst_get_info:
 * @inst: DLS instrument to get info from
 * @fourcc: FOURCC integer id of INFO to get
 *
 * Get a DLS instrument info string by FOURCC integer ID (integer
 * representation of a 4 character RIFF chunk ID, see
 * #IpatchRiff).
 *
 * Returns: (transfer full): New allocated info string value or %NULL if no info with the
 * given @fourcc ID. String should be freed when finished with it.
 */
char *
ipatch_dls2_inst_get_info (IpatchDLS2Inst *inst, guint32 fourcc)
{
  char *val;

  g_return_val_if_fail (IPATCH_IS_DLS2_INST (inst), NULL);

  IPATCH_ITEM_RLOCK (inst);
  val = ipatch_dls2_info_get (inst->info, fourcc);
  IPATCH_ITEM_RUNLOCK (inst);

  return (val);
}

/**
 * ipatch_dls2_inst_set_info:
 * @inst: DLS instrument to set info of
 * @fourcc: FOURCC integer ID of INFO to set
 * @val: (nullable): Value to set info to or %NULL to unset (clear) info.
 *
 * Sets an INFO value in a DLS instrument object.
 * Emits changed signal.
 */
void
ipatch_dls2_inst_set_info (IpatchDLS2Inst *inst, guint32 fourcc,
			   const char *val)
{
  GValue newval = { 0 }, oldval = { 0 };

  g_return_if_fail (IPATCH_IS_DLS2_INST (inst));

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, val);

  g_value_init (&oldval, G_TYPE_STRING);
  g_value_take_string (&oldval, ipatch_dls2_inst_get_info (inst, fourcc));

  IPATCH_ITEM_WLOCK (inst);
  ipatch_dls2_info_set (&inst->info, fourcc, val);
  IPATCH_ITEM_WUNLOCK (inst);

  ipatch_dls2_info_notify ((IpatchItem *)inst, fourcc, &newval, &oldval);

  /* does title property need to be notified? */
  if (fourcc == IPATCH_DLS2_NAME)
    ipatch_item_prop_notify ((IpatchItem *)inst, ipatch_item_pspec_title,
			     &newval, &oldval);
  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_dls2_inst_set_midi_locale:
 * @inst: DLS instrument to set MIDI locale of
 * @bank: MIDI bank number to assign to instrument
 * @program: MIDI program number to assign to instrument
 *
 * Sets the MIDI locale of a DLS instrument (bank and program numbers).
 */
void
ipatch_dls2_inst_set_midi_locale (IpatchDLS2Inst *inst, int bank, int program)
{
  g_object_set (inst, "bank", bank, "program", program, NULL);
}

/**
 * ipatch_dls2_inst_get_midi_locale:
 * @inst: Instrument to get MIDI locale from
 * @bank: (out) (optional): Location to store instrument's MIDI bank number or %NULL
 * @program: (out) (optional): Location to store instrument's MIDI program number or %NULL
 *
 * Gets the MIDI locale of a DLS instrument (bank and program numbers).
 */
void
ipatch_dls2_inst_get_midi_locale (IpatchDLS2Inst *inst,
				  int *bank, int *program)
{
  g_return_if_fail (IPATCH_IS_DLS2_INST (inst));

  IPATCH_ITEM_RLOCK (inst);
  if (bank) *bank = inst->bank;
  if (program) *program = inst->program;
  IPATCH_ITEM_RUNLOCK (inst);
}

/**
 * ipatch_dls2_inst_compare:
 * @p1: First instrument in comparison
 * @p2: Second instrument in comparison
 *
 * Instrument comparison function for sorting. Compare two instruments by their
 * MIDI bank:program numbers. Note that this function is compatible with
 * GCompareFunc and can therefore be used with g_list_sort, etc.
 *
 * FIXME: Also note that percussion instruments are sorted after regular ones.
 *
 * Returns: Comparison result that is less than, equal to, or greater than zero
 * if @p1 is found, respectively, to be less than, to match, or be greater
 * than @p2.
 */
int
ipatch_dls2_inst_compare (const IpatchDLS2Inst *p1,
			  const IpatchDLS2Inst *p2)
{
  gint32 aval, bval;
  guint32 aperc, bperc;

  aperc = (ipatch_item_get_flags ((IpatchItem *)p1)
	   & IPATCH_DLS2_INST_PERCUSSION) ? 1 << 31 : 0;
  bperc = (ipatch_item_get_flags ((IpatchItem *)p2)
	   & IPATCH_DLS2_INST_PERCUSSION) ? 1 << 31 : 0;

  aval = aperc | ((gint32)(p1->bank) << 16) | p1->program;
  bval = bperc | ((gint32)(p2->bank) << 16) | p2->program;

  return (aval - bval);
}

/**
 * ipatch_dls2_inst_get_conns:
 * @inst: Instrument to get connections from
 *
 * Gets a list of connections from a DLS instrument. List should be freed with
 * ipatch_dls2_conn_list_free() (free_conns set to %TRUE) when finished
 * with it.
 *
 * Returns: (element-type IpatchDLS2Conn) (transfer full): New list of connections
 *   (#IpatchDLS2Conn) in @inst or %NULL if no connections. Remember to free it
 *   when finished.
 */
GSList *
ipatch_dls2_inst_get_conns (IpatchDLS2Inst *inst)
{
  GSList *newlist;

  g_return_val_if_fail (IPATCH_IS_DLS2_INST (inst), NULL);

  IPATCH_ITEM_RLOCK (inst);
  newlist = ipatch_dls2_conn_list_duplicate (inst->conns);
  IPATCH_ITEM_RUNLOCK (inst);

  return (newlist);
}

/**
 * ipatch_dls2_inst_set_conn:
 * @inst: DLS instrument
 * @conn: Connection
 *
 * Set a global DLS connection in an instrument. See
 * ipatch_dls2_conn_list_set() for more details.
 */
void
ipatch_dls2_inst_set_conn (IpatchDLS2Inst *inst, const IpatchDLS2Conn *conn)
{
  g_return_if_fail (IPATCH_IS_DLS2_INST (inst));
  g_return_if_fail (conn != NULL);

  IPATCH_ITEM_WLOCK (inst);
  ipatch_dls2_conn_list_set (&inst->conns, conn);
  IPATCH_ITEM_WUNLOCK (inst);
}

/**
 * ipatch_dls2_inst_unset_conn:
 * @inst: DLS instrument
 * @conn: Connection
 *
 * Remove a global DLS connection from an instrument. See
 * ipatch_dls2_conn_list_unset() for more details.
 */
void
ipatch_dls2_inst_unset_conn (IpatchDLS2Inst *inst, const IpatchDLS2Conn *conn)
{
  g_return_if_fail (IPATCH_IS_DLS2_INST (inst));
  g_return_if_fail (conn != NULL);

  IPATCH_ITEM_WLOCK (inst);
  ipatch_dls2_conn_list_unset (&inst->conns, conn);
  IPATCH_ITEM_WUNLOCK (inst);
}

/**
 * ipatch_dls2_inst_unset_all_conns:
 * @inst: DLS instrument
 *
 * Remove all global connections in an instrument.
 */
void
ipatch_dls2_inst_unset_all_conns (IpatchDLS2Inst *inst)
{
  g_return_if_fail (IPATCH_IS_DLS2_INST (inst));

  IPATCH_ITEM_WLOCK (inst);
  ipatch_dls2_conn_list_free (inst->conns, TRUE);
  inst->conns = NULL;
  IPATCH_ITEM_WUNLOCK (inst);
}

/**
 * ipatch_dls2_inst_conn_count:
 * @inst: Instrument to count connections in
 *
 * Count number of connections in a instrument
 *
 * Returns: Count of connections
 */
guint
ipatch_dls2_inst_conn_count (IpatchDLS2Inst *inst)
{
  guint i;

  IPATCH_ITEM_RLOCK (inst);
  i = g_slist_length (inst->conns);
  IPATCH_ITEM_RUNLOCK (inst);

  return (i);
}
