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
 * SECTION: IpatchSLI
 * @short_description: Spectralis instrument file object
 * @see_also: 
 *
 * Object type for Spectralis format instruments.
 */
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSLI.h"
#include "IpatchSLIFile.h"
#include "IpatchSLIZone.h"
#include "IpatchParamProp.h"
#include "IpatchTypeProp.h"
#include "IpatchVirtualContainer_types.h"
#include "version.h"
#include "ipatch_priv.h"

/* properties */
enum {
  PROP_0,
  PROP_TITLE
};

static void ipatch_sli_class_init (IpatchSLIClass *klass);
static void ipatch_sli_init (IpatchSLI *sli);
static void ipatch_sli_get_title (IpatchSLI *sli, GValue *value);
static void ipatch_sli_get_property (GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec);
static void ipatch_sli_item_copy (IpatchItem *dest, IpatchItem *src,
				  IpatchItemCopyLinkFunc link_func,
				  gpointer user_data);

static const GType *ipatch_sli_container_child_types (void);
static const GType *ipatch_sli_container_virtual_types (void);
static gboolean ipatch_sli_container_init_iter (IpatchContainer *container,
						IpatchIter *iter, GType type);
static void ipatch_sli_container_make_unique (IpatchContainer *container,
					      IpatchItem *item);
static void ipatch_sli_parent_file_prop_notify (IpatchItemPropNotify *info);

static gpointer parent_class = NULL;
static GType sli_child_types[3] = { 0 };
static GType sli_virt_types[3] = { 0 };

/* for chaining to original file-name property */
static IpatchItemClass *base_item_class;


/* Spectralis item type creation function */
GType
ipatch_sli_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchSLIClass), NULL, NULL,
      (GClassInitFunc)ipatch_sli_class_init, NULL, NULL,
      sizeof (IpatchSLI),
      0,
      (GInstanceInitFunc)ipatch_sli_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_BASE, "IpatchSLI",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_sli_class_init (IpatchSLIClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  IpatchContainerClass *container_class = IPATCH_CONTAINER_CLASS (klass);

  /* save original base class for chaining file-name property */
  base_item_class = IPATCH_ITEM_CLASS (g_type_class_ref (IPATCH_TYPE_BASE));

  parent_class = g_type_class_peek_parent (klass);

  obj_class->get_property = ipatch_sli_get_property;

  item_class->copy = ipatch_sli_item_copy;

  container_class->child_types = ipatch_sli_container_child_types;
  container_class->virtual_types = ipatch_sli_container_virtual_types;
  container_class->init_iter = ipatch_sli_container_init_iter;
  container_class->make_unique = ipatch_sli_container_make_unique;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  sli_child_types[0] = IPATCH_TYPE_SLI_INST;
  sli_child_types[1] = IPATCH_TYPE_SLI_SAMPLE;

  sli_virt_types[0] = IPATCH_TYPE_VIRTUAL_SLI_INST;
  sli_virt_types[1] = IPATCH_TYPE_VIRTUAL_SLI_SAMPLES;
}

static void
ipatch_sli_init (IpatchSLI *sli)
{
  ipatch_item_clear_flags (IPATCH_ITEM (sli), IPATCH_BASE_CHANGED);
  /* add a prop notify on file-name so sli can notify it's title also */
  ipatch_item_prop_connect_by_name (IPATCH_ITEM (sli), "file-name",
                                    ipatch_sli_parent_file_prop_notify, NULL,
                                    sli);
}

static void
ipatch_sli_get_title (IpatchSLI *sli, GValue *value)
{
  char *filename;
  gchar *s;

  filename = ipatch_base_get_file_name (IPATCH_BASE (sli));
  s = (filename ? g_path_get_basename (filename) : NULL);
  free (filename);
  
  if (!s || *s == G_DIR_SEPARATOR || *s == '.')
  {
    g_free (s);
    s = g_strdup(_(IPATCH_BASE_DEFAULT_NAME));
  }
  g_value_take_string (value, s);
}

static void
ipatch_sli_get_property (GObject *object, guint property_id,
                         GValue *value, GParamSpec *pspec)
{
  IpatchSLI *sli;

  g_return_if_fail (IPATCH_IS_SLI (object));
  sli = IPATCH_SLI (object);

  switch (property_id)
    {
    case PROP_TITLE:
      ipatch_sli_get_title (sli, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sli_item_copy (IpatchItem *dest, IpatchItem *src,
		      IpatchItemCopyLinkFunc link_func, gpointer user_data)
{
  IpatchSLI *src_sli, *dest_sli;
  IpatchItem *newitem;
  GHashTable *repl_samples;
  GSList *p;

  src_sli = IPATCH_SLI (src);
  dest_sli = IPATCH_SLI (dest);

  /* create item replacement hash */
  repl_samples = g_hash_table_new (NULL, NULL);

  IPATCH_ITEM_RLOCK (src_sli);

  if (IPATCH_BASE (src_sli)->file)
    ipatch_base_set_file (IPATCH_BASE (dest_sli), IPATCH_BASE (src_sli)->file);

  p = src_sli->samples;
  while (p)			/* duplicate samples */
  { /* ++ ref new duplicate sample, !! sample list takes it over */
    newitem = ipatch_item_duplicate ((IpatchItem *)(p->data));
    dest_sli->samples = g_slist_prepend (dest_sli->samples, newitem);
    ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_sli));

    /* add to sample pointer replacement hash */
    g_hash_table_insert (repl_samples, p->data, newitem);

    p = g_slist_next (p);
  }

  p = src_sli->insts;
  while (p)			/* duplicate instruments */
  { /* ++ ref new duplicate instrument, !! instrument list takes it over
    * duplicate instrument and replace referenced sample pointers. */
    newitem = ipatch_item_duplicate_replace ((IpatchItem *)(p->data),
                                             repl_samples);
    dest_sli->insts = g_slist_prepend (dest_sli->insts, newitem);
    ipatch_item_set_parent (newitem, IPATCH_ITEM (dest_sli));

    p = g_slist_next (p);
  }

  IPATCH_ITEM_RUNLOCK (src_sli);

  dest_sli->insts = g_slist_reverse (dest_sli->insts);
  dest_sli->samples = g_slist_reverse (dest_sli->samples);

  g_hash_table_destroy (repl_samples);
}

static const GType *
ipatch_sli_container_child_types (void)
{
  return (sli_child_types);
}

static const GType *
ipatch_sli_container_virtual_types (void)
{
  return (sli_virt_types);
}

/* container is locked by caller */
static gboolean
ipatch_sli_container_init_iter (IpatchContainer *container,
				IpatchIter *iter, GType type)
{
  IpatchSLI *sli = IPATCH_SLI (container);

  if (g_type_is_a (type, IPATCH_TYPE_SLI_INST))
    ipatch_iter_GSList_init (iter, &sli->insts);
  else if (g_type_is_a (type, IPATCH_TYPE_SLI_SAMPLE))
    ipatch_iter_GSList_init (iter, &sli->samples);
  else
    {
      g_critical ("Invalid child type '%s' for parent of type '%s'",
		  g_type_name (type), g_type_name (G_OBJECT_TYPE (container)));
      return (FALSE);
    }

  return (TRUE);
}

static void
ipatch_sli_container_make_unique (IpatchContainer *container,
				  IpatchItem *item)
{
  IpatchSLI *sli = IPATCH_SLI (container);
  char *name, *newname;

  if (!(IPATCH_IS_SLI_INST (item) || IPATCH_IS_SLI_SAMPLE (item)))
  {
    g_critical ("Invalid child type '%s' for IpatchSLI object",
                g_type_name (G_TYPE_FROM_INSTANCE (item)));
    return;
  }

  IPATCH_ITEM_WLOCK (sli);

  g_object_get (item, "name", &name, NULL);
  newname = ipatch_sli_make_unique_name (sli, G_TYPE_FROM_INSTANCE (item),
					 name, NULL);
  if (!name || strcmp (name, newname) != 0)
    g_object_set (item, "name", newname, NULL);

  IPATCH_ITEM_WUNLOCK (sli);

  g_free (name);
  g_free (newname);
}

/* property notify for when parent's "file-name" property changes */
static void
ipatch_sli_parent_file_prop_notify (IpatchItemPropNotify *info)
{
  IpatchItem *sli = (IpatchItem *)(info->user_data);
  /* notify that SLI's title has changed */
  ipatch_item_prop_notify (sli, ipatch_item_pspec_title,
                           info->new_value, info->old_value);
}

/**
 * ipatch_sli_new:
 *
 * Create a new Spectralis base object.
 *
 * Returns: New Spectralis base object with a reference count of 1. Caller
 * owns the reference and removing it will destroy the item.
 */
IpatchSLI *
ipatch_sli_new (void)
{
  return (IPATCH_SLI (g_object_new (IPATCH_TYPE_SLI, NULL)));
}

/**
 * ipatch_sli_set_file:
 * @sli: SLI object to set file object of
 * @file: File object to use with the SLI object
 *
 * Sets the file object of a SLI object. SLI files are kept open
 * for sample data that references the file. This function sets a
 * Spectralis object's authoritive file. A convenience function, as
 * ipatch_base_set_file() does the same thing (albeit without more specific
 * type casting).
 */
void
ipatch_sli_set_file (IpatchSLI *sli, IpatchSLIFile *file)
{
  g_return_if_fail (IPATCH_IS_SLI (sli));
  g_return_if_fail (IPATCH_IS_SLI_FILE (file));

  ipatch_base_set_file (IPATCH_BASE (sli), IPATCH_FILE (file));
}

/**
 * ipatch_sli_get_file:
 * @sli: SLI object to get file object of
 *
 * Gets the file object of a SLI object. The returned SLI file object's
 * reference count has been incremented. The caller owns the reference and is
 * responsible for removing it with g_object_unref.
 * A convenience function as ipatch_base_get_file() does the same thing
 * (albeit without more specific type casting).
 *
 * Returns: (transfer full): The SLI file object or %NULL if @sli is not open. Remember
 * to unref the file object with g_object_unref() when done with it.
 */
IpatchSLIFile *
ipatch_sli_get_file (IpatchSLI *sli)
{
  IpatchFile *file;

  g_return_val_if_fail (IPATCH_IS_SLI (sli), NULL);

  file = ipatch_base_get_file (IPATCH_BASE (sli));
  if (file) return (IPATCH_SLI_FILE (file));
  else return (NULL);
}

/**
 * ipatch_sli_make_unique_name:
 * @sli: SLI object
 * @child_type: A child type of @sli to search for a unique name in
 * @name: (nullable): An initial name to use or %NULL
 * @exclude: (nullable): An item to exclude from search or %NULL
 *
 * Generates a unique name for the given @child_type in @sli. The @name
 * parameter is used as a base and is modified, by appending a number, to
 * make it unique (if necessary). The @exclude parameter is used to exclude
 * an existing @sli child item from the search.
 *
 * MT-Note: To ensure that an item is actually unique before being
 * added to a SLI object, ipatch_container_add_unique() should be
 * used.
 *
 * Returns: A new unique name which should be freed when finished with it.
 */
char *
ipatch_sli_make_unique_name (IpatchSLI *sli, GType child_type,
			     const char *name, const IpatchItem *exclude)
{
  GSList **list, *p;
  char curname[IPATCH_SLI_NAME_SIZE + 1];
  guint name_ofs, count = 2;

  g_return_val_if_fail (IPATCH_IS_SLI (sli), NULL);

  if (g_type_is_a (child_type, IPATCH_TYPE_SLI_INST))
    {
      list = &sli->insts;
      name_ofs = G_STRUCT_OFFSET (IpatchSLIInst, name);
      if (!name || !*name) name = _("New Instrument");
    }
  else if (g_type_is_a (child_type, IPATCH_TYPE_SLI_SAMPLE))
    {
      list = &sli->samples;
      name_ofs = G_STRUCT_OFFSET (IpatchSLISample, name);
      if (!name || !*name) name = _("New Sample");
    }
  else
    {
      g_critical ("Invalid child type '%s' of parent type '%s'",
		  g_type_name (child_type), g_type_name (G_OBJECT_TYPE (sli)));
      return (NULL);
    }

  g_strlcpy (curname, name, sizeof (curname));

  IPATCH_ITEM_RLOCK (sli);

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

  IPATCH_ITEM_RUNLOCK (sli);

  return (g_strdup (curname));
}

/**
 * ipatch_sli_find_inst:
 * @sli: SLI to search in
 * @name: Name of Instrument to find
 * @exclude: (nullable): An instrument to exclude from the search or %NULL
 *
 * Find an instrument by @name in an SLI object. If a matching instrument
 * is found, its reference count is incremented before it is returned.
 * The caller is responsible for removing the reference with g_object_unref()
 * when finished with it.
 *
 * Returns: (transfer full): The matching instrument or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchSLIInst *
ipatch_sli_find_inst (IpatchSLI *sli, const char *name,
		      const IpatchSLIInst *exclude)
{
  IpatchSLIInst *inst;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SLI (sli), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  IPATCH_ITEM_RLOCK (sli);
  p = sli->insts;
  while (p)
    {
      inst = (IpatchSLIInst *)(p->data);
      IPATCH_ITEM_RLOCK (inst);	/* MT - Recursive LOCK */

      if (inst != exclude && strcmp (inst->name, name) == 0)
	{
	  g_object_ref (inst);
	  IPATCH_ITEM_RUNLOCK (inst);
	  IPATCH_ITEM_RUNLOCK (sli);
	  return (inst);
	}
      IPATCH_ITEM_RUNLOCK (inst);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sli);

  return (NULL);
}

/**
 * ipatch_sli_find_sample:
 * @sli: SLI to search in
 * @name: Name of sample to find
 * @exclude: (nullable): A sample to exclude from the search or %NULL
 *
 * Find a sample by @name in a SLI object. If a sample is found its
 * reference count is incremented before it is returned. The caller
 * is responsible for removing the reference with g_object_unref()
 * when finished with it.
 *
 * Returns: (transfer full): The matching sample or %NULL if not found. Remember to unref
 * the item when finished with it.
 */
IpatchSLISample *
ipatch_sli_find_sample (IpatchSLI *sli, const char *name,
			const IpatchSLISample *exclude)
{
  IpatchSLISample *sample;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_SLI (sli), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  IPATCH_ITEM_RLOCK (sli);
  p = sli->samples;
  while (p)
    {
      sample = (IpatchSLISample *)(p->data);
      IPATCH_ITEM_RLOCK (sample); /* MT - Recursive LOCK */

      if (p->data != exclude && strcmp (sample->name, name) == 0)
	{
	  g_object_ref (sample);
	  IPATCH_ITEM_RUNLOCK (sample);
	  IPATCH_ITEM_RUNLOCK (sli);
	  return (p->data);
	}
      IPATCH_ITEM_RUNLOCK (sample);
      p = g_slist_next (p);
    }
  IPATCH_ITEM_RUNLOCK (sli);

  return (NULL);
}

/**
 * ipatch_sli_get_zone_references:
 * @sample: Sample to locate referencing zones of.
 *
 * Get list of zones referencing an IpatchSLISample.
 *
 * Returns: (transfer full): New item list containing #IpatchSLIZone objects
 * that refer to @sample. The returned list has a reference count of 1 which
 * the caller owns, unreference to free the list.
 */
IpatchList *
ipatch_sli_get_zone_references (IpatchSLISample *sample)
{
  IpatchList *reflist, *instlist, *zonelist;
  IpatchSLI *sli;
  IpatchSLIZone *zone;
  IpatchIter iter, zone_iter;
  IpatchItem *pitem;

  g_return_val_if_fail (IPATCH_IS_SLI_SAMPLE (sample), NULL);
  pitem = ipatch_item_get_parent (IPATCH_ITEM (sample));
  g_return_val_if_fail (IPATCH_IS_SLI (pitem), NULL);
  sli = IPATCH_SLI (pitem);
  
  reflist = ipatch_list_new ();	/* ++ ref new list */
  instlist = ipatch_sli_get_insts (sli); /* ++ ref instlist */

  ipatch_list_init_iter (instlist, &iter);
  pitem = ipatch_item_first (&iter);
  while (pitem) /* loop over instruments  */
  {
    zonelist = ipatch_sli_inst_get_zones (pitem); /* ++ ref new zone list */
    ipatch_list_init_iter (zonelist, &zone_iter);

    zone = ipatch_sli_zone_first (&zone_iter);
    while (zone)
    {
      if (ipatch_sli_zone_peek_sample (zone) == sample)
      {
        g_object_ref (zone); /* ++ ref zone for new list */
        reflist->items = g_list_prepend (reflist->items, zone);
      }
      zone = ipatch_sli_zone_next (&zone_iter);
    }
    g_object_unref (zonelist); /* -- unref zone list */
    pitem = ipatch_item_next (&iter);
  }
  g_object_unref (instlist); /* -- unref instlist */

  /* reverse list to preserve order since we prepended */
  reflist->items = g_list_reverse (reflist->items);

  return (reflist); /* !! caller takes over reference */
}
