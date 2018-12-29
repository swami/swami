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
 * SECTION: IpatchDLS2
 * @short_description: DLS version 2 instrument file object
 * @see_also: 
 * @stability: Stable
 *
 * Object type for DLS version 2 format instruments.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2.h"
#include "IpatchDLSFile.h"
#include "IpatchDLS2Info.h"
#include "IpatchDLS2Region.h"
#include "IpatchTypeProp.h"
#include "IpatchVirtualContainer_types.h"
#include "version.h"
#include "ipatch_priv.h"

/* non-INFO properties (INFO properties use their FOURCC int values) */
enum
{
  PROP_0,
  PROP_VERSION
};

static void ipatch_dls2_class_init (IpatchDLS2Class *klass);
static void ipatch_dls2_init (IpatchDLS2 *dls);
static void ipatch_dls2_finalize (GObject *gobject);
static void ipatch_dls2_set_property (GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec);
static void ipatch_dls2_get_property (GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec);
static void ipatch_dls2_item_copy (IpatchItem *dest, IpatchItem *src,
				   IpatchItemCopyLinkFunc link_func,
				   gpointer user_data);

static const GType *ipatch_dls2_container_child_types (void);
static const GType *ipatch_dls2_container_virtual_types (void);
static gboolean ipatch_dls2_container_init_iter (IpatchContainer *container,
						 IpatchIter *iter, GType type);
static void ipatch_dls2_container_make_unique (IpatchContainer *container,
					       IpatchItem *item);
static void ipatch_dls2_base_find_unused_locale (IpatchBase *base, int *bank,
						 int *program,
						 const IpatchItem *exclude,
						 gboolean percussion);
static int locale_gcompare_func (gconstpointer a, gconstpointer b);
static IpatchItem *
ipatch_dls2_base_find_item_by_locale (IpatchBase *base, int bank, int program);

static gpointer parent_class = NULL;
static GType dls2_child_types[3] = { 0 };
static GType dls2_virt_types[4] = { 0 };

/* for chaining to original file-name property */
static IpatchItemClass *base_item_class;



/* SoundFont item type creation function */
GType
ipatch_dls2_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchDLS2Class), NULL, NULL,
      (GClassInitFunc)ipatch_dls2_class_init, NULL, NULL,
      sizeof (IpatchDLS2),
      0,
      (GInstanceInitFunc)ipatch_dls2_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_BASE, "IpatchDLS2",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_dls2_class_init (IpatchDLS2Class *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);
  IpatchBaseClass *base_class = IPATCH_BASE_CLASS (klass);

  /* save original base class for chaining file-name property */
  base_item_class = IPATCH_ITEM_CLASS (g_type_class_ref (IPATCH_TYPE_BASE));

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_dls2_finalize;
  obj_class->get_property = ipatch_dls2_get_property;

  /* we use the IpatchItem item_set_property method */
  item_class->item_set_property = ipatch_dls2_set_property;
  item_class->copy = ipatch_dls2_item_copy;

  container_class->child_types = ipatch_dls2_container_child_types;
  container_class->virtual_types = ipatch_dls2_container_virtual_types;
  container_class->init_iter = ipatch_dls2_container_init_iter;
  container_class->make_unique = ipatch_dls2_container_make_unique;

  base_class->find_unused_locale = ipatch_dls2_base_find_unused_locale;
  base_class->find_item_by_locale = ipatch_dls2_base_find_item_by_locale;

  /* non INFO properties */
  g_object_class_override_property (obj_class, IPATCH_DLS2_NAME, "title");

  g_object_class_install_property (obj_class, PROP_VERSION,
		    g_param_spec_string ("version", _("Version"),
					 _("File version \"n.n.n.n\""),
					 NULL,
					 G_PARAM_READWRITE));

  ipatch_dls2_info_install_class_properties (obj_class);

  dls2_child_types[0] = IPATCH_TYPE_DLS2_INST;
  dls2_child_types[1] = IPATCH_TYPE_DLS2_SAMPLE;

  dls2_virt_types[0] = IPATCH_TYPE_VIRTUAL_DLS2_MELODIC;
  dls2_virt_types[1] = IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION;
  dls2_virt_types[2] = IPATCH_TYPE_VIRTUAL_DLS2_SAMPLES;
}

static void
ipatch_dls2_init (IpatchDLS2 *dls)
{
  g_object_set (dls,
		"name", _(IPATCH_BASE_DEFAULT_NAME),
		"software", "libInstPatch v" IPATCH_VERSION,
		NULL);

  ipatch_item_clear_flags (IPATCH_ITEM (dls), IPATCH_BASE_CHANGED);
}

/* function called when SoundFont is being destroyed */
static void
ipatch_dls2_finalize (GObject *gobject)
{
  IpatchDLS2 *dls = IPATCH_DLS2 (gobject);

  IPATCH_ITEM_WLOCK (dls);

  ipatch_dls2_info_free (dls->info);
  dls->info = NULL;

  g_free (dls->dlid);
  dls->dlid = NULL;

  IPATCH_ITEM_WUNLOCK (dls);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
ipatch_dls2_set_property (GObject *object, guint property_id,
			  const GValue *value, GParamSpec *pspec)
{
  IpatchDLS2 *dls = IPATCH_DLS2 (object);
  gboolean retval;

  if (property_id == PROP_VERSION)
    {
      const char *verstr;
      guint16 msu, msl, lsu, lsl;

      verstr = g_value_get_string (value);
      if (verstr && sscanf (verstr, "%hu.%hu.%hu.%hu",
			    &msu, &msl, &lsu, &lsl) != 4)
	{
	  g_warning ("Version property parse error");
	  return;
	}

      IPATCH_ITEM_WLOCK (dls);

      if (verstr)
	{
	  ipatch_item_set_flags (dls, IPATCH_DLS2_VERSION_SET);
	  dls->ms_version = (guint32)msu << 16 || msl;
	  dls->ls_version = (guint32)lsu << 16 || lsl;
	}
      else ipatch_item_clear_flags (dls, IPATCH_DLS2_VERSION_SET);

      IPATCH_ITEM_WUNLOCK (dls);
    }
  else
    {
      IPATCH_ITEM_WLOCK (dls);
      retval = ipatch_dls2_info_set_property (&dls->info, property_id, value);
      IPATCH_ITEM_WUNLOCK (dls);

      /* check of "title" property needs to be notified, values do not need
	 to be sent, since its a read only property */
      if (property_id == IPATCH_DLS2_NAME)
	ipatch_item_prop_notify ((IpatchItem *)dls, ipatch_item_pspec_title,
				 value, NULL);

      if (!retval)
	{
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  return;
	}
    }
}

static void
ipatch_dls2_get_property (GObject *object, guint property_id,
			  GValue *value, GParamSpec *pspec)
{
  IpatchDLS2 *dls;
  gboolean retval;
  char *s;

  g_return_if_fail (IPATCH_IS_DLS2 (object));
  dls = IPATCH_DLS2 (object);

  if (property_id == PROP_VERSION)
    {
      gboolean version_set;
      guint32 ms, ls;

      IPATCH_ITEM_RLOCK (dls);

      version_set = (ipatch_item_get_flags (dls) & IPATCH_DLS2_VERSION_SET) > 0;
      ms = dls->ms_version;
      ls = dls->ls_version;

      IPATCH_ITEM_RUNLOCK (dls);

      if (version_set)
	{
	  s = g_strdup_printf ("%u.%u.%u.%u", ms >> 16, ms & 0xFFFF,
			       ls >> 16, ls & 0xFFFF);
	  g_value_take_string (value, s);
	}
      else g_value_set_string (value, NULL);
    }
  else				/* INFO property or invalid */
    {
      IPATCH_ITEM_RLOCK (dls);
      retval = ipatch_dls2_info_get_property (dls->info, property_id, value);
      IPATCH_ITEM_RUNLOCK (dls);

      if (!retval)
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ipatch_dls2_item_copy (IpatchItem *dest, IpatchItem *src,
		       IpatchItemCopyLinkFunc link_func, gpointer user_data)
{
  IpatchDLS2 *src_dls, *dest_dls;
  IpatchItem *newitem;
  GHashTable *repl_samples;
  GSList *p;

  src_dls = IPATCH_DLS2 (src);
  dest_dls = IPATCH_DLS2 (dest);

  /* create item replacement hash */
  repl_samples = g_hash_table_new (NULL, NULL);

  IPATCH_ITEM_RLOCK (src_dls);

  dest_dls->ms_version = src_dls->ms_version;
  dest_dls->ls_version = src_dls->ls_version;

  if (ipatch_item_get_flags (src_dls) & IPATCH_DLS2_VERSION_SET)
    ipatch_item_set_flags (dest_dls, IPATCH_DLS2_VERSION_SET);

  if (IPATCH_BASE (src_dls)->file)
    ipatch_base_set_file (IPATCH_BASE (dest_dls), IPATCH_BASE (src_dls)->file);

  dest_dls->info = ipatch_dls2_info_duplicate (src_dls->info);

  if (src_dls->dlid)
    dest_dls->dlid = g_memdup (src_dls->dlid, IPATCH_DLS_DLID_SIZE);

  p = src_dls->samples;
  while (p)			/* duplicate samples */
    {
      newitem = ipatch_item_duplicate ((IpatchItem *)(p->data));
      g_return_if_fail (newitem != NULL);

      dest_dls->samples = g_slist_prepend (dest_dls->samples, newitem);
      ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_dls));

      /* add to sample pointer replacement hash */
      g_hash_table_insert (repl_samples, p->data, newitem);

      /* FIXME - phase linked groups? */

      p = g_slist_next (p);
    }

  p = src_dls->insts;
  while (p)			/* duplicate instruments */
    {
      newitem = ipatch_item_duplicate_replace ((IpatchItem *)(p->data),
					       repl_samples);
      g_return_if_fail (newitem != NULL);

      dest_dls->insts = g_slist_prepend (dest_dls->insts, newitem);
      ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_dls));

      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (src_dls);

  dest_dls->insts = g_slist_reverse (dest_dls->insts);
  dest_dls->samples = g_slist_reverse (dest_dls->samples);

  g_hash_table_destroy (repl_samples);
}

static const GType *
ipatch_dls2_container_child_types (void)
{
  return (dls2_child_types);
}

static const GType *
ipatch_dls2_container_virtual_types (void)
{
  return (dls2_virt_types);
}

/* container is locked by caller */
static gboolean
ipatch_dls2_container_init_iter (IpatchContainer *container,
				 IpatchIter *iter, GType type)
{
  IpatchDLS2 *dls = IPATCH_DLS2 (container);

  if (g_type_is_a (type, IPATCH_TYPE_DLS2_INST))
    ipatch_iter_GSList_init (iter, &dls->insts);
  else if (g_type_is_a (type, IPATCH_TYPE_DLS2_SAMPLE))
    ipatch_iter_GSList_init (iter, &dls->samples);
  else
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  return (TRUE);
}

static void
ipatch_dls2_container_make_unique (IpatchContainer *container,
				   IpatchItem *item)
{
  IpatchDLS2 *dls = IPATCH_DLS2 (container);
  gboolean perc;
  char *name, *newname;

  IPATCH_ITEM_WLOCK (dls);

  if (IPATCH_IS_DLS2_INST (item))
    {
      int bank, newbank, program, newprogram;
      ipatch_dls2_inst_get_midi_locale (IPATCH_DLS2_INST (item),
					&bank, &program);
      newbank = bank;
      newprogram = program;

      perc = (ipatch_item_get_flags (item) & IPATCH_DLS2_INST_PERCUSSION) != 0;

      ipatch_base_find_unused_midi_locale (IPATCH_BASE (dls),
					   &newbank, &newprogram, item, perc);

      if (bank != newbank || program != newprogram)
	ipatch_dls2_inst_set_midi_locale (IPATCH_DLS2_INST (item),
					  newbank, newprogram);
    }
  else if (!IPATCH_IS_DLS2_SAMPLE (item))
    {
      g_critical ("Invalid child type '%s' for IpatchDLS2 object",
		  g_type_name (G_TYPE_FROM_INSTANCE (item)));
    IPATCH_ITEM_WUNLOCK (dls);
      return;
    }

  g_object_get (item, "name", &name, NULL);
  newname = ipatch_dls2_make_unique_name (dls, G_TYPE_FROM_INSTANCE (item),
					  name, NULL);
  if (!name || strcmp (name, newname) != 0)
    g_object_set (item, "name", newname, NULL);

  IPATCH_ITEM_WUNLOCK (dls);

  g_free (name);
  g_free (newname);
}

/* base method to find an unused MIDI bank:program locale */
static void
ipatch_dls2_base_find_unused_locale (IpatchBase *base, int *bank,
				     int *program, const IpatchItem *exclude,
				     gboolean percussion)
{
  IpatchDLS2 *dls = IPATCH_DLS2 (base);
  GSList *locale_list = NULL;
  IpatchDLS2Inst *inst;
  GSList *p;
  int b, n;			/* Stores current bank and preset number */
  guint32 lbank, lprogram;

  /* fill array with bank and preset numbers */
  IPATCH_ITEM_RLOCK (dls);
  p = dls->insts;
  while (p)
    {
      inst = (IpatchDLS2Inst *)(p->data);

      /* only add to locale list if not the exclude item */
      if ((gpointer)inst != (gpointer)exclude)
	locale_list = g_slist_prepend (locale_list, GUINT_TO_POINTER
				       ((inst->bank << 16) | inst->program));
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (dls);

  if (!locale_list)
    {
      *program = 0;
      return;
    }

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
   ipatch_dls2_base_find_unused_locale */
static int
locale_gcompare_func (gconstpointer a, gconstpointer b)
{
  return (GPOINTER_TO_UINT (a) - GPOINTER_TO_UINT (b));
}

static IpatchItem *
ipatch_dls2_base_find_item_by_locale (IpatchBase *base, int bank, int program)
{
  IpatchDLS2Inst *inst;

  inst = ipatch_dls2_find_inst (IPATCH_DLS2 (base), NULL, bank, program, NULL);
  return ((IpatchItem *)inst);
}

/**
 * ipatch_dls2_new:
 *
 * Create a new DLS base object.
 *
 * Returns: New DLS base object with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item.
 */
IpatchDLS2 *
ipatch_dls2_new (void)
{
  return (IPATCH_DLS2 (g_object_new (IPATCH_TYPE_DLS2, NULL)));
}

/**
 * ipatch_dls2_set_file:
 * @dls: DLS to set file object of
 * @file: File object to use with the DLS object
 *
 * Sets the file object of a DLS object. DLS files are kept open
 * for sample data that references the file. This function sets a
 * DLS object's authoritive file. A convenience function, as
 * ipatch_base_set_file() does the same thing (albeit without more specific
 * type casting).
 */
void
ipatch_dls2_set_file (IpatchDLS2 *dls, IpatchDLSFile *file)
{
  g_return_if_fail (IPATCH_IS_DLS2 (dls));
  g_return_if_fail (IPATCH_IS_DLS_FILE (file));

  ipatch_base_set_file (IPATCH_BASE (dls), IPATCH_FILE (file));
}

/**
 * ipatch_dls2_get_file:
 * @dls: DLS object to get file object of
 *
 * Gets the file object of a DLS. The returned DLS file object's
 * reference count has been incremented. The caller owns the reference and is
 * responsible for removing it with g_object_unref.
 * A convenience function as ipatch_base_get_file() does the same thing
 * (albeit without more specific type casting).
 *
 * Returns: (transfer full): The DLS file object or %NULL if @dls is not open. Remember
 * to unref the file object with g_object_unref() when done with it.
 */
IpatchDLSFile *
ipatch_dls2_get_file (IpatchDLS2 *dls)
{
  IpatchFile *file;

  g_return_val_if_fail (IPATCH_IS_DLS2 (dls), NULL);

  file = ipatch_base_get_file (IPATCH_BASE (dls));
  if (file) return (IPATCH_DLS_FILE (file));
  else return (NULL);
}

/**
 * ipatch_dls2_get_info:
 * @dls: DLS to get info from
 * @fourcc: FOURCC integer id of INFO to get
 *
 * Get a DLS info string by FOURCC integer ID (integer representation of
 * a 4 character RIFF chunk ID, see #IpatchRiff).
 *
 * Returns: New allocated info string value or %NULL if no info with the
 * given @fourcc ID. String should be freed when finished with it.
 */
char *
ipatch_dls2_get_info (IpatchDLS2 *dls, guint32 fourcc)
{
  char *val;

  g_return_val_if_fail (IPATCH_IS_DLS2 (dls), NULL);

  IPATCH_ITEM_RLOCK (dls);
  val = ipatch_dls2_info_get (dls->info, fourcc);
  IPATCH_ITEM_RUNLOCK (dls);

  return (val);
}

/**
 * ipatch_dls2_set_info:
 * @dls: DLS to set info of
 * @fourcc: FOURCC integer ID of INFO to set
 * @val: (nullable): Value to set info to or %NULL to unset (clear) info.
 *
 * Sets an INFO value in a DLS object.
 *
 * Emits changed signal on DLS object.
 */
void
ipatch_dls2_set_info (IpatchDLS2 *dls, guint32 fourcc, const char *val)
{
  GValue newval = { 0 }, oldval = { 0 };

  g_return_if_fail (IPATCH_IS_DLS2 (dls));

  g_value_init (&newval, G_TYPE_STRING);
  g_value_set_static_string (&newval, val);

  g_value_init (&oldval, G_TYPE_STRING);
  g_value_take_string (&oldval, ipatch_dls2_get_info (dls, fourcc));

  IPATCH_ITEM_WLOCK (dls);
  ipatch_dls2_info_set (&dls->info, fourcc, val);
  IPATCH_ITEM_WUNLOCK (dls);

  ipatch_dls2_info_notify ((IpatchItem *)dls, fourcc, &newval, &oldval);

  /* need to do title notify? */
  if (fourcc == IPATCH_DLS2_NAME)
    ipatch_item_prop_notify ((IpatchItem *)dls, ipatch_item_pspec_title,
			     &newval, &oldval);

  g_value_unset (&oldval);
  g_value_unset (&newval);
}

/**
 * ipatch_dls2_make_unique_name:
 * @dls: DLS object
 * @child_type: A child type of @dls to search for a unique name in
 * @name: (nullable): An initial name to use or NULL
 * @exclude: (nullable): An item to exclude from search or NULL
 *
 * Generates a unique name for the given @child_type in @dls. The @name
 * parameter is used as a base and is modified, by appending a number, to
 * make it unique (if necessary). The @exclude parameter is used to exclude
 * an existing @dls child item from the search.
 *
 * MT-Note: To ensure that an item is actually unique before being
 * added to a DLS object, ipatch_container_add_unique() should be
 * used.
 *
 * Returns: A new unique name which should be freed when finished with it.
 */
char *
ipatch_dls2_make_unique_name (IpatchDLS2 *dls, GType child_type,
			      const char *name, const IpatchItem *exclude)
{
  GSList **list, *p;
  char *curname, *numptr;
  const char *temp;
  guint count = 2, info_ofs, len;

  g_return_val_if_fail (IPATCH_IS_DLS2 (dls), NULL);

  if (g_type_is_a (child_type, IPATCH_TYPE_DLS2_INST))
    {
      list = &dls->insts;
      info_ofs = G_STRUCT_OFFSET (IpatchDLS2Inst, info);
      if (!name || !*name) name = _("New Instrument");
    }
  else if (g_type_is_a (child_type, IPATCH_TYPE_DLS2_SAMPLE))
    {
      list = &dls->samples;
      info_ofs = G_STRUCT_OFFSET (IpatchDLS2Sample, info);
      if (!name || !*name) name = _("New Sample");
    }
  else
    {
      g_critical ("Invalid child type '%s' of parent type '%s'",
		  g_type_name (child_type), g_type_name (G_OBJECT_TYPE (dls)));
      return (NULL);
    }

  len = strlen (name);

  /* allocate string size + 10 chars for number + zero termination */
  curname = g_malloc0 (len + 10 + 1);
  strcpy (curname, name);	/* copy name */
  numptr = curname + len; /* pointer to end of name to concat number */

  IPATCH_ITEM_RLOCK (dls);

  p = *list;
  while (p)	/* check for duplicate */
    {
      IPATCH_ITEM_RLOCK (p->data); /* MT - Recursive LOCK */

      if (p->data != exclude)
	{
	  temp = ipatch_dls2_info_peek
	    (G_STRUCT_MEMBER (IpatchDLS2Info *, p->data, info_ofs),
	     IPATCH_DLS2_NAME);

	  if (temp && strcmp (temp, curname) == 0)
	    {			/* duplicate name */
	      IPATCH_ITEM_RUNLOCK (p->data);

	      sprintf (numptr, "%u", count++);

	      p = *list;		/* start over */
	      continue;
	    }
	}

      IPATCH_ITEM_RUNLOCK (p->data);
      p = g_slist_next (p);
    }

  IPATCH_ITEM_RUNLOCK (dls);

  curname = g_realloc (curname, strlen (curname) + 1);
  return (curname);
}

/**
 * ipatch_dls2_find_inst:
 * @dls: DLS object to search in
 * @name: (nullable): Name of instrument to find or %NULL to match any name
 * @bank: MIDI bank number of instrument to search for or -1 to not search by
 *   MIDI bank:program numbers
 * @program: MIDI program number of instrument to search for, only used
 *   if @bank is 0-128
 * @exclude: (nullable): A instrument to exclude from the search or %NULL
 *
 * Find a instrument by name or bank:program MIDI numbers. If instrument @name
 * and @bank:@program are specified then match for either condition.
 * If a instrument is found its reference count is incremented before it
 * is returned. The caller is responsible for removing the reference
 * with g_object_unref() when finished with it.
 *
 * Returns: (transfer full): The matching instrument or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchDLS2Inst *
ipatch_dls2_find_inst (IpatchDLS2 *dls, const char *name, int bank,
		       int program, const IpatchDLS2Inst *exclude)
{
  IpatchDLS2Inst *inst;
  gboolean bynum = FALSE;
  const char *temp = NULL;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_DLS2 (dls), NULL);

  /* if bank and program are valid, then search by number */
  if (bank >= 0 && program >= 0 && program < 128)
    bynum = TRUE;

  IPATCH_ITEM_RLOCK (dls);
  p = dls->insts;
  while (p)
    {
      inst = (IpatchDLS2Inst *)(p->data);
      IPATCH_ITEM_RLOCK (inst);	/* MT - Recursive LOCK */

      if (inst != exclude
	  && ((bynum && inst->bank == bank && inst->program == program)
	      || (name && (temp = ipatch_dls2_info_peek
			   (inst->info, IPATCH_DLS2_NAME))
		  && strcmp (temp, name) == 0)))
	{
	  g_object_ref (inst);
	  IPATCH_ITEM_RUNLOCK (inst);
	  IPATCH_ITEM_RUNLOCK (dls);
	  return (inst);
	}
      IPATCH_ITEM_RUNLOCK (inst);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (dls);

  return (NULL);
}

/**
 * ipatch_dls2_find_sample:
 * @dls: DLS object to search in
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
IpatchDLS2Sample *
ipatch_dls2_find_sample (IpatchDLS2 *dls, const char *name,
			 const IpatchDLS2Sample *exclude)
{
  IpatchDLS2Sample *sample;
  const char *temp = NULL;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_DLS2 (dls), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  IPATCH_ITEM_RLOCK (dls);
  p = dls->samples;
  while (p)
    {
      sample = (IpatchDLS2Sample *)(p->data);
      IPATCH_ITEM_RLOCK (sample); /* MT - Recursive LOCK */

      if (sample != exclude && (temp = ipatch_dls2_info_peek
				(sample->info, IPATCH_DLS2_NAME))
	  && strcmp (temp, name) == 0)
	{
	  g_object_ref (sample);
	  IPATCH_ITEM_RUNLOCK (sample);
	  IPATCH_ITEM_RUNLOCK (dls);
	  return (sample);
	}
      IPATCH_ITEM_RUNLOCK (sample);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (dls);

  return (NULL);
}

/**
 * ipatch_dls2_get_region_references:
 * @sample: Sample to locate referencing regions of.
 *
 * Get list of regions referencing an #IpatchDLS2Sample.
 *
 * Returns: (transfer full): New item list containing #IpatchDLS2Region objects
 * that refer to @sample. The returned list has a reference count of 1 which
 * the caller owns, unreference it when done.
 */
IpatchList *
ipatch_dls2_get_region_references (IpatchDLS2Sample *sample)
{
  IpatchDLS2 *dls;
  IpatchDLS2Region *region;
  IpatchList *refitems;
  IpatchIter iter, region_iter;
  IpatchDLS2Inst *inst;
  IpatchItem *pitem;
  gboolean success;

  g_return_val_if_fail (IPATCH_IS_DLS2_SAMPLE (sample), NULL);

  pitem = ipatch_item_get_parent (IPATCH_ITEM (sample));
  g_return_val_if_fail (IPATCH_IS_DLS2 (pitem), NULL);
  dls = IPATCH_DLS2 (pitem);

  refitems = ipatch_list_new ();

  IPATCH_ITEM_RLOCK (dls);

  success = ipatch_container_init_iter ((IpatchContainer *)dls, &iter,
				       IPATCH_TYPE_DLS2_INST);
  g_return_val_if_fail (success != FALSE, NULL);

  inst = ipatch_dls2_inst_first (&iter);
  while (inst)			/* loop over instruments */
    {
      IPATCH_ITEM_RLOCK (inst);	/* ## embedded lock */

      success = ipatch_container_init_iter ((IpatchContainer *)dls,
					    &region_iter,
					    IPATCH_TYPE_DLS2_INST);
      g_return_val_if_fail (success != FALSE, NULL);

      region = ipatch_dls2_region_first (&region_iter);
      while (region)		/* loop over regions */
	{
	  if (ipatch_dls2_region_peek_sample (region) == sample)
	    {
	      g_object_ref (region); /* ++ ref region for new iterator */
	      refitems->items = g_list_prepend (refitems->items, region);
	    }
	  region = ipatch_dls2_region_next (&region_iter);
	}
      IPATCH_ITEM_RUNLOCK (inst);

      inst = ipatch_dls2_inst_next (&iter);
    }
  IPATCH_ITEM_RUNLOCK (dls);

  return (refitems);
}
