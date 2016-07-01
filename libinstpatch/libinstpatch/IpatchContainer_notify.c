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
 * IpatchContainer_notify.c - Container add/remove callback notify system
 */
#include <string.h>
#include "IpatchContainer.h"
#include "ipatch_priv.h"

/* hash value used for IpatchContainer callbacks */
typedef struct
{
  IpatchContainerCallback callback; /* callback function */
  IpatchContainerDisconnect disconnect; /* called when callback is disconnected */
  GDestroyNotify notify_func;   /* destroy notify function (this or disconnect will be set but not both) */
  gpointer user_data;		/* user data to pass to function */
  guint handler_id;		/* unique handler ID */
} ContainerCallback;


static guint
ipatch_container_real_add_connect (IpatchContainer *container,
                                   IpatchContainerCallback callback,
                                   IpatchContainerDisconnect disconnect,
                                   GDestroyNotify notify_func,
                                   gpointer user_data);
static guint
ipatch_container_real_remove_connect (IpatchContainer *container,
                                 IpatchItem *child,
                                 IpatchContainerCallback callback,
                                 IpatchContainerDisconnect disconnect,
                                 GDestroyNotify notify_func,
                                 gpointer user_data);
static void
ipatch_container_real_disconnect (guint handler_id, IpatchContainer *container,
				  IpatchItem *child,
				  IpatchContainerCallback callback,
				  gpointer user_data, gboolean isadd);
static gboolean callback_hash_GHRFunc (gpointer key, gpointer value,
				       gpointer user_data);


/* lock for add_callback_next_id, add_callback_hash, and add_wild_callback_list */
G_LOCK_DEFINE_STATIC (add_callbacks);

static guint add_callback_next_id = 1; /* next container add handler ID */

/* hash of container add callbacks
   (IpatchContainer -> GSList<PropCallback>) */
static GHashTable *add_callback_hash;
static GSList *add_wild_callback_list = NULL;	/* container add wildcard cbs */

/* lock for remove_callback_next_id, remove_callback_hash and remove_wild_callback_list */
G_LOCK_DEFINE_STATIC (remove_callbacks);

static guint remove_callback_next_id = 1;  /* next container remove handler ID */

/* container remove callbacks */
static GHashTable *remove_container_callback_hash;  /* IpatchContainer -> GSList<ContainerCallback> */
static GHashTable *remove_child_callback_hash;  /* IpatchItem -> GSList<ContainerCallback> */
static GSList *remove_wild_callback_list = NULL;  /* container add wildcard cbs */

/**
 * _ipatch_container_notify_init: (skip)
 */
void
_ipatch_container_notify_init (void)
{
  /* one time hash init for container callbacks */
  add_callback_hash = g_hash_table_new (NULL, NULL);
  remove_container_callback_hash = g_hash_table_new (NULL, NULL);
  remove_child_callback_hash = g_hash_table_new (NULL, NULL);
}

/**
 * ipatch_container_add_notify:
 * @container: Container item for which an item add occurred
 * @child: Child which was added
 *
 * Notify that a child add has occurred to an #IpatchContainer object.
 * Should be called after the add has occurred.  This function is normally
 * not needed except when using ipatch_container_insert_iter().
 */
void
ipatch_container_add_notify (IpatchContainer *container, IpatchItem *child)
{
  /* dynamically adjustable max callbacks to allocate cbarray for */
  static guint max_callbacks = 64;
  ContainerCallback *cbarray;	/* stack allocated callback array */
  ContainerCallback *cb;
  ContainerCallback *old_cbarray;
  guint old_max_callbacks;
  GSList *match_container, *wild_list;
  guint cbindex = 0, i;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (child));

  /* Container has changed */
  ipatch_item_changed ((IpatchItem *)container);

  /* if hooks not active for container, just return */
  if (!(ipatch_item_get_flags (container) & IPATCH_ITEM_HOOKS_ACTIVE)) return;

  /* allocate callback array on stack (for performance) */
  cbarray = g_alloca (max_callbacks * sizeof (ContainerCallback));

  G_LOCK (add_callbacks);

  /* lookup callback list for this container */
  match_container = g_hash_table_lookup (add_callback_hash, container);

  wild_list = add_wild_callback_list;

  /* duplicate lists into array (since we will call them outside of lock) */

  for (; match_container && cbindex < max_callbacks;
       match_container = match_container->next, cbindex++)
    {
      cb = (ContainerCallback *)(match_container->data);
      cbarray[cbindex].callback = cb->callback;
      cbarray[cbindex].user_data = cb->user_data;
    }

  for (; wild_list && cbindex < max_callbacks;
       wild_list = wild_list->next, cbindex++)
    {
      cb = (ContainerCallback *)(wild_list->data);
      cbarray[cbindex].callback = cb->callback;
      cbarray[cbindex].user_data = cb->user_data;
    }

  if (match_container || wild_list)
    {	/* We exceeded max_callbacks (Bender just shit a brick) */
      old_cbarray = cbarray;
      old_max_callbacks = max_callbacks;

      cbindex += g_slist_length (match_container);
      cbindex += g_slist_length (wild_list);

      max_callbacks = cbindex + 16; /* plus some for kicks */

      cbarray = g_alloca (max_callbacks * sizeof (ContainerCallback));
      memcpy (cbarray, old_cbarray, old_max_callbacks * sizeof (ContainerCallback));

      cbindex = old_max_callbacks;

      /* duplicate rest of the lists */
      for (; match_container && cbindex < max_callbacks;
	   match_container = match_container->next, cbindex++)
	{
	  cb = (ContainerCallback *)(match_container->data);
	  cbarray[cbindex].callback = cb->callback;
	  cbarray[cbindex].user_data = cb->user_data;
	}
    
      for (; wild_list && cbindex < max_callbacks;
	   wild_list = wild_list->next, cbindex++)
	{
	  cb = (ContainerCallback *)(wild_list->data);
	  cbarray[cbindex].callback = cb->callback;
	  cbarray[cbindex].user_data = cb->user_data;
	}
    }

  G_UNLOCK (add_callbacks);

  /* call the callbacks */
  for (i = 0; i < cbindex; i++)
    {
      cb = &cbarray[i];
      cb->callback (container, child, cb->user_data);
    }
}

/**
 * ipatch_container_remove_notify:
 * @container: Container item for which a remove will occur
 * @child: Child which will be removed
 *
 * Notify that a container remove will occur to an #IpatchContainer object.
 * Should be called before the remove occurs.  This function is normally not
 * needed, except when using ipatch_container_remove_iter().
 */
void
ipatch_container_remove_notify (IpatchContainer *container, IpatchItem *child)
{
  /* dynamically adjustable max callbacks to allocate cbarray for */
  static guint max_callbacks = 64;
  ContainerCallback *cbarray;	/* stack allocated callback array */
  ContainerCallback *cb;
  ContainerCallback *old_cbarray;
  guint old_max_callbacks;
  GSList *match_container, *match_child, *wild_list;
  guint cbindex = 0, i;

  g_return_if_fail (IPATCH_IS_CONTAINER (container));
  g_return_if_fail (IPATCH_IS_ITEM (child));

  /* Container has changed */
  ipatch_item_changed ((IpatchItem *)container);

  /* if hooks not active for container, just return */
  if (!(ipatch_item_get_flags (container) & IPATCH_ITEM_HOOKS_ACTIVE)) return;

  /* allocate callback array on stack (for performance) */
  cbarray = g_alloca (max_callbacks * sizeof (ContainerCallback));

  G_LOCK (remove_callbacks);

  /* lookup callback list for container */
  match_container = g_hash_table_lookup (remove_container_callback_hash, container);

  /* lookup callback list for child */
  match_child = g_hash_table_lookup (remove_child_callback_hash, child);

  wild_list = remove_wild_callback_list;

  /* duplicate lists into array (since we will call them outside of lock) */

  for (; match_container && cbindex < max_callbacks;
       match_container = match_container->next, cbindex++)
    {
      cb = (ContainerCallback *)(match_container->data);
      cbarray[cbindex].callback = cb->callback;
      cbarray[cbindex].user_data = cb->user_data;
    }

  for (; match_child && cbindex < max_callbacks;
       match_child = match_child->next, cbindex++)
    {
      cb = (ContainerCallback *)(match_child->data);
      cbarray[cbindex].callback = cb->callback;
      cbarray[cbindex].user_data = cb->user_data;
    }

  for (; wild_list && cbindex < max_callbacks;
       wild_list = wild_list->next, cbindex++)
    {
      cb = (ContainerCallback *)(wild_list->data);
      cbarray[cbindex].callback = cb->callback;
      cbarray[cbindex].user_data = cb->user_data;
    }

  if (match_container || match_child || wild_list)
    {	/* We exceeded max_callbacks (Bender just shit a brick) */
      old_cbarray = cbarray;
      old_max_callbacks = max_callbacks;

      cbindex += g_slist_length (match_container);
      cbindex += g_slist_length (match_child);
      cbindex += g_slist_length (wild_list);

      max_callbacks = cbindex + 16; /* plus some for kicks */

      cbarray = g_alloca (max_callbacks * sizeof (ContainerCallback));
      memcpy (cbarray, old_cbarray, old_max_callbacks * sizeof (ContainerCallback));

      cbindex = old_max_callbacks;

      /* duplicate rest of the lists */

      for (; match_container && cbindex < max_callbacks;
	   match_container = match_container->next, cbindex++)
	{
	  cb = (ContainerCallback *)(match_container->data);
	  cbarray[cbindex].callback = cb->callback;
	  cbarray[cbindex].user_data = cb->user_data;
	}
    
      for (; match_child && cbindex < max_callbacks;
	   match_child = match_child->next, cbindex++)
	{
	  cb = (ContainerCallback *)(match_child->data);
	  cbarray[cbindex].callback = cb->callback;
	  cbarray[cbindex].user_data = cb->user_data;
	}

      for (; wild_list && cbindex < max_callbacks;
	   wild_list = wild_list->next, cbindex++)
	{
	  cb = (ContainerCallback *)(wild_list->data);
	  cbarray[cbindex].callback = cb->callback;
	  cbarray[cbindex].user_data = cb->user_data;
	}
    }

  G_UNLOCK (remove_callbacks);

  /* call the callbacks */
  for (i = 0; i < cbindex; i++)
    {
      cb = &cbarray[i];
      cb->callback (container, child, cb->user_data);
    }
}

/**
 * ipatch_container_add_connect: (skip)
 * @container: (nullable): Container to match (%NULL for wildcard)
 * @callback: Callback function to call on match
 * @disconnect: (nullable): Function to call when callback is disconnected or %NULL
 * @user_data: User defined data pointer to pass to @callback and @disconnect
 *
 * Adds a callback which gets called when a container item add operation occurs
 * and the container matches @container.  When @container is %NULL, @callback
 * will be called for every container add operation.
 *
 * Returns: Handler ID which can be used to disconnect the callback or
 *   0 on error (only occurs on invalid function parameters).
 */
guint
ipatch_container_add_connect (IpatchContainer *container,
			      IpatchContainerCallback callback,
			      IpatchContainerDisconnect disconnect,
			      gpointer user_data)
{
  return (ipatch_container_real_add_connect (container, callback, disconnect, NULL, user_data));
}

/**
 * ipatch_container_add_connect_notify: (rename-to ipatch_container_add_connect)
 * @container: (nullable): Container to match (%NULL for wildcard)
 * @callback: (scope notified) (closure user_data): Callback function to call on match
 * @notify_func: (scope async) (closure user_data) (nullable): Callback destroy notify
 *   when callback is disconnected or %NULL
 * @user_data: (nullable): User defined data pointer to pass to @callback and @disconnect
 *
 * Adds a callback which gets called when a container item add operation occurs
 * and the container matches @container.  When @container is %NULL, @callback
 * will be called for every container add operation.
 *
 * Returns: Handler ID which can be used to disconnect the callback or
 *   0 on error (only occurs on invalid function parameters).
 *
 * Since: 1.1.0
 */
guint
ipatch_container_add_connect_notify (IpatchContainer *container,
			             IpatchContainerCallback callback,
			             GDestroyNotify notify_func,
			             gpointer user_data)
{
  return (ipatch_container_real_add_connect (container, callback, NULL, notify_func, user_data));
}

static guint
ipatch_container_real_add_connect (IpatchContainer *container,
                                   IpatchContainerCallback callback,
                                   IpatchContainerDisconnect disconnect,
                                   GDestroyNotify notify_func,
                                   gpointer user_data)
{
  ContainerCallback *cb;
  GSList *cblist;
  guint handler_id;

  g_return_val_if_fail (!container || IPATCH_IS_CONTAINER (container), 0);
  g_return_val_if_fail (callback != NULL, 0);

  cb = g_slice_new (ContainerCallback);
  cb->callback = callback;
  cb->disconnect = disconnect;
  cb->user_data = user_data;

  G_LOCK (add_callbacks);

  handler_id = add_callback_next_id++;
  cb->handler_id = handler_id;

  if (container)
    {
      /* get existing list for Container (if any) */
      cblist = g_hash_table_lookup (add_callback_hash, container);

      /* update the list in the hash table */
      g_hash_table_insert (add_callback_hash, container,
			   g_slist_prepend (cblist, cb));
    }
  else	/* callback is wildcard, just add to the wildcard list */
    add_wild_callback_list = g_slist_prepend (add_wild_callback_list, cb);

  G_UNLOCK (add_callbacks);

  return (handler_id);
}

/**
 * ipatch_container_remove_connect: (skip)
 * @container: (nullable): Container to match (%NULL for wildcard)
 * @child: (nullable): Child item to match (%NULL for wildcard)
 * @callback: Callback function to call on match
 * @disconnect: (nullable): Function to call when callback is disconnected or %NULL
 * @user_data: (closure): User defined data pointer to pass to @callback and @disconnect
 *
 * Adds a callback which gets called when a container item remove operation
 * occurs and the container matches @container and child item matches @child.
 * The @container and/or @child parameters can be %NULL in which case they are
 * wildcard.  If both are %NULL then @callback will be called for every
 * container remove operation.  Note that specifying only @child or both
 * @container and @child is the same, since a child belongs to only one container.
 *
 * Returns: Handler ID which can be used to disconnect the callback or
 *   0 on error (only occurs on invalid function parameters).
 */
guint
ipatch_container_remove_connect (IpatchContainer *container,
				 IpatchItem *child,
				 IpatchContainerCallback callback,
				 IpatchContainerDisconnect disconnect,
				 gpointer user_data)
{
  return (ipatch_container_real_remove_connect (container, child, callback,
                                                disconnect, NULL, user_data));
}

/**
 * ipatch_container_remove_connect_notify: (rename-to ipatch_container_remove_connect)
 * @container: (nullable): Container to match (%NULL for wildcard)
 * @child: (nullable): Child item to match (%NULL for wildcard)
 * @callback: (scope notified) (closure user_data): Callback function to call on match
 * @notify_func: (scope async) (closure user_data) (nullable): Function to call
 *   when callback is disconnected or %NULL
 * @user_data: (nullable): User defined data pointer to pass to @callback and @disconnect
 *
 * Adds a callback which gets called when a container item remove operation
 * occurs and the container matches @container and child item matches @child.
 * The @container and/or @child parameters can be %NULL in which case they are
 * wildcard.  If both are %NULL then @callback will be called for every
 * container remove operation.  Note that specifying only @child or both
 * @container and @child is the same, since a child belongs to only one container.
 *
 * Returns: Handler ID which can be used to disconnect the callback or
 *   0 on error (only occurs on invalid function parameters).
 *
 * Since: 1.1.0
 */
guint
ipatch_container_remove_connect_notify (IpatchContainer *container,
                                        IpatchItem *child,
                                        IpatchContainerCallback callback,
                                        GDestroyNotify notify_func,
                                        gpointer user_data)
{
  return (ipatch_container_real_remove_connect (container, child, callback,
                                                NULL, notify_func, user_data));
}

static guint
ipatch_container_real_remove_connect (IpatchContainer *container,
                                 IpatchItem *child,
                                 IpatchContainerCallback callback,
                                 IpatchContainerDisconnect disconnect,
                                 GDestroyNotify notify_func,
                                 gpointer user_data)
{
  ContainerCallback *cb;
  GSList *cblist;
  guint handler_id;

  g_return_val_if_fail (!container || IPATCH_IS_CONTAINER (container), 0);
  g_return_val_if_fail (!child || IPATCH_IS_ITEM (child), 0);
  g_return_val_if_fail (callback != NULL, 0);

  cb = g_slice_new (ContainerCallback);
  cb->callback = callback;
  cb->disconnect = disconnect;
  cb->notify_func = notify_func;
  cb->user_data = user_data;

  G_LOCK (remove_callbacks);

  handler_id = remove_callback_next_id++;
  cb->handler_id = handler_id;

  if (child)	/* child and container:child are equivalent (child has only 1 parent) */
    { /* get existing list for child (if any) */
      cblist = g_hash_table_lookup (remove_child_callback_hash, child);

      /* update new list head */
      g_hash_table_insert (remove_child_callback_hash, child,
			   g_slist_prepend (cblist, cb));
    }
  else if (container)
    { /* get existing list for container (if any) */
      cblist = g_hash_table_lookup (remove_container_callback_hash, container);

      /* update new list head */
      g_hash_table_insert (remove_container_callback_hash, container,
			   g_slist_prepend (cblist, cb));
    }
  else	/* callback is completely wildcard, just add to the wildcard list */
    remove_wild_callback_list = g_slist_prepend (remove_wild_callback_list, cb);

  G_UNLOCK (remove_callbacks);

  return (handler_id);
}

/**
 * ipatch_container_add_disconnect:
 * @handler_id: ID of callback handler
 *
 * Disconnects a container add callback previously connected with
 * ipatch_container_add_connect() by handler ID.  The
 * ipatch_container_add_disconnect_matched() function can be used instead to
 * disconnect by original callback criteria and is actually faster.
 */
void
ipatch_container_add_disconnect (guint handler_id)
{
  g_return_if_fail (handler_id != 0);
  ipatch_container_real_disconnect (handler_id, NULL, NULL, NULL, NULL, TRUE);
}

/**
 * ipatch_container_add_disconnect_matched: (skip)
 * @container: Container to match
 * @callback: Callback function to match
 * @user_data: User data to match
 *
 * Disconnects a container add callback previously connected with
 * ipatch_container_add_connect() by match criteria.
 */
void
ipatch_container_add_disconnect_matched (IpatchContainer *container,
					 IpatchContainerCallback callback,
					 gpointer user_data)
{
  g_return_if_fail (callback != NULL);
  ipatch_container_real_disconnect (0, container, NULL, callback, user_data, TRUE);
}

/**
 * ipatch_container_remove_disconnect:
 * @handler_id: ID of callback handler
 *
 * Disconnects a container remove callback previously connected with
 * ipatch_container_remove_connect() by handler ID.  The
 * ipatch_container_remove_disconnect_matched() function can be used instead to
 * disconnect by original callback criteria and is actually faster.
 */
void
ipatch_container_remove_disconnect (guint handler_id)
{
  g_return_if_fail (handler_id != 0);
  ipatch_container_real_disconnect (handler_id, NULL, NULL, NULL, NULL, FALSE);
}

/**
 * ipatch_container_remove_disconnect_matched: (skip)
 * @container: (nullable): Container to match (can be %NULL if @child is set)
 * @child: (nullable): Child item to match (can be %NULL if @container is set)
 * @callback: Callback function to match
 * @user_data: User data to match
 *
 * Disconnects a handler previously connected with
 * ipatch_container_remove_connect() by match criteria.
 */
void
ipatch_container_remove_disconnect_matched (IpatchContainer *container,
					    IpatchItem *child,
					    IpatchContainerCallback callback,
					    gpointer user_data)
{
  g_return_if_fail (callback != NULL);
  ipatch_container_real_disconnect (0, container, child, callback, user_data, FALSE);
}

/* a bag used in ipatch_container_real_disconnect */
typedef struct
{
  gboolean found;		/* Set to TRUE if handler found */
  IpatchItem *match;		/* container or child -  in: (match only), out */
  ContainerCallback cb;		/* in: handler_id, out: disconnect, user_data */
  gpointer update_key; 		/* out: key of list root requiring update */
  GSList *update_list;		/* out: new root of list to update */
  gboolean update_needed;	/* out: set when a list root needs updating */
  gboolean isadd;		/* same value as function parameter */
} DisconnectBag;

/* function for removing a callback using match criteria (hash table lookup).
 * Faster than iterating over the hash (required when searching by ID). */
static void
disconnect_matched (GHashTable *hash, DisconnectBag *bag)
{
  ContainerCallback *cb;
  GSList *list, *newroot, *p;

  list = g_hash_table_lookup (hash, bag->match);
  if (!list) return;

  for (p = list; p; p = p->next)	/* loop over callbacks in list */
    {
      cb = (ContainerCallback *)(p->data);

      /* matches criteria? */
      if (cb->callback == bag->cb.callback && cb->user_data == bag->cb.user_data)
	{ /* return callback's disconnect func */
	  bag->found = TRUE;
	  bag->cb.disconnect = cb->disconnect;

	  g_slice_free (ContainerCallback, cb);
	  newroot = g_slist_delete_link (list, p);

	  if (!newroot)	/* no more list? - remove hash entry */
	    g_hash_table_remove (hash, bag->match);
	  else if (newroot != list)	/* root of list changed? - update hash entry */
	    g_hash_table_insert (hash, bag->match, newroot);

	  break;
	}
    }
}

/* Used by disconnect functions.
 * Either handler_id should be set to a non-zero value or the other
 * parameters should be assigned but not both.
 * isadd specifies if the handler is an add callback (TRUE) or remove
 * callback (FALSE).
 */
static void
ipatch_container_real_disconnect (guint handler_id, IpatchContainer *container,
				  IpatchItem *child,
				  IpatchContainerCallback callback,
				  gpointer user_data, gboolean isadd)
{
  ContainerCallback *cb;
  DisconnectBag bag = { 0 };
  gboolean isfoundchild = FALSE;
  GSList *p;

  g_return_if_fail (handler_id != 0 || callback != 0);
  g_return_if_fail (handler_id == 0 || callback == 0);

  if (!handler_id)	/* find by match criteria? */
    {
      bag.match = child ? child : (IpatchItem *)container;
      bag.cb.callback = callback;
      bag.cb.user_data = user_data;
    }
  else bag.cb.handler_id = handler_id; /* find by handler ID */

  bag.isadd = isadd;

  if (isadd)	/* add callback search? */
    {
      G_LOCK (add_callbacks);

      if (handler_id)	/* search by ID? */
	{	/* scan every list in add callback hash and remove if found */
	  g_hash_table_foreach_remove (add_callback_hash, callback_hash_GHRFunc,
				       &bag);
	  if (bag.update_needed)  /* update root of list if needed (can't do that in GHRFunc) */
	    g_hash_table_insert (add_callback_hash, bag.update_key, bag.update_list);
	}
      else disconnect_matched (add_callback_hash, &bag);  /* lookup by match and remove if found */

      /* if not found, check wildcard list (if search by handler ID
       * or NULL container) */
      if (!bag.found && (handler_id || !container))
	{
	  for (p = add_wild_callback_list; p; p = p->next)
	    {
	      cb = (ContainerCallback *)(p->data);

	      if ((handler_id && cb->handler_id == handler_id)
		  || (!handler_id && cb->callback == callback
		      && cb->user_data == user_data))
		{
		  bag.found = TRUE;
		  bag.cb.disconnect = cb->disconnect;
		  bag.cb.user_data = cb->user_data;
		  g_slice_free (ContainerCallback, cb);

		  add_wild_callback_list
		    = g_slist_delete_link (add_wild_callback_list, p);
		  break;
		}
	    }
	}
      G_UNLOCK (add_callbacks);
    }
  else		/* remove callback search */
    {
      G_LOCK (remove_callbacks);

      /* check child remove callback list if search by ID or child is set */
      if (handler_id)	/* search by ID? */
	{
	  g_hash_table_foreach_remove (remove_child_callback_hash,
				       callback_hash_GHRFunc, &bag);
	  if (bag.update_needed)  /* update root of list if needed (can't do that in GHRFunc) */
	    g_hash_table_insert (remove_child_callback_hash, bag.update_key,
				 bag.update_list);
	}
      else if (child)	/* match by child? */
	disconnect_matched (remove_child_callback_hash, &bag);

      if (bag.found) isfoundchild = TRUE;	/* indicate it is a child callback */

      if (!bag.found)	/* not yet found? */
	{
	  /* check container remove callback list if search by ID or container is set */
	  if (handler_id)	/* search by ID? */
	    {
	      g_hash_table_foreach_remove (remove_container_callback_hash,
					   callback_hash_GHRFunc, &bag);
	      if (bag.update_needed)  /* update root of list if needed (can't do that in GHRFunc) */
		g_hash_table_insert (remove_container_callback_hash, bag.update_key,
				     bag.update_list);
	    }
	  else if (container)	/* match by container? */
	    disconnect_matched (remove_container_callback_hash, &bag);
	}

      /* if not yet found, check wildcard list (if search by handler ID
       * or NULL container and child) */
      if (!bag.found && (handler_id || (!container && !child)))
	{
	  for (p = remove_wild_callback_list; p; p = p->next)
	    {
	      cb = (ContainerCallback *)(p->data);

	      if ((handler_id && cb->handler_id == handler_id)
		  || (!handler_id && cb->callback == callback
		      && cb->user_data == user_data))
		{
		  bag.found = TRUE;
		  bag.cb.disconnect = cb->disconnect;
		  bag.cb.user_data = cb->user_data;
		  g_slice_free (ContainerCallback, cb);

		  remove_wild_callback_list
		    = g_slist_delete_link (remove_wild_callback_list, p);
		  break;
		}
	    }
	}
      G_UNLOCK (remove_callbacks);
    }

  if (!bag.found)
    {
      if (handler_id)
	g_critical (G_STRLOC ": Failed to find %s container handler with ID '%d'",
		    isadd ? "add" : "remove", handler_id);
      else
	g_critical (G_STRLOC ": Failed to find %s container handler with criteria %p:%p:%p:%p",
		    isadd ? "add" : "remove", container, child, callback, user_data);
    }

  /* see if callback found and it had a disconnect func */
  if (bag.cb.disconnect)
    {
      if (isfoundchild)
	bag.cb.disconnect (NULL, bag.match, bag.cb.user_data);
      else bag.cb.disconnect ((IpatchContainer *)bag.match, NULL, bag.cb.user_data);
    }
}

/* finds a container add or remove handler by ID and removes it */
static gboolean
callback_hash_GHRFunc (gpointer key, gpointer value, gpointer user_data)
{
  DisconnectBag *bag = (DisconnectBag *)user_data;
  GSList *cblist = (GSList *)value, *p, *newroot;
  ContainerCallback *cb;

  p = cblist;
  while (p)		    /* loop over callbacks in callback list */
    {
      cb = (ContainerCallback *)(p->data);

      /* matches criteria? (by ID or by match) */
      if ((bag->cb.handler_id && cb->handler_id == bag->cb.handler_id)
	  || (!bag->cb.handler_id && key == bag->match
	      && cb->callback == bag->cb.callback
	      && cb->user_data == bag->cb.user_data))
	{
	  /* return callback's item, pspec, disconnect func and user_data */
	  bag->found = TRUE;
	  bag->cb.disconnect = cb->disconnect;
	  bag->cb.user_data = cb->user_data;
	  bag->match = key;

	  g_slice_free (ContainerCallback, cb);
	  newroot = g_slist_delete_link (cblist, p);

	  if (!newroot) return (TRUE);  /* no more list? Remove hash entry */

	  /* if root not the same, return update information
	     (can't be done in GHRFunc) */
	  if (newroot != cblist)
	    {
	      bag->update_key = key;
	      bag->update_list = newroot;
	    }

	  return (FALSE);	/* don't remove entry (callback list not empty) */
	}
      p = g_slist_next (p);
    }

  return (FALSE);		/* don't remove entry (item not found) */
}
