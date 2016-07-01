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
/*
 * IpatchItemProp.c - IpatchItem property change callback system
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchItem.h"
#include "IpatchParamProp.h"
#include "ipatch_priv.h"

/* hash key for IpatchItem property matching */
typedef struct
{
  IpatchItem *item;		/* IpatchItem to match for callback */
  GParamSpec *pspec;		/* GParamSpec to match for callback */
} PropMatchKey;

/* hash value used for IpatchItem property callbacks */
typedef struct
{
  IpatchItemPropCallback callback; /* callback function */
  IpatchItemPropDisconnect disconnect; /* called when callback is disconnected */
  GDestroyNotify notify_func;   /* Either notify_func or disconnect will be set, but not both */
  gpointer user_data;		/* user data to pass to function */
  guint handler_id;		/* unique handler ID */
} PropCallback;

static void prop_match_key_free (gpointer data);
static guint prop_callback_hash_func (gconstpointer key);
static gboolean prop_callback_equal_func (gconstpointer a, gconstpointer b);
static guint
ipatch_item_prop_real_connect (IpatchItem *item, GParamSpec *pspec,
                               IpatchItemPropCallback callback,
                               IpatchItemPropDisconnect disconnect,
                               GDestroyNotify notify_func,
                               gpointer user_data);
static void
ipatch_item_prop_real_disconnect (guint handler_id, IpatchItem *item,
				  GParamSpec *pspec,
				  IpatchItemPropCallback callback,
				  gpointer user_data);
static gboolean prop_callback_hash_GHRFunc (gpointer key, gpointer value,
					    gpointer user_data);

/* lock for prop_callback_next_id, prop_callback_hash and
   wild_prop_callback_list */
G_LOCK_DEFINE_STATIC (prop_callbacks);

static guint prop_callback_next_id = 1; /* next handler ID */

/* hash of IpatchItem prop callbacks
   (PropMatchKey -> GSList<PropCallback>) */
static GHashTable *prop_callback_hash;

/* wildcard callbacks (item and property are wildcard) */
static GSList *wild_prop_callback_list = NULL;


/* called by ipatch_init() */
void
_ipatch_item_prop_init (void)
{
  /* create IpatchItem property callback hash */
  prop_callback_hash
    = g_hash_table_new_full (prop_callback_hash_func, prop_callback_equal_func,
			     prop_match_key_free, NULL);
}

/* GDestroyNotify function to free prop_callback_hash keys */
static void
prop_match_key_free (gpointer data)
{
  g_slice_free (PropMatchKey, data);
}

/* hash function for IpatchItem property notify callback hash */
static guint
prop_callback_hash_func (gconstpointer key)
{
  const PropMatchKey *propkey = (const PropMatchKey *)key;
  return (GPOINTER_TO_UINT (propkey->item)
	  + GPOINTER_TO_UINT (propkey->pspec));
}

/* hash equal function for IpatchItem property notify callback hash */
static gboolean
prop_callback_equal_func (gconstpointer a, gconstpointer b)
{
  const PropMatchKey *akey = (const PropMatchKey *)a;
  const PropMatchKey *bkey = (const PropMatchKey *)b;
  return (akey->item == bkey->item && akey->pspec == bkey->pspec);
}

/**
 * ipatch_item_prop_notify:
 * @item: Item whose property changed
 * @pspec: Parameter specification for @item of parameter that changed
 * @new_value: The new value that was assigned
 * @old_value: (nullable): Old value that property had (can be %NULL for read only props)
 *
 * Usually only used by #IpatchItem object implementations, rather
 * than explicitly called by the user.  It should be called AFTER item
 * property changes that occur outside of the #IpatchItem
 * <structfield>item_set_property</structfield> method.
 */
void
ipatch_item_prop_notify (IpatchItem *item, GParamSpec *pspec,
			 const GValue *new_value, const GValue *old_value)
{
  /* dynamically adjustable max callbacks to allocate cbarray for */
  static guint max_callbacks = 64;
  PropMatchKey cbkey;
  PropCallback *cbarray;	/* NULL <callback> terminated array */
  PropCallback *cb;
  PropCallback *old_cbarray;
  guint old_max_callbacks;
  GSList *match_both, *match_item, *match_prop, *wild_list;
  guint cbindex = 0, i;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (G_IS_VALUE (new_value));
  g_return_if_fail (!old_value || G_IS_VALUE (old_value));

  /* should save-able state change be indicated (set #IpatchBase dirty flag) */
  if (!(pspec->flags & IPATCH_PARAM_NO_SAVE_CHANGE))
      ipatch_item_changed (item);

  /* if hooks not active for item, just return */
  if (!(ipatch_item_get_flags (item) & IPATCH_ITEM_HOOKS_ACTIVE)) return;

  /* key for IpatchItem property callback hash */
  cbkey.item = item;
  cbkey.pspec = pspec;

  /* allocate callback array on stack (for performance) */
  cbarray = g_alloca (max_callbacks * sizeof (PropCallback));

  G_LOCK (prop_callbacks);

  /* lookup callback list for this IpatchItem:Property */
  match_both = g_hash_table_lookup (prop_callback_hash, &cbkey);

  /* lookup callback list for IpatchItem:*  */
  cbkey.pspec = NULL;
  match_item = g_hash_table_lookup (prop_callback_hash, &cbkey);

  /* lookup callback list for *:Property  */
  cbkey.pspec = pspec;
  cbkey.item = NULL;
  match_prop = g_hash_table_lookup (prop_callback_hash, &cbkey);

  wild_list = wild_prop_callback_list;

  /* duplicate lists into array (since we will call them outside of lock) */

  for (; match_both && cbindex < max_callbacks; match_both = match_both->next)
    cbarray[cbindex++] = *(PropCallback *)(match_both->data);

  for (; match_item && cbindex < max_callbacks; match_item = match_item->next)
    cbarray[cbindex++] = *(PropCallback *)(match_item->data);

  for (; match_prop && cbindex < max_callbacks; match_prop = match_prop->next)
    cbarray[cbindex++] = *(PropCallback *)(match_prop->data);

  for (; wild_list && cbindex < max_callbacks; wild_list = wild_list->next)
    cbarray[cbindex++] = *(PropCallback *)(wild_list->data);

  if (match_both || match_item || match_prop || wild_list)
    {	/* We exceeded max_callbacks (Bender just shit a brick) */
      old_cbarray = cbarray;
      old_max_callbacks = max_callbacks;

      cbindex += g_slist_length (match_both);
      cbindex += g_slist_length (match_item);
      cbindex += g_slist_length (match_prop);
      cbindex += g_slist_length (wild_list);

      max_callbacks = cbindex + 16; /* plus some for kicks */

      cbarray = g_alloca (max_callbacks * sizeof (PropCallback));
      memcpy (cbarray, old_cbarray, old_max_callbacks * sizeof (PropCallback));

      cbindex = old_max_callbacks;

      /* duplicate rest of the lists */
      for (; match_both && cbindex < max_callbacks; match_both = match_both->next)
	cbarray[cbindex++] = *(PropCallback *)(match_both->data);
    
      for (; match_item && cbindex < max_callbacks; match_item = match_item->next)
	cbarray[cbindex++] = *(PropCallback *)(match_item->data);
    
      for (; match_prop && cbindex < max_callbacks; match_prop = match_prop->next)
	cbarray[cbindex++] = *(PropCallback *)(match_prop->data);

      for (; wild_list && cbindex < max_callbacks; wild_list = wild_list->next)
	cbarray[cbindex++] = *(PropCallback *)(wild_list->data);
    }

  G_UNLOCK (prop_callbacks);

  if (cbindex)
    {
      IpatchItemPropNotify info = { 0 };

      info.item = item;
      info.pspec = pspec;
      info.new_value = new_value;
      info.old_value = old_value;
    
      /* call the callbacks */
      for (i = 0; i < cbindex; i++)
	{
	  cb = &cbarray[i];
	  info.user_data = cb->user_data;
	  cb->callback (&info);
	}

      /* call event data destroy functions if any have been set */
      if (info.eventdata[0].destroy)
	info.eventdata[0].destroy (info.eventdata[0].data);
      if (info.eventdata[1].destroy)
	info.eventdata[1].destroy (info.eventdata[1].data);
      if (info.eventdata[2].destroy)
	info.eventdata[2].destroy (info.eventdata[2].data);
      if (info.eventdata[3].destroy)
	info.eventdata[3].destroy (info.eventdata[3].data);
    }
}

/**
 * ipatch_item_prop_notify_by_name:
 * @item: Item whose property changed
 * @prop_name: Name of property that changed
 * @new_value: The new value that was assigned
 * @old_value: (nullable): Old value that property had (can be %NULL
 *   for read only properties)
 *
 * Usually only used by #IpatchItem object implementations, rather
 * than explicitly called by the user. Like ipatch_item_prop_notify()
 * except takes a property name instead of a parameter spec, for
 * added convenience.
 */
void
ipatch_item_prop_notify_by_name (IpatchItem *item, const char *prop_name,
				 const GValue *new_value,
				 const GValue *old_value)
{
  GObjectClass *klass;
  GParamSpec *pspec, *redirect;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (prop_name != NULL);
  g_return_if_fail (G_IS_VALUE (new_value));
  g_return_if_fail (!old_value || G_IS_VALUE (old_value));

  /* get the item's class and lookup the property */
  klass = G_OBJECT_GET_CLASS (item);
  pspec = g_object_class_find_property (klass, prop_name);
  g_return_if_fail (pspec != NULL);

  /* use overridden param spec if its an override param spec */
  redirect = g_param_spec_get_redirect_target (pspec);
  if (redirect) pspec = redirect;

  ipatch_item_prop_notify (item, pspec, new_value, old_value);
}

/**
 * ipatch_item_prop_connect: (skip)
 * @item: (nullable): IpatchItem to match (or %NULL for wildcard)
 * @pspec: (nullable): Property parameter specification to match (or %NULL for wildcard)
 * @callback: (scope notified): Callback function
 * @disconnect: (nullable) (closure user_data): Callback disconnect function
 *   (called when the callback is disconnected) can be %NULL.
 * @user_data: (nullable): User defined data to pass to @callback and @disconnect function.
 *
 * Connect a callback for a specific #IpatchItem and property. If a property
 * change occurs for the given @item and @pspec then the callback is
 * invoked.  The parameters @item and/or @pspec may be %NULL for wild card
 * matching (if both are %NULL then callback will be called for all #IpatchItem
 * property changes).
 *
 * Returns: Unique handler ID which can be used to remove the handler or 0 on
 *   error (only when function params are incorrect).
 */
guint
ipatch_item_prop_connect (IpatchItem *item, GParamSpec *pspec,
			  IpatchItemPropCallback callback,
			  IpatchItemPropDisconnect disconnect,
			  gpointer user_data)
{
  return (ipatch_item_prop_real_connect (item, pspec, callback, disconnect, NULL, user_data));
}

/**
 * ipatch_item_prop_connect_notify: (rename-to ipatch_item_prop_connect)
 * @item: (nullable): IpatchItem to match (or %NULL for wildcard)
 * @pspec: (nullable): Property parameter specification to match (or %NULL for wildcard)
 * @callback: (scope notified): Callback function
 * @notify_func: (scope async) (closure user_data) (nullable): Callback destroy notify
 *   (called when the callback is disconnected) can be %NULL.
 * @user_data: (nullable): User defined data to pass to @callback and @disconnect function.
 *
 * Connect a callback for a specific #IpatchItem and property. If a property
 * change occurs for the given @item and @pspec then the callback is
 * invoked.  The parameters @item and/or @pspec may be %NULL for wild card
 * matching (if both are %NULL then callback will be called for all #IpatchItem
 * property changes).
 *
 * Returns: Unique handler ID which can be used to remove the handler or 0 on
 *   error (only when function params are incorrect).
 *
 * Since: 1.1.0
 */
guint
ipatch_item_prop_connect_notify (IpatchItem *item, GParamSpec *pspec,
                                 IpatchItemPropCallback callback,
                                 GDestroyNotify notify_func,
                                 gpointer user_data)
{
  return (ipatch_item_prop_real_connect (item, pspec, callback, NULL, notify_func, user_data));
}

static guint
ipatch_item_prop_real_connect (IpatchItem *item, GParamSpec *pspec,
                               IpatchItemPropCallback callback,
                               IpatchItemPropDisconnect disconnect,
                               GDestroyNotify notify_func,
                               gpointer user_data)
{
  PropMatchKey *cbkey = NULL;
  PropCallback *cb;
  GSList *cblist;
  guint handler_id;

  g_return_val_if_fail (!item || IPATCH_IS_ITEM (item), 0);
  g_return_val_if_fail (!pspec || G_IS_PARAM_SPEC (pspec), 0);
  g_return_val_if_fail (callback != NULL, 0);

  if (item || pspec)
    {
      cbkey = g_slice_new (PropMatchKey);
      cbkey->item = item;
      cbkey->pspec = pspec;
    }

  cb = g_slice_new (PropCallback);
  cb->callback = callback;
  cb->disconnect = disconnect;
  cb->notify_func = notify_func;
  cb->user_data = user_data;

  G_LOCK (prop_callbacks);

  handler_id = prop_callback_next_id++;
  cb->handler_id = handler_id;

  if (cbkey)
    {
      /* get existing list for IpatchItem:Property (if any) */
      cblist = g_hash_table_lookup (prop_callback_hash, cbkey);
    
      /* if IpatchItem:Property already exists then cbkey gets freed here */
      g_hash_table_insert (prop_callback_hash, cbkey,
			   g_slist_prepend (cblist, cb));
    }
  else	/* callback is completely wildcard, just add to the wildcard list */
    wild_prop_callback_list = g_slist_prepend (wild_prop_callback_list, cb);

  G_UNLOCK (prop_callbacks);

  return (handler_id);
}

/**
 * ipatch_item_prop_connect_by_name: (skip)
 * @item: (nullable): IpatchItem to match (or %NULL for wildcard)
 * @prop_name: Name of property of @item to match
 * @callback: Callback function
 * @disconnect: (nullable): Callback disconnect function (called when the callback is
 *   disconnect) can be NULL.
 * @user_data: (closure): User defined data to pass to @callback and @disconnect function.
 * 
 * Like ipatch_item_prop_add_callback() but takes the name of a property
 * instead of the parameter spec, for added convenience.
 *
 * Returns: Unique handler ID which can be used to remove the handler or 0 on
 *   error (only when function params are incorrect).
 */
guint
ipatch_item_prop_connect_by_name (IpatchItem *item, const char *prop_name,
				  IpatchItemPropCallback callback,
				  IpatchItemPropDisconnect disconnect,
				  gpointer user_data)
{
  GParamSpec *pspec;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), 0);
  g_return_val_if_fail (prop_name != NULL, 0);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), prop_name);
  g_return_val_if_fail (pspec != NULL, 0);

  return (ipatch_item_prop_real_connect (item, pspec, callback, disconnect, NULL, user_data));
}

/**
 * ipatch_item_prop_connect_by_name_notify: (rename-to ipatch_item_prop_connect_by_name)
 * @item: (nullable): IpatchItem to match (or %NULL for wildcard)
 * @prop_name: Name of property of @item to match
 * @callback: Callback function
 * @notify_func: (scope async) (closure user_data) (nullable): Callback destroy notify
 *   (called when the callback is disconnected) can be %NULL.
 * @user_data: (nullable): User defined data to pass to @callback and @disconnect function.
 * 
 * Like ipatch_item_prop_add_callback() but takes the name of a property
 * instead of the parameter spec, for added convenience.
 *
 * Returns: Unique handler ID which can be used to remove the handler or 0 on
 *   error (only when function params are incorrect).
 *
 * Since: 1.1.0
 */
guint
ipatch_item_prop_connect_by_name_notify (IpatchItem *item, const char *prop_name,
                                         IpatchItemPropCallback callback,
                                         GDestroyNotify notify_func,
                                         gpointer user_data)
{
  GParamSpec *pspec;

  g_return_val_if_fail (IPATCH_IS_ITEM (item), 0);
  g_return_val_if_fail (prop_name != NULL, 0);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), prop_name);
  g_return_val_if_fail (pspec != NULL, 0);

  return (ipatch_item_prop_real_connect (item, pspec, callback, NULL, notify_func, user_data));
}

/* a bag used in next 2 removal functions */
typedef struct
{
  gboolean found;		/* Set to TRUE if handler found */
  PropMatchKey key;		/* out: (remove_callback func), in: (matched func) */
  PropCallback cb;		/* in: handler_id, out: disconnect, user_data */
  PropMatchKey update_key; 	/* out: key of list root requiring update */
  GSList *update_list;		/* out: new root of list to update */
  gboolean update_needed;	/* out: set if root of list needs update */
} PropRemoveBag;

/**
 * ipatch_item_prop_disconnect:
 * @handler_id: Handler ID as returned from ipatch_item_prop_connect().
 * 
 * Disconnects an #IpatchItem property change callback handler by its ID.
 */
void
ipatch_item_prop_disconnect (guint handler_id)
{
  g_return_if_fail (handler_id != 0);
  ipatch_item_prop_real_disconnect (handler_id, NULL, NULL, NULL, NULL);
}

/**
 * ipatch_item_prop_disconnect_matched: (skip)
 * @item: (nullable): #IpatchItem of handler to match (does not need to be valid)
 * @pspec: (nullable): GParamSpec of handler to match (does not need to be valid)
 * @callback: Callback function to match
 * @user_data: (nullable): User data to match
 * 
 * Disconnects first #IpatchItem property change callback matching all the
 * function parameters.
 * Note: Only the pointer values of @item and @pspec are used so they don't
 * actually need to be valid anymore.
 */
void
ipatch_item_prop_disconnect_matched (IpatchItem *item, GParamSpec *pspec,
				     IpatchItemPropCallback callback,
				     gpointer user_data)
{
  g_return_if_fail (callback != NULL);
  ipatch_item_prop_real_disconnect (0, item, pspec, callback, user_data);
}

/**
 * ipatch_item_prop_disconnect_by_name: (skip)
 * @item: #IpatchItem of handler to match (NOTE: Must be a valid object!)
 * @prop_name: Name of property of @item to match
 * @callback: (nullable): Callback function to match
 * @user_data: (nullable): User data to match
 *
 * Like ipatch_item_prop_disconnect_matched() but takes a name of the
 * property to match instead of a parameter spec, for added convenience.
 * Note: @item must still be valid (to look up the property), contrary to
 * ipatch_item_prop_disconnect_matched().
 */
void
ipatch_item_prop_disconnect_by_name (IpatchItem *item, const char *prop_name,
				     IpatchItemPropCallback callback,
				     gpointer user_data)
{
  GParamSpec *pspec;

  g_return_if_fail (IPATCH_IS_ITEM (item));
  g_return_if_fail (prop_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), prop_name);
  g_return_if_fail (pspec != NULL);

  ipatch_item_prop_disconnect_matched (item, pspec, callback, user_data);
}

/* used by ipatch_item_prop_disconnect and ipatch_item_prop_disconnect_matched.
 * Either handler_id should be set to a non-zero value or the other
 * parameters should be assigned but not both. */
static void
ipatch_item_prop_real_disconnect (guint handler_id, IpatchItem *item,
				  GParamSpec *pspec,
				  IpatchItemPropCallback callback,
				  gpointer user_data)
{
  PropMatchKey *update_key;
  PropRemoveBag bag = { 0 };
  PropCallback *cb;
  GSList *p;

  g_return_if_fail (handler_id != 0 || callback != NULL);
  g_return_if_fail (handler_id == 0 || callback == NULL);

  if (!handler_id)	/* find by match criteria? */
    {
      bag.key.item = item;
      bag.key.pspec = pspec;
      bag.cb.callback = callback;
      bag.cb.user_data = user_data;
    }
  else bag.cb.handler_id = handler_id; /* find by handler ID */

  G_LOCK (prop_callbacks);

  /* only look in hash if ID search or item or pspec specified (not wildcard) */
  if (handler_id || (item || pspec))
    g_hash_table_foreach_remove (prop_callback_hash, prop_callback_hash_GHRFunc,
				 &bag);

  /* hash entry did not get removed and requires list root to be updated,
     this can't be done in the GHRFunc */
  if (bag.update_needed)
    { /* allocate and copy key for insert (gets freed) */
      update_key = g_slice_new (PropMatchKey);
      *update_key = bag.update_key;
      g_hash_table_insert (prop_callback_hash, update_key, bag.update_list);
    }

  /* if not found and find ID or !item and !pspec (wildcard), check wildcard list */
  if (!bag.found && (handler_id || (!item && !pspec)))
    {
      for (p = wild_prop_callback_list; p; p = p->next)
	{
	  cb = (PropCallback *)(p->data);

	  /* callback matches criteria? (handler_id or callback:user_data) */
	  if ((handler_id && cb->handler_id == handler_id)
	      || (!handler_id && cb->callback == callback
		  && cb->user_data == user_data))
	    {
	      bag.found = TRUE;
	      bag.cb.disconnect = cb->disconnect;
              bag.cb.notify_func = cb->notify_func;
	      bag.cb.user_data = cb->user_data;
	      g_slice_free (PropCallback, cb);

	      wild_prop_callback_list
		= g_slist_delete_link (wild_prop_callback_list, p);
	      break;
	    }
	}
    }

  G_UNLOCK (prop_callbacks);

  if (!bag.found)
    {
      if (handler_id)
	g_critical (G_STRLOC ": Failed to find handler with ID '%d'", handler_id);
      else g_critical (G_STRLOC ": Failed to find handler matching criteria %p:%p:%p:%p",
		       item, pspec, callback, user_data);
    }

  /* see if callback found and it had a disconnect func or notify_func */
  if (bag.cb.disconnect)
    bag.cb.disconnect (bag.key.item, bag.key.pspec, bag.cb.user_data);
  else if (bag.cb.notify_func)
    bag.cb.notify_func (bag.cb.user_data);
}

/* finds a handler by ID or item/property/callback/user_data and removes it */
static gboolean
prop_callback_hash_GHRFunc (gpointer key, gpointer value, gpointer user_data)
{
  PropRemoveBag *bag = (PropRemoveBag *)user_data;
  PropMatchKey *propkey = (PropMatchKey *)key;
  GSList *cblist = (GSList *)value, *p, *newroot;
  PropCallback *cb;

  p = cblist;
  while (p)		    /* loop over callbacks in callback list */
    {
      cb = (PropCallback *)(p->data);

      /* criteria matches? (by ID or by match) */
      if ((bag->cb.handler_id && cb->handler_id == bag->cb.handler_id)
	  || (!bag->cb.handler_id && propkey->item == bag->key.item
	      && propkey->pspec == bag->key.pspec
	      && cb->callback == bag->cb.callback
	      && cb->user_data == bag->cb.user_data))
	{
	  /* return callback's item, pspec, disconnect func and user_data */
	  bag->found = TRUE;
	  bag->cb.disconnect = cb->disconnect;
          bag->cb.notify_func = cb->notify_func;
	  bag->cb.user_data = cb->user_data;
	  bag->key.item = propkey->item;
	  bag->key.pspec = propkey->pspec;

	  g_slice_free (PropCallback, cb);
	  newroot = g_slist_delete_link (cblist, p);

	  if (!newroot) return (TRUE);  /* no more list? Remove hash entry */

	  /* if root not the same, return update information
	     (can't be done in GHRFunc) */
	  if (newroot != cblist)
	    {
	      bag->update_needed = TRUE;
	      bag->update_key = *(PropMatchKey *)key;
	      bag->update_list = newroot;
	    }

	  return (FALSE);
	}
      p = g_slist_next (p);
    }

  return (FALSE);
}
