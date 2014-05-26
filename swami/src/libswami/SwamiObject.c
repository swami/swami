/*
 * SwamiObject.c - Child object properties and type rank system
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include <stdio.h>
#include <stdarg.h>

#include <glib.h>
#include <glib-object.h>
#include <gobject/gobjectnotifyqueue.c>
#include <gobject/gvaluecollector.h>

#include "SwamiObject.h"
#include "SwamiRoot.h"
#include "builtin_enums.h"
#include "i18n.h"
#include "swami_priv.h"

#define PARAM_SPEC_PARAM_ID(pspec)              ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)      ((pspec)->param_id = (id))

/* Swami child object properties */
enum
{
  OBJ_PROP_0,
  OBJ_PROP_NAME,
  OBJ_PROP_RANK,
  OBJ_PROP_FLAGS,
  OBJ_PROP_ROOT
};

/* --- private function prototypes --- */

static gint swami_gcompare_typerank_by_rank (gconstpointer a, gconstpointer b);
static gint swami_gcompare_typerank_by_type (gconstpointer a, gconstpointer b);
static void swami_type_recurse_make_list (GType type, GList **list);

static gboolean node_traverse_match_name (GNode *node, gpointer data);
static gboolean node_traverse_match_type (GNode *node, gpointer data);
static gint swami_gcompare_object_rank (gconstpointer a, gconstpointer b);

static void swami_install_object_property (guint property_id,
					   GParamSpec *pspec);
static void object_property_notify_dispatcher (GObject *object, guint n_pspecs,
					       GParamSpec **pspecs);
static inline SwamiObjectPropBag *object_new_propbag (GObject *object);
static void free_propbag (gpointer data);
static void object_get_property (GObject *object, GParamSpec *pspec,
				 GValue *value);
static void object_set_property (GObject *object, GParamSpec *pspec,
				 const GValue *value,
				 GObjectNotifyQueue *nqueue);

GQuark swami_object_propbag_quark; /* object qdata quark for property bag */

/* global lock for all Swami object property bags
   (only used for name property currently) */
G_LOCK_DEFINE_STATIC (SwamiObjectPropBag);

static GParamSpecPool *object_property_pool; /* Swami property pool */
static GObjectNotifyContext *object_property_notify_context;


/* structure used to rank GTypes */
typedef struct
{
  GType type;
  int rank;
} TypeRank;

G_LOCK_DEFINE_STATIC (typerank); /* a lock for typerank system */
static GHashTable *typerank_hash = NULL;

/* signal id of SwamiRoot PROP_NOTIFY signal (looked up by name) */
static guint root_prop_notify_signal_id;


/* initialization for swami object properties and type rank system */
void
_swami_object_init (void)
{
  static GObjectNotifyContext opn_context = { 0, NULL, NULL };

  root_prop_notify_signal_id = g_signal_lookup ("swami-prop-notify",
						SWAMI_TYPE_ROOT);
  typerank_hash = g_hash_table_new (NULL, NULL);
  object_property_pool = g_param_spec_pool_new (FALSE);

  opn_context.quark_notify_queue
    = g_quark_from_static_string ("Swami-object-property-notify-queue");
  opn_context.dispatcher = object_property_notify_dispatcher;
  object_property_notify_context = &opn_context;

  /* get a quark for property bag object qdata quark */
  swami_object_propbag_quark = g_quark_from_static_string ("_SwamiPropBag");

  /* install Swami object properties */
  swami_install_object_property (OBJ_PROP_NAME,
				 g_param_spec_string ("name", _("Name"),
						      _("Name"),
						      NULL,
						      G_PARAM_READWRITE));
  swami_install_object_property (OBJ_PROP_RANK,
				 g_param_spec_int ("rank", _("Rank"),
						   _("Rank"),
						   1, 100, SWAMI_RANK_NORMAL,
						   G_PARAM_READWRITE));
  swami_install_object_property (OBJ_PROP_FLAGS,
				 g_param_spec_flags ("flags", _("Flags"),
						     _("Flags"),
						     SWAMI_TYPE_OBJECT_FLAGS,
						     0,
						     G_PARAM_READWRITE));
  swami_install_object_property (OBJ_PROP_ROOT,
				 g_param_spec_object ("root", _("Root"),
						      _("Root object"),
						      SWAMI_TYPE_ROOT,
						      G_PARAM_READWRITE));
}

/**
 * swami_type_set_rank:
 * @type: Type to set rank value of
 * @group_type: An ancestor type of @type
 * @rank: Rank value (see #SwamiRank enum).
 *
 * Sets the ranking of instance GTypes derived from an ancestor @group_type.
 * This is useful to set a default instance type or to define order to types.
 * An example of use would be to set a higher rank on a plugin wavetable type
 * to cause it to be used when creating a default wavetable instance.
 */
void
swami_type_set_rank (GType type, GType group_type, int rank)
{
  GSList *grplist, *p;
  TypeRank *typerank;

  g_return_if_fail (g_type_is_a (type, group_type));
  rank = CLAMP (rank, 1, 100);

  G_LOCK (typerank);

  grplist = g_hash_table_lookup (typerank_hash, GSIZE_TO_POINTER (group_type));
  if (grplist)
    {
      p = grplist;
      while (p)	     /* remove existing TypeRank for @type (if any) */
	{
	  if (((TypeRank *)(p->data))->type == type)
	    {
	      g_slice_free (TypeRank, p->data);
	      grplist = g_slist_delete_link (grplist, p);
	      break;
	    }
	  p = g_slist_next (p);
	}
    }

  if (rank != SWAMI_RANK_NORMAL)
    {
      typerank = g_slice_new (TypeRank);
      typerank->type = type;
      typerank->rank = rank;
      grplist = g_slist_insert_sorted (grplist, typerank,
				       swami_gcompare_typerank_by_rank);
    }

  g_hash_table_insert (typerank_hash, GSIZE_TO_POINTER (group_type), grplist);

  G_UNLOCK (typerank);
}

static gint
swami_gcompare_typerank_by_rank (gconstpointer a, gconstpointer b)
{
  TypeRank *atyperank = (TypeRank *)a, *btyperank = (TypeRank *)b;
  return (atyperank->rank - btyperank->rank);
}

static gint
swami_gcompare_typerank_by_type (gconstpointer a, gconstpointer b)
{
  TypeRank *atyperank = (TypeRank *)a, *btyperank = (TypeRank *)b;
  return (atyperank->type - btyperank->type);
}

/**
 * swami_type_get_rank:
 * @type: Instance type to get rank of
 * @group_type: Ancestor group type
 *
 * Gets the instance rank of the given @type derived from a @group_type.
 * See swami_type_set_rank() for more info.
 *
 * Returns: Rank value (#SWAMI_RANK_NORMAL if not explicitly set).
 */
int
swami_type_get_rank (GType type, GType group_type)
{
  GSList *grplist, *p;
  TypeRank find;
  int rank = SWAMI_RANK_NORMAL;

  g_return_val_if_fail (g_type_is_a (type, group_type), 0);
  find.type = type;

  G_LOCK (typerank);

  grplist = g_hash_table_lookup (typerank_hash, GSIZE_TO_POINTER (group_type));
  p = g_slist_find_custom (grplist, &find, swami_gcompare_typerank_by_type);
  if (p) rank = ((TypeRank *)(p->data))->rank;

  G_UNLOCK (typerank);

  return (rank);
}

/**
 * swami_type_get_children:
 * @group_type: Group type to get derived types from
 *
 * Gets an array of types that are derived from @group_type, sorted by rank.
 * For types that have not had their rank explicitly set #SWAMI_RANK_NORMAL
 * will be assumed.
 *
 * Returns: Newly allocated and zero terminated array of GTypes sorted by rank
 * or %NULL if no types found. Should be freed when finished.
 */
GType *
swami_type_get_children (GType group_type)
{
  GList *typelist = NULL, *sortlist = NULL, *normal = NULL, *tp;
  GSList *grplist, *p;
  TypeRank *typerank;
  GType *retarray, *ap;

  g_return_val_if_fail (group_type != 0, NULL);

  /* create list of type tree under (and including) group_type */
  swami_type_recurse_make_list (group_type, &typelist);
  if (!typelist) return (NULL);

  G_LOCK (typerank);

  /* sort typelist */
  grplist = g_hash_table_lookup (typerank_hash, GSIZE_TO_POINTER (group_type));
  p = grplist;
  while (p)
    {
      typerank = (TypeRank *)(p->data);
      tp = g_list_find (typelist, GSIZE_TO_POINTER (typerank->type));
      if (tp)
	{
	  typelist = g_list_remove_link (typelist, tp);
	  sortlist = g_list_concat (tp, sortlist);

	  /* finds the node to insert SWAMI_RANK_NORMAL types at */
	  if (typerank->rank > SWAMI_RANK_NORMAL) normal = tp;
	}
      p = g_slist_next (p);
    }

  G_UNLOCK (typerank);

  /* add remaining SWAMI_RANK_NORMAL types (not in hash) */
  if (typelist)
    {
      if (normal) /* found node to insert SWAMI_RANK_NORMAL types before? */
	{
	  GList *last = g_list_last (typelist);

	  /* link SWAMI_RANK_NORMAL types into sortlist */
	  if (normal->prev) normal->prev->next = typelist;
	  typelist->prev = normal->prev;
	  normal->prev = last;
	  last->next = normal;
	} /* all ranks in hash below or equal to SWAMI_RANK_NORMAL, append */
      else sortlist = g_list_concat (sortlist, typelist);
    }

  tp = sortlist = g_list_reverse (sortlist);
  ap = retarray = g_malloc ((g_list_length (sortlist) + 1) * sizeof (GType));
  while (tp)
    {
      *ap = GPOINTER_TO_SIZE (tp->data);	/* store GType into array element */
      ap++;
      tp = g_list_delete_link (tp, tp);	/* delete and iterate */
    }
  *ap = 0;

  return (retarray);
}

/* recursive function to find non-abstract types derived from initial type */
static void
swami_type_recurse_make_list (GType type, GList **list)
{
  GType *children, *t;

  if (!G_TYPE_IS_ABSTRACT (type)) /* only add non-abstract types */
    *list = g_list_prepend (*list, GSIZE_TO_POINTER (type));

  children = g_type_children (type, NULL);
  t = children;
  while (*t)
    {
      swami_type_recurse_make_list (*t, list);
      t++;
    }
  g_free (children);
}

/**
 * swami_type_get_default:
 * @group_type: Group type to get default (highest ranked) instance type from
 *
 * Gets the default (highest ranked) type derived from a group_type.
 * Like swami_type_get_children() but returns first type instead of an array.
 *
 * Returns: GType of default type or 0 if no types derived from @group_type.
 */
GType
swami_type_get_default (GType group_type)
{
  GType *types, rettype = 0;

  types = swami_type_get_children (group_type);
  if (types)
    {
      rettype = *types;
      g_free (types);
    }

  return (rettype);
}

/**
 * swami_object_set_default:
 * @object: Object to set as default
 * @type: A type that @object is derived from (or its type)
 *
 * Set an @object as default amongst its peers of the same @type. Uses
 * the Swami rank system (see "rank" Swami property) to elect a default
 * object. All other object's registered to the same #SwamiRoot and of
 * the same @type (or derived thereof) are set to #SWAMI_RANK_NORMAL and the
 * @object's rank is set higher.
 */
void
swami_object_set_default (GObject *object, GType type)
{
  IpatchList *list;
  IpatchIter iter;
  GObject *pobj;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object, type));

  list = swami_object_find_by_type (object, g_type_name (type));/* ++ ref */
  g_return_if_fail (list != NULL);

  ipatch_list_init_iter (list, &iter);
  pobj = ipatch_iter_first (&iter);
  while (pobj)
    {
      swami_object_set (pobj, "rank", (pobj == object) ? SWAMI_RANK_DEFAULT
			: SWAMI_RANK_NORMAL);
      pobj = ipatch_iter_next (&iter);
    }
  g_object_unref (list);	/* -- unref list */
}

/* a bag for swami_object_get_by_name() */
typedef struct
{
  const char *name;
  GObject *match;
} ObjectFindNameBag;

/**
 * swami_object_get_by_name:
 * @object: Swami root object (#SwamiRoot) or an object added to one
 * @name: Name to look for
 *
 * Finds an object by its Swami name property, searches recursively
 * through object tree. The @object can be either a #SwamiRoot or, as
 * an added convenience, an object registered to one.
 *
 * Returns: Object that matched @name or %NULL if not found. Remember to
 * unref it when finished.
 */
GObject *
swami_object_get_by_name (GObject *object, const char *name)
{
  ObjectFindNameBag findbag;
  SwamiObjectPropBag *propbag;
  SwamiRoot *root;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!SWAMI_IS_ROOT (object))
    {
      propbag = g_object_get_qdata (object, swami_object_propbag_quark);
      g_return_val_if_fail (propbag != NULL, NULL);
      g_return_val_if_fail (SWAMI_IS_ROOT (propbag->root), NULL);
      root = propbag->root;
    }
  else root = SWAMI_ROOT (object);

  findbag.name = name;
  findbag.match = NULL;

  SWAMI_LOCK_READ (root->proptree);

  g_node_traverse (root->proptree->tree, G_PRE_ORDER, G_TRAVERSE_ALL,
		   -1, node_traverse_match_name, &findbag);

  SWAMI_UNLOCK_READ (root->proptree);

  return (findbag.match);
}

static gboolean
node_traverse_match_name (GNode *node, gpointer data)
{
  ObjectFindNameBag *findbag = (ObjectFindNameBag *)data;
  SwamiPropTreeNode *propnode;
  SwamiObjectPropBag *propbag;

  propnode = (SwamiPropTreeNode *)(node->data);
  propbag = g_object_get_qdata (G_OBJECT (propnode->object),
				swami_object_propbag_quark);
  if (propbag && propbag->name && strcmp (findbag->name, propbag->name) == 0)
    {
      findbag->match = propnode->object;
      g_object_ref (findbag->match);
      return (TRUE);		/* stop this madness */
    }

  return (FALSE);		/* continue until found */
}

/* a bag for swami_object_find_by_type() */
typedef struct
{
  GType type;
  IpatchList *list;
} ObjectFindTypeBag;

/**
 * swami_object_find_by_type:
 * @object: Swami root object (#SwamiRoot) or an object added to one
 * @type_name: Name of GObject derived GType of objects to search for, objects
 *   derived from this type will also match.
 *
 * Lookup all objects by @type_name or derived from @type_name
 * recursively.  The @object can be either a #SwamiRoot or, as an
 * added convenience, an object registered to one. The returned
 * iterator list is sorted by Swami rank (see "rank" Swami property).
 *
 * Returns: New list of objects of @type_name and/or derived
 * from it or NULL if no matches to that type. The returned list has a
 * reference count of 1 which the caller owns, remember to unref it when
 * finished.
 */
IpatchList *
swami_object_find_by_type (GObject *object, const char *type_name)
{
  ObjectFindTypeBag findbag;
  SwamiObjectPropBag *propbag;
  SwamiRoot *root;
  GType type;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  type = g_type_from_name (type_name);
  g_return_val_if_fail (type != 0, NULL);
  g_return_val_if_fail (g_type_is_a (type, G_TYPE_OBJECT), NULL);

  if (!SWAMI_IS_ROOT (object))
    {
      propbag = g_object_get_qdata (object, swami_object_propbag_quark);
      g_return_val_if_fail (propbag != NULL, NULL);
      g_return_val_if_fail (SWAMI_IS_ROOT (propbag->root), NULL);
      root = propbag->root;
    }
  else root = SWAMI_ROOT (object);

  findbag.type = type;
  findbag.list = ipatch_list_new (); /* ++ ref new list */

  SWAMI_LOCK_READ (root->proptree);

  g_node_traverse (root->proptree->tree, G_PRE_ORDER, G_TRAVERSE_ALL,
		   -1, node_traverse_match_type, &findbag);

  SWAMI_UNLOCK_READ (root->proptree);

  if (!findbag.list->items)   /* free list if no matching items */
    {
      g_object_unref (findbag.list); /* -- unref list */
      findbag.list = NULL;
    } /* sort object list by Swami rank */
  else findbag.list->items = g_list_sort (findbag.list->items,
					  swami_gcompare_object_rank);
  return (findbag.list);
}

static gboolean
node_traverse_match_type (GNode *node, gpointer data)
{
  ObjectFindTypeBag *bag = (ObjectFindTypeBag *)data;
  SwamiPropTreeNode *propnode;

  propnode = (SwamiPropTreeNode *)(node->data);
  if (g_type_is_a (G_TYPE_FROM_INSTANCE (propnode->object), bag->type))
    {
      g_object_ref (propnode->object); /* ++ ref for list */
      bag->list->items = g_list_prepend (bag->list->items, propnode->object);
    }

  return (FALSE);		/* don't stop for nothin */
}

static gint
swami_gcompare_object_rank (gconstpointer a, gconstpointer b)
{
  int arank, brank;

  swami_object_get (G_OBJECT (a), "rank", &arank, NULL);
  swami_object_get (G_OBJECT (b), "rank", &brank, NULL);
  return (arank - brank);
}

/**
 * swami_object_get_by_type:
 * @object: Swami root object (#SwamiRoot) or an object added to one
 * @type_name: Name of GObject derived GType to search for
 *
 * Lookup the highest ranking (see "rank" Swami property) object of
 * the given @type_name or derived from it. The returned object's
 * reference count is incremented and the caller is responsible for
 * removing it with g_object_unref when finished with it.
 *
 * Returns: The highest ranking object of the given type or NULL if none.
 *   Remember to unreference it when done.
 */
GObject *
swami_object_get_by_type (GObject *object, const char *type_name)
{
  IpatchList *list;
  GObject *match = NULL;

  list = swami_object_find_by_type (object, type_name);	/* ++ ref list */
  if (list)
    {
      match = (GObject *)(list->items->data);
      g_object_ref (match);
      g_object_unref (list);	/* -- unref list */
    }

  return (match);
}

/**
 * swami_object_get_valist:
 * @object: An object
 * @first_property_name: Name of first Swami property to get
 * @var_args: Pointer to store first property value in followed by
 *   property name pointer pairs, terminated with a NULL property name.
 *
 * Get Swami properties from an object.
 */
void
swami_object_get_valist (GObject *object, const char *first_property_name,
			 va_list var_args)
{
  const char *name;

  g_return_if_fail (G_IS_OBJECT (object));

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      char *error;

      pspec = g_param_spec_pool_lookup (object_property_pool, name,
					SWAMI_TYPE_ROOT, FALSE);
      if (!pspec)
	{
	  g_warning ("%s: no Swami property named `%s'", G_STRLOC, name);
	  break;
	}

      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: Swami property `%s' is not readable",
		     G_STRLOC, pspec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      object_get_property (object, pspec, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      g_value_unset (&value);
      name = va_arg (var_args, char *);
    }
}

/**
 * swami_object_get_property:
 * @object: An object
 * @property_name: Name of Swami property to get
 * @value: An initialized value to store the property value in. The value's
 *   type must be a type that the property can be transformed to.
 *
 * Get a Swami property from an object.
 */
void
swami_object_get_property (GObject *object, const char *property_name,
			   GValue *value)
{
  GParamSpec *pspec;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  pspec = g_param_spec_pool_lookup (object_property_pool, property_name,
				    SWAMI_TYPE_ROOT, FALSE);
  if (!pspec) g_warning ("%s: no Swami property named `%s'", G_STRLOC,
			 property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: Swami property `%s' is not readable",
	       G_STRLOC, pspec->name);
  else
    {
      GValue *prop_value, tmp_value = { 0, };

      /* auto-conversion of the callers value type */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
	{
	  g_value_reset (value);
	  prop_value = value;
	}
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec),
					    G_VALUE_TYPE (value)))
	{
	  g_warning ("can't retrieve Swami property `%s' of type `%s' as"
		     " value of type `%s'", pspec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		     G_VALUE_TYPE_NAME (value));
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  prop_value = &tmp_value;
	}
      object_get_property (object, pspec, prop_value);
      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
}

/**
 * swami_object_get:
 * @object: A GObject
 * @first_prop_name: Name of first Swami property to get
 * @...: First value should be a pointer to store the first property value into
 *   followed by property name and pointer pairs, terminated by a %NULL
 *   property name.
 *
 * Get multiple Swami properties from an @object.
 */
void
swami_object_get (gpointer object, const char *first_prop_name, ...)
{
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));

  va_start (var_args, first_prop_name);
  swami_object_get_valist (object, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * swami_object_set_valist:
 * @object: An object
 * @first_property_name: Name of first Swami property to set
 * @var_args: Pointer to get first property value from followed by
 *   property name pointer pairs, terminated with a NULL property name.
 *
 * Set Swami properties of an object.
 */
void
swami_object_set_valist (GObject *object, const char *first_property_name,
			 va_list var_args)
{
  GObjectNotifyQueue *nqueue;
  const char *name;

  g_return_if_fail (G_IS_OBJECT (object));

  nqueue = g_object_notify_queue_freeze (object,
					 object_property_notify_context);
  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      gchar *error = NULL;
      GParamSpec *pspec = g_param_spec_pool_lookup (object_property_pool, name,
						    SWAMI_TYPE_ROOT, FALSE);
      if (!pspec)
	{
	  g_warning ("%s: no Swami property named `%s'", G_STRLOC, name);
	  break;
	}

      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: Swami property `%s' is not writable",
		     G_STRLOC, pspec->name);
	  break;
	}

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occured */
	  break;
	}
      object_set_property (object, pspec, &value, nqueue);
      g_value_unset (&value);
      name = va_arg (var_args, char *);
    }
  g_object_notify_queue_thaw (object, nqueue);
}

/**
 * swami_object_set_property:
 * @object: An object
 * @property_name: Name of Swami property to set
 * @value: Value to set the property to. The value's type must be the same
 *   as the property's type.
 *
 * Set a Swami property of an object.
 */
void
swami_object_set_property (GObject *object, const char *property_name,
			   const GValue *value)
{
  GObjectNotifyQueue *nqueue;
  GParamSpec *pspec;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  nqueue = g_object_notify_queue_freeze (object,
					 object_property_notify_context);
  pspec = g_param_spec_pool_lookup (object_property_pool, property_name,
				    SWAMI_TYPE_ROOT, FALSE);
  if (!pspec) g_warning ("%s: no Swami property named `%s'", G_STRLOC,
			 property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: Swami property `%s' is not writable",
	       G_STRLOC, pspec->name);
  else object_set_property (object, pspec, value, nqueue);

  g_object_notify_queue_thaw (object, nqueue);
}

/**
 * swami_object_set:
 * @object: A GObject
 * @first_prop_name: Name of first Swami property to set
 * @...: First parameter should be the value to set the first property
 *   value to followed by property name and pointer pairs, terminated
 *   by a %NULL property name.
 *
 * Set multiple Swami properties on an @object.
 */
void
swami_object_set (gpointer object, const char *first_prop_name, ...)
{
  va_list var_args;
  
  g_return_if_fail (G_IS_OBJECT (object));

  va_start (var_args, first_prop_name);
  swami_object_set_valist (object, first_prop_name, var_args);
  va_end (var_args);
}

/* install a Swami property */
static void
swami_install_object_property (guint property_id, GParamSpec *pspec)
{
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */

  if (g_param_spec_pool_lookup (object_property_pool, pspec->name,
				SWAMI_TYPE_ROOT, FALSE))
    {
      g_warning (G_STRLOC ": already a Swami property named `%s'",
		 pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (object_property_pool, pspec, SWAMI_TYPE_ROOT);
}

/**
 * swami_find_object_property:
 * @property_name: Name of Swami property to find
 *
 * Find a Swami property.
 *
 * Returns: Parameter spec with @property_name or %NULL if not found.
 *   Returned parameter spec is internal and should not be modified or freed.
 */
GParamSpec *
swami_find_object_property (const char *property_name)
{
  g_return_val_if_fail (property_name != NULL, NULL);

  return (g_param_spec_pool_lookup (object_property_pool, property_name,
				    SWAMI_TYPE_ROOT, FALSE));
}

/**
 * swami_list_object_properties:
 * @n_properties: Location to store number of param specs or %NULL
 *
 * Get the list of Swami properties.
 *
 * Returns: A %NULL terminated array of GParamSpecs. Free the array when
 * finished with it, but don't mess with the contents.
 */
GParamSpec **
swami_list_object_properties (guint *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  pspecs = g_param_spec_pool_list (object_property_pool, SWAMI_TYPE_ROOT, &n);
  if (n_properties) *n_properties = n;

  return (pspecs);
}

/**
 * swami_object_get_flags:
 * @object: Object to get Swami flags from
 *
 * Get Swami flags from an object (see #SwamiObjectFlags).
 *
 * Returns: The flags value.
 */
guint
swami_object_get_flags (GObject *object)
{
  SwamiObjectPropBag *propbag;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  propbag = g_object_get_qdata (object, swami_object_propbag_quark);
  if (!propbag) return (0);

  return (propbag->flags);
}

/**
 * swami_object_set_flags:
 * @object: Object to set Swami flags on
 * @flags: Flags to set
 *
 * Sets Swami @flags in an @object. Flag bits are only set, not cleared.
 */
void
swami_object_set_flags (GObject *object, guint flags)
{
  SwamiObjectPropBag *propbag;

  g_return_if_fail (G_IS_OBJECT (object));

  G_LOCK (SwamiObjectPropBag);

  propbag = g_object_get_qdata (object, swami_object_propbag_quark);
  if (!propbag) propbag = object_new_propbag (object);
  propbag->flags |= flags;

  G_UNLOCK (SwamiObjectPropBag);
}

/**
 * swami_object_clear_flags:
 * @object: Object to clear Swami flags on
 * @flags: Flags to clear
 *
 * Clears Swami @flags in an @object. Flag bits are only cleared, not set.
 */
void
swami_object_clear_flags (GObject *object, guint flags)
{
  SwamiObjectPropBag *propbag;

  g_return_if_fail (G_IS_OBJECT (object));

  propbag = g_object_get_qdata (object, swami_object_propbag_quark);
  if (!propbag) return;
  propbag->flags &= ~flags;
}

/* dispatches swami property changes */
static void
object_property_notify_dispatcher (GObject *object, guint n_pspecs,
				   GParamSpec **pspecs)
{
  SwamiObjectPropBag *propbag;
  guint i;

  propbag = g_object_get_qdata (object, swami_object_propbag_quark);
  if (!propbag || !propbag->root) return; /* rootless objects */

  for (i = 0; i < n_pspecs; i++)
    g_signal_emit (propbag->root, root_prop_notify_signal_id,
		   g_quark_from_string (pspecs[i]->name), pspecs[i]);
}

static inline SwamiObjectPropBag *
object_new_propbag (GObject *object)
{
  SwamiObjectPropBag *propbag;

  propbag = g_slice_new0 (SwamiObjectPropBag);
  g_object_set_qdata_full (object, swami_object_propbag_quark, propbag,
			   free_propbag);
  return (propbag);
}

static void
free_propbag (gpointer data)
{
  g_slice_free (SwamiObjectPropBag, data);
}

/* Swami child object get property routine */
static void
object_get_property (GObject *object, GParamSpec *pspec, GValue *value)
{
  guint property_id = PARAM_SPEC_PARAM_ID (pspec);
  SwamiObjectPropBag *propbag;

  propbag = g_object_get_qdata (object, swami_object_propbag_quark);

  switch (property_id)
    {
    case OBJ_PROP_NAME:
      G_LOCK (SwamiObjectPropBag);
      if (propbag) g_value_set_string (value, propbag->name);
      else g_value_set_string (value, NULL);
      G_UNLOCK (SwamiObjectPropBag);
      break;
    case OBJ_PROP_RANK:
      if (propbag) g_value_set_int (value, propbag->rank);
      else g_value_set_int (value, SWAMI_RANK_NORMAL);
      break;
    case OBJ_PROP_FLAGS:
      if (propbag) g_value_set_flags (value, propbag->flags);
      else g_value_set_flags (value, 0);
      break;
    case OBJ_PROP_ROOT:
      if (propbag) g_value_set_object (value, propbag->root);
      else g_value_set_object (value, NULL);
      break;
    default:
      break;
    }
}

static void
object_set_property (GObject *object, GParamSpec *pspec, const GValue *value,
		     GObjectNotifyQueue *nqueue)
{
  GValue tmp_value = { 0, };

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set Swami property `%s' of type `%s' from"
	       " value of type `%s'", pspec->name,
	       g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value)
	   && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      char *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type `%s' is invalid for Swami"
		 " property `%s' of type `%s'",
		 contents, G_VALUE_TYPE_NAME (value), pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      guint property_id = PARAM_SPEC_PARAM_ID (pspec);
      SwamiObjectPropBag *propbag;

      G_LOCK (SwamiObjectPropBag);
      propbag = g_object_get_qdata (object, swami_object_propbag_quark);
      if (!propbag) propbag = object_new_propbag (object);

      switch (property_id)
	{
	case OBJ_PROP_NAME:
	  g_free (propbag->name);
	  propbag->name = g_value_dup_string (value);
	  break;
	case OBJ_PROP_RANK:
	  propbag->rank = g_value_get_int (value);
	  break;
	case OBJ_PROP_FLAGS:
	  propbag->flags = g_value_get_flags (value);
	  break;
	case OBJ_PROP_ROOT:
	  propbag->root = g_value_get_object (value);
	  break;
	default:
	  break;
	}

      G_UNLOCK (SwamiObjectPropBag);

      g_object_notify_queue_add (G_OBJECT (object), nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}


/**
 * swami_object_set_origin:
 * @obj: Object to assign origin object value
 * @origin: Value to assign as the origin
 *
 * This is used for associating an origin object to another object.  Currently
 * this is only used for #GtkList item selections.  The current active item
 * selection is assigned to the swamigui_root object.  In order to determine
 * what GUI widget is the source, this function can be used to associate the
 * widget object with the #GtkList selection (for example the #SwamiguiTree or
 * #SwamiguiSplits widgets).  In this way, only the selection needs to be
 * assigned to the root object and the source widget can be determined.
 */
void
swami_object_set_origin (GObject *obj, GObject *origin)
{
  g_return_if_fail (G_IS_OBJECT (obj));
  g_return_if_fail (G_IS_OBJECT (origin));

  g_object_set_data (obj, "_SwamiOrigin", origin);
}

/**
 * swami_object_get_origin:
 * @obj: Object to get the origin object of
 *
 * See swamigui_object_set_origin() for more details.
 *
 * Returns: The origin object of @obj or NULL if no origin object assigned.
 *   The caller owns a reference to the returned object and should unref it
 *   when finished.
 */
GObject *
swami_object_get_origin (GObject *obj)
{
  GObject *origin;

  g_return_val_if_fail (G_IS_OBJECT (obj), NULL);

  origin = g_object_get_data (obj, "_SwamiOrigin");
  if (origin) g_object_ref (origin);

  return (origin);
}
