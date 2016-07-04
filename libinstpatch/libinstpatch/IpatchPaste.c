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
 * SECTION: IpatchPaste
 * @short_description: Object paste instance
 * @see_also: 
 * @stability: Stable
 *
 * This object provides a system and instance for doing cut/paste operations
 * on instrument items.
 */
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
#include "misc.h"
#include "IpatchPaste.h"
#include "IpatchBase.h"
#include "IpatchConverter.h"
#include "IpatchContainer.h"
#include "IpatchTypeProp.h"
#include "IpatchVirtualContainer.h"
#include "builtin_enums.h"
#include "marshals.h"
#include "ipatch_priv.h"
#include "i18n.h"
#include "util.h"

typedef struct
{
  IpatchPasteTestFunc test_func;
  IpatchPasteExecFunc exec_func;
  GDestroyNotify notify_func;
  gpointer user_data;
  int id;
  int flags;
} PasteHandler;

/* info for an item add operation */
typedef struct
{
  IpatchItem *additem;		/* item to add */
  IpatchContainer *parent;	/* parent to add additem to */
  IpatchItem *conflict;		/* conflict item (if any) */
  IpatchPasteChoice choice;	/* choice of how to handle conflict */
} AddItemBag;

/* info for an item link operation */
typedef struct
{
  IpatchItem *from;		/* object to link from (set property) */
  IpatchItem *to;		/* object to link to */
} LinkItemBag;

static gint handler_priority_GCompareFunc (gconstpointer a, gconstpointer b);
static void ipatch_paste_class_init (IpatchPasteClass *klass);
static void ipatch_paste_init (IpatchPaste *paste);
static void ipatch_paste_finalize (GObject *gobject);
static guint resolve_hash_func (gconstpointer key);
static gboolean resolve_equal_func (gconstpointer a, gconstpointer b);
static void resolve_key_destroy_func (gpointer data);
static guint check_hash_func (gconstpointer key);
static gboolean check_equal_func (gconstpointer a, gconstpointer b);
static void check_item_conflicts_GHFunc (gpointer key, gpointer value,
					 gpointer user_data);
static IpatchItem *paste_copy_link_func_deep (IpatchItem *item,
					      IpatchItem *link,
					      gpointer user_data);

static GStaticRecMutex paste_handlers_m = G_STATIC_REC_MUTEX_INIT;
static GSList *paste_handlers = NULL;	/* list of PasteHandler structs */
static int ipatch_paste_handler_id = 0;         // handler ID counter

static gpointer parent_class = NULL;


/**
 * ipatch_register_paste_handler: (skip)
 * @test_func: Callback function to test if a paste operation is handled
 * @exec_func: Paste execution function
 * @flags: (type IpatchPastePriority): Currently just a value
 *   from #IpatchPastePriority or 0 for default priority.
 *
 * Registers a handler function to paste objects for
 * which @test_func returns %TRUE.
 */
void
ipatch_register_paste_handler (IpatchPasteTestFunc test_func,
			       IpatchPasteExecFunc exec_func,
			       int flags)
{
  ipatch_register_paste_handler_full (test_func, exec_func, NULL, NULL, flags);
}

/**
 * ipatch_register_paste_handler_full: (rename-to ipatch_register_paste_handler)
 * @test_func: (scope notified): Callback function to test if a paste operation is handled
 * @exec_func: (scope notified): Paste execution function
 * @notify_func: (nullable) (scope async) (closure user_data): Called when paste
 *   handler is unregistered
 * @user_data: (nullable): Data to pass to @notify_func or %NULL
 * @flags: (type IpatchPastePriority): Currently just a value
 *   from #IpatchPastePriority or 0 for default priority.
 *
 * Registers a handler function to paste objects for
 * which @test_func returns %TRUE.  Like ipatch_register_paste_handler() but is friendly
 * to GObject Introspection.
 *
 * Returns: Handler ID, which can be used to unregister it or -1 if invalid parameters
 *
 * Since: 1.1.0
 */
int
ipatch_register_paste_handler_full (IpatchPasteTestFunc test_func,
			            IpatchPasteExecFunc exec_func,
			            GDestroyNotify notify_func,
                                    gpointer user_data, int flags)
{
  PasteHandler *handler;
  int id;

  g_return_val_if_fail (test_func != NULL, -1);
  g_return_val_if_fail (exec_func != NULL, -1);

  if (flags == 0) flags = IPATCH_PASTE_PRIORITY_DEFAULT;

  handler = g_slice_new (PasteHandler);         // ++ alloc handler structure
  handler->test_func = test_func;
  handler->exec_func = exec_func;
  handler->notify_func = notify_func;
  handler->user_data = user_data;
  handler->flags = flags;

  g_static_rec_mutex_lock (&paste_handlers_m);
  id = ++ipatch_paste_handler_id;
  handler->id = id;
  paste_handlers = g_slist_insert_sorted (paste_handlers, handler,
					  handler_priority_GCompareFunc);
  g_static_rec_mutex_unlock (&paste_handlers_m);

  return (id);
}

static gint
handler_priority_GCompareFunc (gconstpointer a, gconstpointer b)
{
  PasteHandler *ahandler = (PasteHandler *)a, *bhandler = (PasteHandler *)b;

  /* priority sorts from highest to lowest so subtract a from b */
  return ((bhandler->flags & IPATCH_PASTE_FLAGS_PRIORITY_MASK)
	  - (ahandler->flags & IPATCH_PASTE_FLAGS_PRIORITY_MASK));
}

/**
 * ipatch_unregister_paste_handler:
 * @id: ID of handler which was returned from ipatch_register_paste_handler_full().
 *
 * Unregister a paste handler.
 *
 * Returns: %TRUE if found and unregistered, %FALSE otherwise
 *
 * Since: 1.1.0
 */
gboolean
ipatch_unregister_paste_handler (int id)
{
  PasteHandler *handler;
  GDestroyNotify notify_func = NULL;
  gpointer user_data;
  GSList *p;

  g_static_rec_mutex_lock (&paste_handlers_m);

  for (p = paste_handlers; p; p = p->next)
  {
    handler = (PasteHandler *)(p->data);

    if (handler->id == id)
    {
      paste_handlers = g_slist_delete_link (paste_handlers, p);
      notify_func = handler->notify_func;
      user_data = handler->user_data;
      g_slice_free (PasteHandler, handler);     // -- free handler structure
    }
  }

  g_static_rec_mutex_unlock (&paste_handlers_m);

  if (notify_func) notify_func (user_data);

  return (p != NULL);
}

/**
 * ipatch_simple_paste:
 * @dest: Destination item to paste to
 * @src: Source item
 * @err: Location to store error info or %NULL
 *
 * Simple paste of a single @src item to @dest item.  Any conflicts are
 * ignored which means that conflicts will remain and should be resolved.
 *
 * Returns: %TRUE on success, %FALSE otherwise in which case @err should be set.
 */
gboolean
ipatch_simple_paste (IpatchItem *dest, IpatchItem *src, GError **err)
{
  IpatchPaste *paste;

  g_return_val_if_fail (IPATCH_IS_ITEM (dest), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (src), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  paste = ipatch_paste_new ();	/* ++ ref new object */

  /* setup object paste */
  if (!ipatch_paste_objects (paste, dest, src, err))
    {	/* object paste failed */
      g_object_unref (paste);	/* -- unref paste object */
      return (FALSE);
    }

  /* finish paste operation */
  if (!ipatch_paste_finish (paste, err))
    {	/* finish paste failed */
      g_object_unref (paste);	/* -- unref paste object */
      return (FALSE);
    }

  g_object_unref (paste);

  return (TRUE);
}

/**
 * ipatch_is_paste_possible:
 * @dest: Destination item
 * @src: Source item
 *
 * Check if the given items can be pasted from @src to @dest.
 *
 * Returns: %TRUE if paste is possible, %FALSE otherwise
 */
gboolean
ipatch_is_paste_possible (IpatchItem *dest, IpatchItem *src)
{
  PasteHandler *handler;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_ITEM (dest), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (src), FALSE);

  g_static_rec_mutex_lock (&paste_handlers_m);

  for (p = paste_handlers; p; p = p->next)
    {
      handler = (PasteHandler *)(p->data);
      if (handler->test_func (dest, src)) break;
    }

  g_static_rec_mutex_unlock (&paste_handlers_m);

  return (p != NULL);
}

GType
ipatch_paste_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info = {
      sizeof (IpatchPasteClass), NULL, NULL,
      (GClassInitFunc)ipatch_paste_class_init, NULL, NULL,
      sizeof (IpatchPaste), 0,
      (GInstanceInitFunc)ipatch_paste_init,
    };

    obj_type = g_type_register_static (G_TYPE_OBJECT, "IpatchPaste",
				       &obj_info, 0);

    /* register the default handler */
    ipatch_register_paste_handler (ipatch_paste_default_test_func,
				   ipatch_paste_default_exec_func, 0);
  }

  return (obj_type);
}

static void
ipatch_paste_class_init (IpatchPasteClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = ipatch_paste_finalize;
}

static void
ipatch_paste_init (IpatchPaste *paste)
{
  paste->add_hash = g_hash_table_new (NULL, NULL);
}

/* function called when a patch is being destroyed */
static void
ipatch_paste_finalize (GObject *gobject)
{
  IpatchPaste *paste = IPATCH_PASTE (gobject);
  AddItemBag *addbag;
  LinkItemBag *linkbag;
  GSList *p;

  g_hash_table_destroy (paste->add_hash);

  for (p = paste->add_list; p; p = p->next)
    {
      addbag = (AddItemBag *)(p->data);
      g_object_unref (addbag->additem);
      g_object_unref (addbag->parent);
      if (addbag->conflict) g_object_unref (addbag->conflict);
    }

  for (p = paste->link_list; p; p = p->next)
    {
      linkbag = (LinkItemBag *)(p->data);
      g_object_unref (linkbag->from);
      g_object_unref (linkbag->to);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

/*
 * ipatch_paste_new:
 *
 * Create a new paste object for patch object paste operations.
 *
 * Returns: New paste object with refcount of 1 which caller owns.
 */
IpatchPaste *
ipatch_paste_new (void)
{
  return (IPATCH_PASTE (g_object_new (IPATCH_TYPE_PASTE, NULL)));
}

/**
 * ipatch_paste_objects:
 * @paste: Paste object
 * @dest: Destination item of paste
 * @src: Source item of paste
 * @err: Location to store error info or %NULL
 *
 * Setup a paste operation.  Multiple item pastes can occur for the same
 * @paste instance.  Existing duplicated items are used if present (example:
 * if multiple instruments are pasted between different IpatchBase objects
 * and they link to the same sample, they will both use the same sample in
 * the final paste operation).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
ipatch_paste_objects (IpatchPaste *paste, IpatchItem *dest, IpatchItem *src,
		      GError **err)
{
  PasteHandler *handler = NULL;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (dest), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (src), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  g_static_rec_mutex_lock (&paste_handlers_m);

  for (p = paste_handlers; p; p = p->next)
    {
      handler = (PasteHandler *)(p->data);
      if (handler->test_func (dest, src)) break;
    }

  g_static_rec_mutex_unlock (&paste_handlers_m);

  if (!p)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
		   _("Pasting object of type %s to %s is unsupported"),
		   G_OBJECT_TYPE_NAME (src),
		   G_OBJECT_TYPE_NAME (dest));
      return (FALSE);
    }

  return (handler->exec_func (paste, dest, src, err));
}

/* hash key used by ipatch_paste_resolve */
typedef struct
{ /* NOTE: item and parent are ref'd by paste->add_list, no need here */
  IpatchItem *item;		/* item for these property values */
  IpatchContainer *parent;	/* parent of item (or proposed parent) */
  GValueArray *valarray;	/* array of all of item's unique prop. values */
  GParamSpec **pspecs;	/* parameter specs for these properties (NULL term.) */
  guint8 index;		/* index to first value/pspec of this unique group */
  guint8 count;		/* # of values/pspecs in this unique prop group */
  guint8 free_valarray;	/* boolean, TRUE if hash should free valarray */
} ResolveHashKey;

/* bag used for passing multiple vars to check_item_conflicts_GHFunc */
typedef struct
{
  IpatchPaste *paste;
  GHashTable *confl_hash;
  IpatchPasteResolveFunc resolve_func;
  gboolean cancel;
} CheckBag;

/**
 * ipatch_paste_resolve:
 * @paste: Paste object
 * @resolve_func: (scope call) (closure user_data): Resolve callback function
 *   which is invoked for each conflict.
 * @user_data: User defined data to pass to @resolve_func.
 *
 * This function is used to make choices as to how conflicts are resolved.
 * Conflicting objects are those with identical unique property values.  For
 * each conflicting object that would result from a paste, the @resolve_func is
 * called allowing a choice to be made as to how it is handled.  The default
 * choice is to ignore the duplicate, resulting in conflicting objects.
 * This function can be executed multiple times, the choices are only executed
 * once ipatch_paste_finish() is called.
 *
 * Returns: %TRUE on success, %FALSE if operation was canceled (@resolve_func
 *   returned #IPATCH_PASTE_CHOICE_CANCEL).
 */
gboolean
ipatch_paste_resolve (IpatchPaste *paste, IpatchPasteResolveFunc resolve_func,
		      gpointer user_data)
{
  GHashTable *confl_hash;	/* hash of add item unique property values */
  GHashTable *check_hash;	/* hash of parent children to check */
  GParamSpec **pspecs, **ps;
  AddItemBag *addbag, *confl_addbag;
  ResolveHashKey *key, skey;
  CheckBag checkbag;
  GValueArray *valarray;
  gboolean free_valarray;
  guint32 groups;
  GSList *p;
  int i, lastbit, count, index, choice;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (resolve_func != NULL, FALSE);

  /* create conflict hash, we try and increase performance by hashing items
     and their unique property values (rather than having to compare every
     item to every other possible conflicting item) */
  confl_hash = g_hash_table_new_full (resolve_hash_func, resolve_equal_func,
				      resolve_key_destroy_func, NULL);

  /* hash of Parent:ChildType to check for conflicts. Uses existing
     ResolveHashKey structures. */
  check_hash = g_hash_table_new (check_hash_func, check_equal_func);

  /* add paste items to hash and detect conflicts with other paste items */
  for (p = paste->add_list; p; p = p->next)
    {
      addbag = (AddItemBag *)(p->data);

      /* get item's unique property values (if any) */
      valarray = ipatch_item_get_unique_props (addbag->additem);
      if (!valarray) continue;	/* no unique props? skip */
      free_valarray = TRUE;

      /* get corresponding property param specs and group bits */
      pspecs = ipatch_item_type_get_unique_specs (G_OBJECT_TYPE (addbag->additem),
						   &groups);
      if (!pspecs)	/* should never happen! */
	{
	  g_value_array_free (valarray);
	  continue;
	}

      lastbit = (groups & 1);
      count = 0;
      index = 0;

      /* loop over unique property groups */
      for (i = 0, ps = pspecs;   ;   i++, ps++, groups >>= 1)
	{
	  if (!*ps || lastbit != (groups & 1))	/* last val or end of group? */
	    { /* initialize the static hash key */
	      skey.valarray = valarray;
	      skey.item = addbag->additem;
	      skey.parent = addbag->parent;
	      skey.pspecs = pspecs;
	      skey.index = index;
	      skey.count = count;

	      /* check for conflict within paste items */
	      confl_addbag = g_hash_table_lookup (confl_hash, &skey);

	      if (!confl_addbag)	/* no conflict? */
		{ /* add to conflict detect hash */
		  key = g_new (ResolveHashKey, 1);
		  *key = skey;	/* copy static key to allocated key */
		  key->free_valarray = free_valarray;	/* free once only */
		  g_hash_table_insert (confl_hash, key, addbag);

		  free_valarray = FALSE;	/* hash will free it */

		  /* Parent:ItemType not yet added to check_hash? */
		  if (!g_hash_table_lookup (check_hash, key))
		    g_hash_table_insert (check_hash, key, addbag);

		  if (!*ps) break;	/* NULL pspec terminator? */
		}
	      else	/* we have a conflict, tell caller about it */
		{
		  choice = resolve_func (paste, confl_addbag->additem,
					 addbag->additem);
		  /* cancel requested? */
		  if (choice == IPATCH_PASTE_CHOICE_CANCEL)
		    {
		      g_hash_table_destroy (confl_hash);
		      g_hash_table_destroy (check_hash);
		      return (FALSE);
		    }

		  addbag->conflict = g_object_ref (confl_addbag->additem);
		  addbag->choice = choice;
		}

	      index = i;	/* next group index */
	      count = 1;	/* reset group count */
	      lastbit = (groups & 1);
	    }
	  else count++;	/* not end of group, increment current val count */
	}

      /* if valarray was not used in hash, then free it here */
      if (free_valarray) g_value_array_free (valarray);
    }

  /* bag for passing multilple variables to hash foreach function */
  checkbag.paste = paste;
  checkbag.confl_hash = confl_hash;
  checkbag.resolve_func = resolve_func;
  checkbag.cancel = FALSE;

  g_hash_table_foreach (check_hash, check_item_conflicts_GHFunc, &checkbag);

  g_hash_table_destroy (confl_hash);
  g_hash_table_destroy (check_hash);

  if (checkbag.cancel) return (FALSE);

  return (TRUE);
}

/* hash function used for detecting conflicting items quickly (in theory) */
static guint
resolve_hash_func (gconstpointer key)
{
  ResolveHashKey *rkey = (ResolveHashKey *)key;
  GValueArray *valarray;
  GValue *value;
  guint hashval;
  int i, end;

  /* hash the parent, item type and group value index */
  hashval = GPOINTER_TO_UINT (rkey->parent);
  hashval += G_OBJECT_TYPE (rkey->item);

  i = rkey->index;
  hashval += i;

  valarray = rkey->valarray;

  end = i + rkey->count;	/* value end index */

  for (; i < end; i++)	/* hash the property values for this group */
    {
      value = g_value_array_get_nth (valarray, i);
      hashval += ipatch_util_value_hash (value);
    }

  return (hashval);
}

/* hash key compare func for ResolveHashKeys */
static gboolean
resolve_equal_func (gconstpointer a, gconstpointer b)
{
  ResolveHashKey *akey = (ResolveHashKey *)a, *bkey = (ResolveHashKey *)b;
  GValue *aval, *bval;
  int i, end;

  /* value index, parent or item type not the same? */
  if (akey->index != bkey->index
      || akey->parent != bkey->parent
      || G_OBJECT_TYPE (akey->item) != G_OBJECT_TYPE (bkey->item))
    return (FALSE);

  i = akey->index;
  end = i + akey->count;

  for (; i < end; i++)	/* see if the property values are the same */
    {
      aval = g_value_array_get_nth (akey->valarray, i);
      bval = g_value_array_get_nth (bkey->valarray, i);

      if (g_param_values_cmp (akey->pspecs[i], aval, bval) != 0)
	return (FALSE);
    }

  return (TRUE);	/* keys match (conflict) */
}

static void
resolve_key_destroy_func (gpointer data)
{
  ResolveHashKey *rkey = (ResolveHashKey *)data;

  if (rkey->free_valarray) g_value_array_free (rkey->valarray);
  g_free (rkey);
}

/* hash function used for hashing parent:ItemTypes to check later for
   duplicates.  They are hashed so that we only have to check the given
   children once. */
static guint
check_hash_func (gconstpointer key)
{
  ResolveHashKey *rkey = (ResolveHashKey *)key;

  return (GPOINTER_TO_UINT (rkey->parent) + G_OBJECT_TYPE (rkey->item));
}

/* hash key compare func for Parent:ItemType check hash */
static gboolean
check_equal_func (gconstpointer a, gconstpointer b)
{
  ResolveHashKey *akey = (ResolveHashKey *)a, *bkey = (ResolveHashKey *)b;

  return (akey->parent == bkey->parent
	  && G_OBJECT_TYPE (akey->item) == G_OBJECT_TYPE (bkey->item));
}

/* check for conflicts in existing items using Parent:ItemType hash */
static void
check_item_conflicts_GHFunc (gpointer key, gpointer value,
			     gpointer user_data)
{
  CheckBag *checkbag = (CheckBag *)user_data;
  ResolveHashKey *rkey = (ResolveHashKey *)key;
  ResolveHashKey skey;	/* static key */
  IpatchPasteResolveFunc resolve_func = (IpatchPasteResolveFunc)user_data;
  AddItemBag *confl_addbag;
  GParamSpec **pspecs, **ps;
  GValueArray *valarray;
  guint32 groups;
  IpatchIter iter;
  IpatchItem *item;
  IpatchList *list;
  int i, lastbit, count, index, choice;

  if (checkbag->cancel) return;	/* operation cancelled? */

  /* get children of specific type for parent */
  list = ipatch_container_get_children (rkey->parent,
					G_OBJECT_TYPE (rkey->item));
  ipatch_list_init_iter (list, &iter);

  /* get property param specs and group bits (all items are of same type) */
  pspecs = ipatch_item_type_get_unique_specs (G_OBJECT_TYPE (rkey->item),
					      &groups);
  /* loop over container children */
  for (item = ipatch_item_first (&iter); item; item = ipatch_item_next (&iter))
    {
      /* get item's unique property values */
      valarray = ipatch_item_get_unique_props (item);
      if (!valarray) continue;	/* no unique props? skip (shouldn't happen) */

      lastbit = (groups & 1);
      count = 0;
      index = 0;

      /* loop over unique property groups */
      for (i = 0, ps = pspecs;   ;   i++, ps++, groups >>= 1)
	{
	  if (!*ps || lastbit != (groups & 1))	/* last val or end of group? */
	    { /* initialize the static hash key */
	      skey.valarray = valarray;
	      skey.item = item;
	      skey.parent = rkey->parent;
	      skey.pspecs = pspecs;
	      skey.index = index;
	      skey.count = count;

	      /* check for conflict within paste items */
	      confl_addbag = g_hash_table_lookup (checkbag->confl_hash, &skey);

	      if (confl_addbag)	/* conflict? */
		{
		  choice = resolve_func (checkbag->paste, item,
					 confl_addbag->additem);
		  /* cancel requested? */
		  if (choice == IPATCH_PASTE_CHOICE_CANCEL)
		    {
		      checkbag->cancel = TRUE;
		      g_value_array_free (valarray);
		      g_object_unref (list);	/* -- unref list */
		      return;
		    }

		  confl_addbag->conflict = g_object_ref (item);
		  confl_addbag->choice = choice;
		}

	      index = i;	/* next group index */
	      count = 1;	/* reset group count */
	      lastbit = (groups & 1);
	    }
	  else count++;	/* not end of group, increment current val count */

	g_value_array_free (valarray);

      }	/* for each unique parameter group */

    }	/* for container items */

  g_object_unref (list);	/* -- unref list */
}

/**
 * ipatch_paste_finish:
 * @paste: Paste object
 * @err: Location to store error info or %NULL
 *
 * Complete the paste operation(s) (add/link objects).  Conflicts are handled
 * for the choices made with ipatch_paste_resolve() (defaults to ignore which
 * will result in conflicts).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_paste_finish (IpatchPaste *paste, GError **err)
{
  AddItemBag *addbag;
  LinkItemBag *linkbag;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  /* add items in add list */
  for (p = paste->add_list; p; p = p->next)
    {
      addbag = (AddItemBag *)(p->data);

      if (addbag->choice == IPATCH_PASTE_CHOICE_IGNORE)
	ipatch_container_add (addbag->parent, addbag->additem);

      /* FIXME - Need to implement replace operation */
    }

  /* link items in link list */
  for (p = paste->link_list; p; p = p->next)
    {
      linkbag = (LinkItemBag *)(p->data);
      g_object_set (linkbag->from, "link-item", linkbag->to, NULL);
    }

  return (TRUE);
}

/**
 * ipatch_paste_get_add_list:
 * @paste: Paste object
 *
 * Get list of objects to add with paste operation.  This can be called
 * after ipatch_paste_objects() or after ipatch_paste_finish().  In the first
 * case the objects have not yet been added, in the second case the paste
 * operation has been completed.  The list of objects returned are only those
 * which are not conflicting or a choice of #IPATCH_PASTE_CHOICE_IGNORE or
 * #IPATCH_PASTE_CHOICE_REPLACE was selected.
 *
 * Returns: (transfer full): List of objects being added with paste operation or %NULL if none.
 *   Returned list has a refcount of 1 which the caller owns, unref when done.
 */
IpatchList *
ipatch_paste_get_add_list (IpatchPaste *paste)
{
  IpatchList *retlist;
  GList *newlist = NULL;
  AddItemBag *bag;
  GSList *p;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), NULL);

  for (p = paste->add_list; p; p = p->next)
  {
    bag = (AddItemBag *)(p->data);

    if (bag->choice == IPATCH_PASTE_CHOICE_IGNORE
	|| bag->choice == IPATCH_PASTE_CHOICE_REPLACE)
      newlist = g_list_prepend (newlist, g_object_ref (bag->additem));
  }

  if (newlist)
  {
    retlist = ipatch_list_new ();	/* ++ ref list */
    retlist->items = g_list_reverse (newlist);  /* reverse the list, since we prepended */
    return (retlist);			/* !! caller takes over reference */
  }

  return (NULL);
}

/**
 * ipatch_paste_object_add:
 * @paste: Paste object
 * @additem: New item to add.
 * @parent: Container to parent @additem to.
 * @orig: (nullable): Original item associated with @additem (if duplicated for example).
 *   If supplied then an association between the @orig object and the @additem
 *   will be made, and any references to @orig of subsequent deep duplications
 *   will use the new @additem instead.
 *
 * Used by #IpatchPasteExecFunc handlers.  Adds an object addition operation
 * to a paste instance.
 */
void
ipatch_paste_object_add (IpatchPaste *paste, IpatchItem *additem, 
			 IpatchContainer *parent, IpatchItem *orig)
{
  AddItemBag *addbag;

  g_return_if_fail (IPATCH_IS_PASTE (paste));
  g_return_if_fail (IPATCH_IS_ITEM (additem));
  g_return_if_fail (IPATCH_IS_CONTAINER (parent));
  g_return_if_fail (!orig || IPATCH_IS_ITEM (orig));

  /* create a bag to hold the item add info and add to add_list */
  addbag = g_new (AddItemBag, 1);
  addbag->additem = g_object_ref (additem);
  addbag->parent = g_object_ref (parent);
  addbag->conflict = NULL;
  addbag->choice = IPATCH_PASTE_CHOICE_IGNORE;

  if (paste->add_list_last)
  {
    paste->add_list_last = g_slist_append (paste->add_list_last, addbag);
    paste->add_list_last = paste->add_list_last->next;
  }
  else  /* Empty list */
  {
    paste->add_list = g_slist_append (paste->add_list, addbag);
    paste->add_list_last = paste->add_list;
  }

  /* set up an association to the original item */
  if (orig) g_hash_table_insert (paste->add_hash, orig, addbag);
}

/**
 * ipatch_paste_object_add_duplicate:
 * @paste: Paste object
 * @item: Item to duplicate and add
 * @parent: Container to parent duplicated item to.
 *
 * Used by #IpatchPasteExecFunc handlers.  Duplicates an item and adds an
 * addition operation to a paste instance.  Useful for duplicating an object
 * within the same IpatchBase parent.  For this reason the duplicated item is
 * automatically forced to be unique and no association is added for @item to
 * the new duplicate.
 *
 * Returns: (transfer none): The new duplicate of @item (no reference added for caller).
 */
IpatchItem *
ipatch_paste_object_add_duplicate (IpatchPaste *paste, IpatchItem *item,
				   IpatchContainer *parent)
{
  IpatchItem *dup;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), NULL);
  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);
  g_return_val_if_fail (IPATCH_IS_CONTAINER (parent), NULL);

  dup = ipatch_item_duplicate (item);	/* ++ ref new dup */

  /* make unique if requested */
  ipatch_container_make_unique (IPATCH_CONTAINER (parent), dup);

  /* add the object add operation of the duplicated item */
  ipatch_paste_object_add (paste, dup, IPATCH_CONTAINER (parent), NULL);

  g_object_unref (dup);	/* -- unref dup object */

  return (dup);
}

/* bag used in ipatch_paste_object_add_duplicate_deep() */
typedef struct
{
  IpatchPaste *paste;
  IpatchContainer *dest_base;
} DupDeepBag;

/**
 * ipatch_paste_object_add_duplicate_deep:
 * @paste: Paste object
 * @item: Item to deep duplicate and add.
 * @parent: Container to parent @item to.
 *
 * Used by #IpatchPasteExecFunc handlers.  Deep duplicates @item and registers
 * it as an add to @parent in the @paste operation, also registers all new
 * duplicated dependencies of @item.  Any existing matching duplicate items in
 * the @paste instance are used rather than duplicating them again.
 *
 * Returns: (transfer none): The new duplicate of @item (no reference added for caller).
 */
IpatchItem *
ipatch_paste_object_add_duplicate_deep (IpatchPaste *paste, IpatchItem *item,
					IpatchContainer *parent)
{
  DupDeepBag bag;
  IpatchItem *dup;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), NULL);
  g_return_val_if_fail (IPATCH_IS_ITEM (item), NULL);
  g_return_val_if_fail (IPATCH_IS_CONTAINER (parent), NULL);

  bag.paste = paste;
  bag.dest_base = IPATCH_CONTAINER
    (ipatch_item_get_base (IPATCH_ITEM (parent))); /* ++ ref base */

  /* deep duplicate the item (custom link function to use existing dups) */
  dup = ipatch_item_duplicate_link_func (item, paste_copy_link_func_deep, &bag);

  /* add the duplicate object addition operation to paste instance */
  ipatch_paste_object_add (paste, dup, parent, item);

  g_object_unref (dup);		/* !! paste instance owns ref */

  g_object_unref (bag.dest_base);	/* -- unref base */

  return (dup);
}

/* IpatchItemCopyLinkFunc for deep duplicating an object and dependencies but
   using existing dups in paste instance, if any */
static IpatchItem *
paste_copy_link_func_deep (IpatchItem *item, IpatchItem *link,
			   gpointer user_data)
{
  DupDeepBag *dupbag = (DupDeepBag *)user_data;
  AddItemBag *bag;
  IpatchItem *dup;

  if (!link) return (NULL);

  /* look up link item in paste add hash */
  bag = g_hash_table_lookup (dupbag->paste->add_hash, link);

  /* FIXME - HACK until SoundFont stereo handling is improved.
   * Reciprocal stereo linking cluster #&*!s things. */
  if (IPATCH_IS_SF2_SAMPLE (item))
  {
    if (!bag) return (NULL);

    /* Re-link the other sample */
    ipatch_sf2_sample_set_linked (IPATCH_SF2_SAMPLE (bag->additem),
                                  IPATCH_SF2_SAMPLE (item));

    return (bag->additem);
  }

  if (!bag)	/* link not in hash? - Duplicate link and add it to paste. */
    {
      dup = g_object_new (G_OBJECT_TYPE (link), NULL); /* ++ ref new item */
      g_return_val_if_fail (dup != NULL, NULL);

      ipatch_paste_object_add (dupbag->paste, dup, dupbag->dest_base, link);

      /* recursively copy the link object to the duplicate (finish duping) */
      ipatch_item_copy_link_func (dup, link, paste_copy_link_func_deep,
				  user_data);

      g_object_unref (dup);	/* !! paste instance holds a ref */
    }
  else dup = bag->additem;

  return (dup);
}

/**
 * ipatch_paste_object_add_convert:
 * @paste: Paste object
 * @conv_type: IpatchConverter derived type to use for conversion.
 * @item: Item to convert and add.
 * @parent: Container to parent converted item to.
 * @item_list: (out) (optional): Location to store pointer to the list of added items or %NULL
 *   to ignore.  Caller owns a reference to the list.
 * @err: Location to store error info or %NULL to ignore.
 *
 * Used by #IpatchPasteExecFunc handlers.  Converts @item using an
 * #IpatchConverter of type @conv_type and registers
 * it as an add to @parent in the @paste operation, also registers all new
 * dependencies of @item.  Any existing matching converted item dependencies in
 * the @paste instance are used rather than duplicating them again.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_paste_object_add_convert (IpatchPaste *paste, GType conv_type,
				 IpatchItem *item, IpatchContainer *parent,
                                 IpatchList **item_list, GError **err)
{
  IpatchConverter *converter;
  const IpatchConverterInfo *convinfo;
  IpatchList *list;
  GObject *dest;
  GList *p;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (g_type_is_a (conv_type, IPATCH_TYPE_CONVERTER), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (item), FALSE);
  g_return_val_if_fail (IPATCH_IS_CONTAINER (parent), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  convinfo = ipatch_lookup_converter_info (conv_type, 0, 0);
  g_return_val_if_fail (convinfo != NULL, FALSE);

  converter = IPATCH_CONVERTER (g_object_new (conv_type, NULL));  /* ++ ref */
  g_return_val_if_fail (converter != NULL, FALSE);

  ipatch_converter_add_input (converter, G_OBJECT (item));

  /* check if converter needs its destination item supplied */
  if (convinfo->dest_count == IPATCH_CONVERTER_COUNT_ONE_OR_MORE
      || convinfo->dest_count == 1)
    {
      dest = g_object_new (convinfo->dest_type, NULL);	/* ++ ref */
      if (log_if_fail (dest != NULL))
	{
	  g_object_unref (converter);	/* -- unref converter */
	  return (FALSE);
	}

      ipatch_converter_add_output (converter, dest);
      g_object_unref (dest);	/* -- unref */
    }
  else if (log_if_fail (convinfo->dest_count == 0))
    {
      g_object_unref (converter);	/* -- unref converter */
      return (FALSE);
    }

  if (!ipatch_converter_convert (converter, err))
    {
      g_object_unref (converter);	/* -- unref converter */
      return (FALSE);
    }

  list = ipatch_converter_get_outputs (converter);      /* ++ ref list */
  g_object_unref (converter);	/* -- unref converter */

  /* add objects to paste operation */
  for (p = list->items; p; p = p->next)
    ipatch_paste_object_add (paste, IPATCH_ITEM (p->data), parent, item);

  if (item_list) *item_list = list;     /* !! caller takes over reference */
  else g_object_unref (list);   /* -- unref list */

  return (TRUE);
}

/**
 * ipatch_paste_object_link:
 * @paste: Paste object
 * @from: Item to link from
 * @to: Item to link to
 *
 * Used by #IpatchPasteExecFunc handlers.  Registers a link operation.
 */
void
ipatch_paste_object_link (IpatchPaste *paste, IpatchItem *from, IpatchItem *to)
{
  LinkItemBag *linkbag;

  g_return_if_fail (IPATCH_IS_PASTE (paste));
  g_return_if_fail (IPATCH_IS_ITEM (from));
  g_return_if_fail (IPATCH_IS_ITEM (to));

  /* create a bag to hold the item link info and add to link_list */
  linkbag = g_new (LinkItemBag, 1);
  linkbag->from = g_object_ref (from);
  linkbag->to = g_object_ref (to);

  paste->link_list = g_slist_prepend (paste->link_list, linkbag);
}

/**
 * ipatch_paste_default_test_func:
 * @dest: Destination item of paste operation
 * @src: Source item of paste operation
 *
 * Default #IpatchPasteTestFunc.  Useful for alternative paste implementations
 * which would like to chain to the default function (to override only specific
 * object types for example).
 *
 * Returns: %TRUE if paste supported by this handler, %FALSE otherwise
 */
gboolean
ipatch_paste_default_test_func (IpatchItem *dest, IpatchItem *src)
{
  GType src_type, link_type, type;
  const GType *child_types = NULL, *ptype;
  GParamSpec *spec;

  g_return_val_if_fail (IPATCH_IS_ITEM (dest), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (src), FALSE);

  src_type = G_OBJECT_TYPE (src);

  /* destination is a container? */
  if (IPATCH_IS_CONTAINER (dest))
    {
      /* get child types for destination container */
      child_types = ipatch_container_get_child_types (IPATCH_CONTAINER (dest));
      if (!child_types) return (FALSE);		/* no child types??!! */

      /* check if src type in child types */
      for (ptype = child_types; *ptype; ptype++)
	if (g_type_is_a (src_type, *ptype))
	  return (TRUE);	/* if child type found, paste supported */

      /* src is a link type of any of container's children types? */
      for (ptype = child_types; *ptype; ptype++)
	{
	  ipatch_type_get (*ptype, "link-type", &link_type, NULL);
	  if (g_type_is_a (src_type, link_type))
	    return (TRUE);	/* link type found, paste supported */
	}
    }
  else if (IPATCH_IS_VIRTUAL_CONTAINER (dest))	/* dest is a virtual container? */
    {
      IpatchItem *child_obj;

      /* get the child type of the virtual container */
      ipatch_type_get (G_OBJECT_TYPE (dest), "virtual-child-type", &type, NULL);

      /* does source object conform to the virtual container child type? */
      if (type && g_type_is_a (G_OBJECT_TYPE (src), type))
	return (TRUE);

      /* or can it be pasted to the child type recursively? */
      child_obj = g_object_new (type, NULL); /* ++ ref child object */
      if (child_obj)
      {
	if (ipatch_is_paste_possible(child_obj, src))
	{
	  g_object_unref (child_obj);	/* -- unref child_obj */
	  return (TRUE); /* can be pasted recursively into the child type */
	}
	g_object_unref (child_obj);	/* -- unref child_obj */
      }
    }
  else	/* destination is not a container - src is link type of dest? */
    {
      /* dest has link item property (FIXME - Future proof?) */
      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest),
					   "link-item");
      /* link item property and src is of the required type? */
      if (spec && g_type_is_a (src_type, spec->value_type))
	return (TRUE);
    }


  /* ## see if paste could occur if source object is converted ## */


  /* destination is a container? */
  if (IPATCH_IS_CONTAINER (dest))
    {
      /* child_types already retrieved above */

      /* check if src type can be converted to any child types */
      for (ptype = child_types; *ptype; ptype++)
	{
	  if (ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src), *ptype))
	    return (TRUE);
	}

      /* can src be converted to a container's child link type? */
      for (ptype = child_types; *ptype; ptype++)
	{
	  ipatch_type_get (*ptype, "link-type", &link_type, NULL);

	  if (ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src), link_type))
	    return (TRUE);
	}
    }
  else if (IPATCH_IS_VIRTUAL_CONTAINER (dest))	/* dest is a virtual container? */
    {
      /* get the child type of the virtual container */
      ipatch_type_get (G_OBJECT_TYPE (dest), "virtual-child-type", &type, NULL);

      if (type)
	{
	  /* can object be converted to container child type? */
	  if (ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src), type))
	    return (TRUE);
	}
    }
  else	/* dest is not a container - can convert src to link type of dest? */
    {
      /* dest has link item property (FIXME - Future proof?) */
      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest),
					   "link-item");
      if (!spec) return (FALSE);

      /* can src be converted to link type of dest? */
      if (ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src),
					spec->value_type))
	return (TRUE);
    }

  return (FALSE);
}

/**
 * ipatch_paste_default_exec_func:
 * @paste: Paste object
 * @src: Source object of paste
 * @dest: Destination object of paste
 * @err: Location to store error info or %NULL
 *
 * Default #IpatchPasteExecFunc.  Useful for alternative paste implementations
 * which would like to chain to the default function (to override only specific
 * object types for example).
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set).
 */
gboolean
ipatch_paste_default_exec_func (IpatchPaste *paste, IpatchItem *dest,
				IpatchItem *src, GError **err)
{
  IpatchItem *src_base, *dest_base;
  IpatchItem *link, *dup;
  GParamSpec *spec;
  GType src_type, link_type, type;
  const GType *child_types = NULL, *ptype;
  const IpatchConverterInfo *convinfo, *matchinfo;
  IpatchVirtualContainerConformFunc conform_func;
  IpatchList *list;

  g_return_val_if_fail (IPATCH_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (src), FALSE);
  g_return_val_if_fail (IPATCH_IS_ITEM (dest), FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  src_base = ipatch_item_get_base (src);        // ++ ref
  dest_base = ipatch_item_get_base (dest);      // ++ ref
  src_type = G_OBJECT_TYPE (src);

  /* destination is a container? */
  if (IPATCH_IS_CONTAINER (dest))
    {
      /* get child types for destination container */
      child_types = ipatch_container_get_child_types (IPATCH_CONTAINER (dest));
      if (!child_types) goto not_handled;

      /* check if src type in child types */
      for (ptype = child_types; *ptype; ptype++)
	if (g_type_is_a (src_type, *ptype)) break;

      if (*ptype)	/* matching child type found? */
	{
	  /* paste is local (within the same IpatchBase?) */
	  if (src_base == dest_base)
	    ipatch_paste_object_add_duplicate (paste, src, IPATCH_CONTAINER (dest));
	  else	/* deep duplicate the object and add to the paste instance */
	    ipatch_paste_object_add_duplicate_deep (paste, src,
						    IPATCH_CONTAINER (dest));
	  goto ret_ok;
	}

      /* src is a link type of any of container's children types? */
      for (ptype = child_types; *ptype; ptype++)
	{
	  ipatch_type_get (*ptype, "link-type", &link_type, NULL);
	  if (g_type_is_a (src_type, link_type)) break;
	}
    
      /* matching link type found? */
      if (*ptype)
	{
	  GObject *newchild;
    
	  newchild = g_object_new (*ptype, NULL);	/* ++ ref new child object */
	  if (!newchild)
	    {
	      g_warning ("Failed to create linked child of type %s -> %s",
			 g_type_name (*ptype), g_type_name (link_type));
	      goto not_handled;
	    }
    
	  /* add the object add operation of the new child */
	  ipatch_paste_object_add (paste, IPATCH_ITEM (newchild),
				   IPATCH_CONTAINER (dest), NULL);
    
	  /* link the new child item to the source object */
	  g_object_set (newchild, "link-item", src, NULL);
    
	  g_object_unref (newchild);	/* -- unref creator's ref */
    
	  goto ret_ok;	/* paste was handled */
	}    
    }
  else if (IPATCH_IS_VIRTUAL_CONTAINER (dest))	/* dest is a virtual container? */
    {
      IpatchItem *newchild;
      GValue val = { 0 };

      /* get the child type of the virtual container */
      ipatch_type_get (G_OBJECT_TYPE (dest),
		       "virtual-child-type", &type,
		       "virtual-child-conform-func", &conform_func,
		       NULL);
      if (!type) goto not_handled;

      /* does source object conform to the virtual container type? */
      if (g_type_is_a (G_OBJECT_TYPE (src), type))
	{
	  /* if src is foreign, deep duplicate it, otherwise local duplicate */
	  if (src_base != dest_base)
	    dup = ipatch_paste_object_add_duplicate_deep (paste, src,
						IPATCH_CONTAINER (dest_base));
	  else dup = ipatch_paste_object_add_duplicate (paste, src,
						IPATCH_CONTAINER (dest_base));

	  if (conform_func) conform_func (G_OBJECT (dup));

	  goto ret_ok;	/* paste was handled */
	}

      /* can it be pasted into the child type recursively? */
      newchild = g_object_new (type, NULL);	/* ++ ref new child object */
      if (!newchild)
      {
	g_warning ("Failed to create child of type %s", g_type_name (type));
	goto not_handled;
      }

      if (conform_func) conform_func (G_OBJECT (newchild));

      if (ipatch_is_paste_possible(newchild, src))
      {
	if (!ipatch_simple_paste(newchild, src, err))
	{
	  g_object_unref (newchild);	/* -- unref creator's ref */
	  goto ret_err;	/* paste not handled */
	}

	/* Inherit title of the new item from the pasted one */
	g_value_init (&val, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (src), "title", &val);
	g_object_set_property (G_OBJECT (newchild), "name", &val);
	g_value_unset (&val);
	ipatch_container_make_unique (IPATCH_CONTAINER (dest_base), newchild);

	/* add the object add operation of the new child */
	ipatch_paste_object_add (paste, IPATCH_ITEM (newchild),
				 IPATCH_CONTAINER (dest_base), NULL);
	g_object_unref (newchild);	/* -- unref creator's ref */
	goto ret_ok;	/* paste was handled */
      }
      g_object_unref (newchild);	/* -- unref creator's ref */
    }
  else	/* destination is not a container - src is link type of dest? */
    {
      /* dest has link item property (FIXME - Future proof?) */
      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest),
					   "link-item");
      /* if no link item property or src isn't of the required type - error */
      if (!spec || !g_type_is_a (src_type, spec->value_type))
	goto not_handled;

      /* if src is foreign, duplicate it */
      if (src_base != dest_base)
	link = ipatch_paste_object_add_duplicate_deep
	  (paste, src, IPATCH_CONTAINER (dest_base));
      else link = src;

      /* add the link operation */
      ipatch_paste_object_link (paste, dest, link);

      goto ret_ok;	/* we done */
    }


  /* ## see if paste could occur if source object is converted ## */


  /* destination is a container? */
  if (IPATCH_IS_CONTAINER (dest))
    {
      /* child_types already retrieved above */

      /* check if src type can be converted to any child types, pick the highest
         rated converter */
      matchinfo = NULL;
      for (ptype = child_types; *ptype; ptype++)
	{
	  convinfo = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src),
						   *ptype);
	  if (convinfo &&
	      (!matchinfo || convinfo->priority > matchinfo->priority))
	    matchinfo = convinfo;
	}

      if (matchinfo)	/* found a converter match? */
      {
        if (ipatch_paste_object_add_convert (paste, matchinfo->conv_type, src,
                                             IPATCH_CONTAINER (dest), NULL, err))
          goto ret_ok;
        else goto ret_err;
      }

      /* can src be converted to a container's child link type? */
      for (ptype = child_types; *ptype; ptype++)
	{
	  ipatch_type_get (*ptype, "link-type", &link_type, NULL);

	  convinfo = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src),
						   link_type);
	  if (convinfo &&
	      (!matchinfo || convinfo->priority > matchinfo->priority))
	    matchinfo = convinfo;
	}
    
      /* matching converter found? */
      if (matchinfo)
	{
	  GObject *newchild;

	  /* convert the source object to the matching link type */
	  if (!ipatch_paste_object_add_convert (paste, matchinfo->conv_type,
				                src, IPATCH_CONTAINER (dest_base),
                                                &list, err))    /* ++ ref list */
            goto ret_err;

	  newchild = g_object_new (*ptype, NULL);  /* ++ ref new child object */
	  if (!newchild)
	    {
	      g_warning ("Failed to create linked child of type %s -> %s",
			 g_type_name (*ptype), g_type_name (link_type));
              g_object_unref (list);    /* -- unref list */
	      goto not_handled;
	    }

	  /* add the object add operation of the new child */
	  ipatch_paste_object_add (paste, IPATCH_ITEM (newchild),
				   IPATCH_CONTAINER (dest), NULL);

	  /* link the new child item to the converted source object */
	  g_object_set (newchild, "link-item", list->items->data, NULL);

	  g_object_unref (newchild);	/* -- unref creator's ref */
          g_object_unref (list);    /* -- unref list */

	  goto ret_ok;	/* paste was handled */
	}    
    }
  else if (IPATCH_IS_VIRTUAL_CONTAINER (dest))	/* dest is a virtual container? */
    {
      /* get the child type of the virtual container */
      ipatch_type_get (G_OBJECT_TYPE (dest),
		       "virtual-child-type", &type,
		       "virtual-child-conform-func", &conform_func,
		       NULL);
      if (!type) goto not_handled;

      /* can object be converted to container child type? */
      convinfo = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src), type);
      if (!convinfo) goto not_handled;

      /* add the conversion operation to the paste */
      if (!ipatch_paste_object_add_convert (paste, convinfo->conv_type, src,
				            IPATCH_CONTAINER (dest_base),
                                            &list, err))     /* ++ ref list */
        goto ret_err;

      if (conform_func) conform_func (G_OBJECT (list->items->data));

      g_object_unref (list);          /* -- unref list */

      goto ret_ok;	/* paste was handled */
    }
  else	/* dest is not a container - can convert src to link type of dest? */
    {
      /* dest has link item property (FIXME - Future proof?) */
      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dest), "link-item");
      if (!spec) goto not_handled;

      /* can src be converted to link type of dest? */
      convinfo = ipatch_lookup_converter_info (0, G_OBJECT_TYPE (src),
					       spec->value_type);
      if (!convinfo) goto not_handled;

      /* convert the src object to the link type and add to paste operation */
      if (!ipatch_paste_object_add_convert (paste, convinfo->conv_type,
                                            src, IPATCH_CONTAINER (dest_base),
                                            &list, err))      /* ++ ref list */
        goto ret_err;

      /* add the link operation */
      ipatch_paste_object_link (paste, dest, IPATCH_ITEM (list->items->data));

      g_object_unref (list);          /* -- unref list */

      goto ret_ok;	/* we done */
    }

 not_handled:
  g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNHANDLED_CONVERSION,
	       _("Unhandled paste operation type '%s' => '%s'"),
	       G_OBJECT_TYPE_NAME (src), G_OBJECT_TYPE_NAME (dest));
  // Fall through

 ret_err:
  if (src_base) g_object_unref (src_base);      // -- unref
  if (dest_base) g_object_unref (dest_base);    // -- unref
  return (FALSE);

 ret_ok:
  if (src_base) g_object_unref (src_base);      // -- unref
  if (dest_base) g_object_unref (dest_base);    // -- unref
  return (TRUE);
}

