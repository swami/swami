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
 * SECTION: IpatchDLS2Info
 * @short_description: DLS version 2 info functions and structure
 * @see_also: 
 * @stability: Stable
 *
 * Structure and functions used for storing DLS informational properties at
 * many levels of the format.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2Info.h"
#include "IpatchParamProp.h"
#include "ipatch_priv.h"

/* list bag for associating a hash with an object class that has IpatchDLS2Info
   properties */
typedef struct
{
  GObjectClass *obj_class;
  GHashTable *prop_hash;
} HashListBag;

static void install_prop_helper (GObjectClass *obj_class, guint property_id,
				 GParamSpec *pspec, GHashTable *hash);

/* list of GHashTable (property_id -> GParamSpec *) to speed up info
   property notifies */
static GSList *info_hash_list = NULL;


/**
 * ipatch_dls2_info_get:
 * @info: DLS2 info list
 * @fourcc: FOURCC info ID
 *
 * Gets the value of the info specified by the @fourcc ID from an @info list.
 *
 * Returns: Newly allocated info string value or %NULL if the specified info
 * is not set. Should be freed when no longer needed.
 */
char *
ipatch_dls2_info_get (IpatchDLS2Info *info, guint32 fourcc)
{
  const char *value;

  value = ipatch_dls2_info_peek (info, fourcc);
  if (value) return (g_strdup (value));
  else return (NULL);
}

/**
 * ipatch_dls2_info_peek:
 * @info: DLS2 info list
 * @fourcc: FOURCC info ID
 *
 * Gets the value of the info specified by the @fourcc ID from an @info list.
 * Like ipatch_dls2_info_get but returns the string value without duplicating
 * it.
 *
 * Returns: (transfer none): Info string value or %NULL if the specified info is not
 * set. Value is internal and should not be modified or freed.
 */
G_CONST_RETURN char *
ipatch_dls2_info_peek (IpatchDLS2Info *info, guint32 fourcc)
{
  GSList *p = info;
  IpatchDLS2InfoBag *bag;

  while (p)
    {
      bag = (IpatchDLS2InfoBag *)(p->data);
      if (bag->fourcc == fourcc) return (bag->value);
      p = g_slist_next (p);
    }
  return (NULL);
}

/**
 * ipatch_dls2_info_set:
 * @info: DLS2 info list
 * @fourcc: FOURCC info ID
 * @value: (nullable): String value to set info to or %NULL to unset
 *
 * Sets the info specified by the @fourcc ID in an @info list to a
 * string @value.
 */
void
ipatch_dls2_info_set (IpatchDLS2Info **info, guint32 fourcc, const char *value)
{
  GSList *p, *last = NULL;
  IpatchDLS2InfoBag *bag;

  p = *info;
  while (p)		 /* search for existing info with fourcc ID */
    {
      bag = (IpatchDLS2InfoBag *)(p->data);
      if (bag->fourcc == fourcc)
	{			/* found the info by foucc ID */
	  g_free (bag->value);

	  if (!value)		/* unset the value? */
	    {
	      *info = g_slist_delete_link (*info, p);
	      ipatch_dls2_info_bag_free (bag);
	    }
	  else bag->value = g_strdup (value); /* set the value */

	  return;
	}
      last = p;
      p = g_slist_next (p);
    }

  if (!value) return;		/* no value to unset */

  bag = ipatch_dls2_info_bag_new ();
  bag->fourcc = fourcc;
  bag->value = g_strdup (value);

  if (last) last = g_slist_append (last, bag);	/* info list not empty? assign to keep gcc happy */
  else *info = g_slist_append (NULL, bag);
}

/**
 * ipatch_dls2_info_free:
 * @info: DLS2 info list
 *
 * Free a DLS info list.
 */
void
ipatch_dls2_info_free (IpatchDLS2Info *info)
{
  GSList *p = info;
  IpatchDLS2InfoBag *bag;

  while (p)
    {
      bag = (IpatchDLS2InfoBag *)(p->data);
      g_free (bag->value);
      ipatch_dls2_info_bag_free (bag);
      p = g_slist_delete_link (p, p);
    }
}

/**
 * ipatch_dls2_info_duplicate:
 * @info: DLS2 info list to duplicate
 *
 * Duplicate a DLS2 info list.
 *
 * Returns: (transfer full): Newly created info list or %NULL if @info was NULL. Free it with
 * ipatch_dls2_info_free() when finished with it.
 */
IpatchDLS2Info *
ipatch_dls2_info_duplicate (IpatchDLS2Info *info)
{
  GSList *newinfo = NULL, *p = info;
  IpatchDLS2InfoBag *newbag, *bag;

  while (p)
    {
      bag = (IpatchDLS2InfoBag *)(p->data);
      newbag = ipatch_dls2_info_bag_new ();
      newbag->fourcc = bag->fourcc;
      newbag->value = g_strdup (bag->value);
      newinfo = g_slist_prepend (newinfo, bag);

      p = g_slist_next (p);
    }

  return (g_slist_reverse (newinfo));
}

/**
 * ipatch_dls2_info_is_defined:
 * @fourcc: FOURCC INFO id to check if defined
 *
 * Checks if a FOURCC INFO id is a defined INFO id.
 *
 * Returns: %TRUE if @fourcc INFO id is defined, %FALSE otherwise
 */
gboolean
ipatch_dls2_info_is_defined (guint32 fourcc)
{
  switch (fourcc)
    {
    case IPATCH_DLS2_NAME:
    case IPATCH_DLS2_DATE:
    case IPATCH_DLS2_ENGINEER:
    case IPATCH_DLS2_PRODUCT:
    case IPATCH_DLS2_COPYRIGHT:
    case IPATCH_DLS2_COMMENT:
    case IPATCH_DLS2_SOFTWARE:
    case IPATCH_DLS2_ARCHIVE_LOCATION:
    case IPATCH_DLS2_ARTIST:
    case IPATCH_DLS2_COMMISSIONED:
    case IPATCH_DLS2_GENRE:
    case IPATCH_DLS2_KEYWORDS:
    case IPATCH_DLS2_MEDIUM:
    case IPATCH_DLS2_SUBJECT:
    case IPATCH_DLS2_SOURCE:
    case IPATCH_DLS2_SOURCE_FORM:
    case IPATCH_DLS2_TECHNICIAN:
      return (TRUE);
    default:
      return (FALSE);
    }
}

/**
 * ipatch_dls2_info_install_class_properties:
 * @obj_class: GObjectClass to install INFO properties on
 *
 * Installs INFO properties for the supplied @obj_class. Used for
 * class construction of objects implementing IpatchDLS2InfoType
 * properties.
 */
void
ipatch_dls2_info_install_class_properties (GObjectClass *obj_class)
{
  HashListBag *bag;
  GHashTable *hash;

  hash = g_hash_table_new (NULL, NULL);

  bag = g_new (HashListBag, 1);
  bag->obj_class = obj_class;
  bag->prop_hash = hash;
  info_hash_list = g_slist_prepend (info_hash_list, bag);

  install_prop_helper (obj_class, IPATCH_DLS2_NAME,
	g_param_spec_string ("name", _("Name"),
			     _("Name"),
			     _("untitled"),
			     G_PARAM_READWRITE | IPATCH_PARAM_UNIQUE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_DATE,
	g_param_spec_string ("date", _("Date"),
			     _("Creation date (YYYY-MM-DD)"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_ENGINEER,
	g_param_spec_string ("engineer", _("Engineer"),
			     _("Engineers separated by \"; \""),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_PRODUCT,
	g_param_spec_string ("product", _("Product"),
			     _("Product intended for"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_COPYRIGHT,
	g_param_spec_string ("copyright", _("Copyright"),
			     _("Copyright"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_COMMENT,
	g_param_spec_string ("comment", _("Comments"),
			     _("Comments"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_SOFTWARE,
	g_param_spec_string ("software", _("Software"),
			     _("Editor software used"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_ARCHIVE_LOCATION,
	g_param_spec_string ("archive-location", _("Archive Location"),
			     _("Location where subject is archived"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_ARTIST,
	g_param_spec_string ("artist", _("Artist"),
			     _("Original artist"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_COMMISSIONED,
	g_param_spec_string ("commissioned", _("Commissioned"),
			     _("Who commissioned the material"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_GENRE,
	g_param_spec_string ("genre", _("Genre"),
			     _("Genre"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_KEYWORDS,
	g_param_spec_string ("keywords", _("Keywords"),
			     _("Keywords (separated by \"; \")"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_MEDIUM,
	g_param_spec_string ("medium", _("Medium"),
		_("Original medium of the material (record, CD, etc)"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_SUBJECT,
	g_param_spec_string ("subject", _("Subject"),
			     _("Subject of the material"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_SOURCE,
	g_param_spec_string ("source", _("Source"),
			     _("Source of the original material"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_SOURCE_FORM,
	g_param_spec_string ("source-form", _("Source form"),
			     _("Original source that was digitized"),
			     NULL, G_PARAM_READWRITE), hash);
  install_prop_helper (obj_class, IPATCH_DLS2_TECHNICIAN,
	g_param_spec_string ("technician", _("Technician"),
			     _("Technician who sampled the material"),
			     NULL, G_PARAM_READWRITE), hash);
}

/* helper function to hash property_id->pspec and install the property also */
static void
install_prop_helper (GObjectClass *obj_class, guint property_id,
		     GParamSpec *pspec, GHashTable *hash)
{
  g_hash_table_insert (hash, GUINT_TO_POINTER (property_id), pspec);
  g_object_class_install_property (obj_class, property_id, pspec);
}

/**
 * ipatch_dls2_info_set_property:
 * @info_list: Pointer to a list of #IpatchDLS2Info structures
 * @property_id: FOURCC INFO property id to set value of
 * @value: A string GValue to set INFO value to
 *
 * A function used by object set_property methods that implement a
 * #IpatchDLS2Info list to set an INFO property.
 *
 * Returns: %TRUE if @property_id is a valid INFO id, %FALSE otherwise
 */
gboolean
ipatch_dls2_info_set_property (GSList **info_list, guint property_id,
			       const GValue *value)
{
  if (ipatch_dls2_info_is_defined (property_id))
    {
      ipatch_dls2_info_set (info_list, property_id,
			    g_value_get_string (value));
      return (TRUE);
    }
  else return (FALSE);
}

/**
 * ipatch_dls2_info_get_property:
 * @info_list: A list of #IpatchDLS2Info structures
 * @property_id: FOURCC INFO property id to get value of
 * @value: A string GValue to store the value of the info to
 *
 * A function used by object set_property methods that implement a
 * #IpatchDLS2Info list to get an INFO property.
 *
 * Returns: %TRUE if @property_id is a valid INFO id, %FALSE otherwise
 */
gboolean
ipatch_dls2_info_get_property (GSList *info_list, guint property_id,
			       GValue *value)
{
  if (ipatch_dls2_info_is_defined (property_id))
    {
      g_value_set_string (value,
			  ipatch_dls2_info_get (info_list, property_id));
      return (TRUE);
    }
  else return (FALSE);
}

/**
 * ipatch_dls2_info_notify:
 * @item: Item with INFO properties to notify property change on
 * @fourcc: FOURCC property ID of info that has changed
 * @new_value: New value assigned to the property
 * @old_value: Old value of property
 *
 * Notify a changed INFO property on @item for the given fourcc ID.
 * A convenience function to objects that implement a #IpatchDLS2Info list.
 */
void
ipatch_dls2_info_notify (IpatchItem *item, guint32 fourcc,
			 const GValue *new_value, const GValue *old_value)
{
  GHashTable *found_prop_hash = NULL;
  GObjectClass *obj_class;
  GParamSpec *found_pspec = NULL;
  GSList *p;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (G_IS_VALUE (new_value));
  g_return_if_fail (G_IS_VALUE (old_value));

  obj_class = G_OBJECT_GET_CLASS (item);

  /* search for property hash table for the object's class */
  for (p = info_hash_list; p; p = p->next)
    {
      if (((HashListBag *)(p->data))->obj_class == obj_class)
	{
	  found_prop_hash = ((HashListBag *)(p->data))->prop_hash;
	  break;
	}
    }

  g_return_if_fail (found_prop_hash);

  found_pspec = g_hash_table_lookup (found_prop_hash, GUINT_TO_POINTER (fourcc));
  g_return_if_fail (found_pspec != NULL);

  ipatch_item_prop_notify (item, found_pspec, new_value, old_value);
}

/**
 * ipatch_dls2_info_bag_new: (skip)
 *
 * Create a new DLS info bag structure.
 *
 * Returns: Newly allocated info bag.
 */
IpatchDLS2InfoBag *
ipatch_dls2_info_bag_new (void)
{
  return (g_slice_new0 (IpatchDLS2InfoBag));
}

/**
 * ipatch_dls2_info_bag_free: (skip)
 * @bag: Info bag structure to free
 *
 * Free a DLS info bag allocated with ipatch_dls2_info_bag_new().
 */
void
ipatch_dls2_info_bag_free (IpatchDLS2InfoBag *bag)
{
  g_return_if_fail (bag != NULL);
  g_slice_free (IpatchDLS2InfoBag, bag);
}
