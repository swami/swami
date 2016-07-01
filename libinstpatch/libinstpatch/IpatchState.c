/*
 * IpatchState.c - Item state (undo/redo) system
 *
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
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
#include <stdio.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchState.h"
#include "ipatch_priv.h"

#define ERRMSG_NO_ACTIVE_STATE_GROUP "No active state group!"

static void ipatch_state_class_init (IpatchStateClass *klass);
static void ipatch_state_init (IpatchState *state);
static void ipatch_state_finalize (GObject *gobject);
static gboolean tree_traverse_object_unref (GNode *node, gpointer data);

static gboolean traverse_set_depends (GNode *node, gpointer data);
static gboolean traverse_is_dependent (GNode *node, gpointer data);
static gboolean traverse_unset_depends (GNode *node, gpointer data);
static void traverse_undo (IpatchStateItem *item, IpatchStateItem *last_depend,
			   IpatchStateItem *last_nondep,
			   IpatchState *state);

gpointer state_parent_class = NULL;


GType
ipatch_state_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateClass), NULL, NULL,
      (GClassInitFunc) ipatch_state_class_init, NULL, NULL,
      sizeof (IpatchState),
      0,
      (GInstanceInitFunc) ipatch_state_init,
    };

    item_type = g_type_register_static (IPATCH_TYPE_LOCK, "IpatchState",
					&item_info, 0);
  }

  return (item_type);
}

static void
ipatch_state_class_init (IpatchStateClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  state_parent_class = g_type_class_peek_parent (klass);
  gobj_class->finalize = ipatch_state_finalize;
}

static void
ipatch_state_init (IpatchState *state)
{
  state->root = g_node_new (NULL); /* create root state NULL tree node */
  state->position = state->root;
  state->group_root = g_node_new (NULL); /* create root group NULL node */

  state->active_group_key = g_private_new ((GDestroyNotify)g_object_unref);
}

static void
ipatch_state_finalize (GObject *gobject)
{
  IpatchState *state = IPATCH_STATE (gobject);

  IPATCH_LOCK_WRITE (state);

  /* unref all IpatchStates in tree */
  g_node_traverse (state->root, G_IN_ORDER, G_TRAVERSE_ALL, -1,
		   tree_traverse_object_unref, NULL);
  state->root = NULL;
  state->position = NULL;

  /* unref all IpatchStateGroups in group tree */
  g_node_traverse (state->group_root, G_IN_ORDER, G_TRAVERSE_ALL, -1,
		   tree_traverse_object_unref, NULL);
  state->group_root = NULL;

  IPATCH_UNLOCK_WRITE (state);

  if (G_OBJECT_CLASS (state_parent_class)->finalize)
    G_OBJECT_CLASS (state_parent_class)->finalize (gobject);
}

/* unref all GObjects in a tree before murdering it */
static gboolean
tree_traverse_object_unref (GNode *node, gpointer data)
{
  GObject *obj = (GObject *)(node->data);
  g_node_unlink (node);		/* unlink the node from the tree */
  if (obj) g_object_unref (obj); /* -- unref object (object destroys node) */
  return (FALSE);
}

/**
 * ipatch_state_new:
 *
 * Create a new state history object. Returned object has a ref count of 1
 * which the caller owns.
 *
 * Returns: The new state history object.
 */
IpatchState *
ipatch_state_new (void)
{
  return (g_object_new (IPATCH_TYPE_STATE, NULL));
}

/**
 * ipatch_state_begin_group:
 * @state: State history object
 * @descr: Description of group action
 *
 * Start a state group. State groups are used to group multiple actions.
 * Each thread has its own active group, if this function is called without
 * ending a previous group, a nested group is started. There is no limit to
 * the depth of group nesting.
 */
void
ipatch_state_begin_group (IpatchState *state, const char *descr)
{
  IpatchStateGroup *group, *active;
  GNode *parent, *n;

  g_return_if_fail (IPATCH_IS_STATE (state));

  group = g_object_new (IPATCH_TYPE_STATE_GROUP, NULL); /* ++ ref group */
  if (descr) group->descr = g_strdup (descr);

  /* get calling thread's active group */
  active = g_private_get (state->active_group_key);
  if (active) parent = active->node; /* nest new group */
  else parent = state->group_root; /* top level group */

  /* group tree takes control of group creator's ref */
  IPATCH_LOCK_WRITE (state);
  n = g_node_prepend_data (parent, group);
  group->node = n;
  IPATCH_UNLOCK_WRITE (state);

  g_object_ref (group);	/* ++ ref group for active group private var */
  g_private_set (state->active_group_key, group);
}

/**
 * ipatch_state_end_group:
 * @state: State history object
 *
 * End the current active state group.  Causes the next nested group parent
 * to become active or the active group to be deactivated if there are no
 * more nested group parents.
 */
void
ipatch_state_end_group (IpatchState *state)
{
  IpatchStateGroup *active;
  GNode *node;

  g_return_if_fail (IPATCH_IS_STATE (state));

  active = g_private_get (state->active_group_key);
  if (!active)
    {
      g_critical (ERRMSG_NO_ACTIVE_STATE_GROUP);
      return;
    }

  node = active->node->parent;	/* move up group tree */
  if (node && !node->data) node = NULL; /* root dummy node? */

  if (node) g_object_ref (node->data); /* ++ ref group obj for private var */
  g_private_set (state->active_group_key, node->data);
}

/**
 * ipatch_state_set_active_group:
 * @state: State history object
 * @group: (nullable): State group or %NULL to deactivate grouping
 *
 * Sets the active state group for the calling thread in a state history
 * object or deactivates grouping if @group is NULL.
 */
void
ipatch_state_set_active_group (IpatchState *state,
			      IpatchStateGroup *group)
{
  g_return_if_fail (IPATCH_IS_STATE (state));
  g_return_if_fail (!group || IPATCH_IS_STATE_GROUP (group));

  if (group) g_object_ref (group); /* ++ ref group object for private var */
  g_private_set (state->active_group_key, group);
}

/**
 * ipatch_state_get_active_group:
 * @state: State history object
 *
 * Get the current active state group for the calling thread. The ref count of
 * the returned state group is incremented and the caller is responsible for
 * removing it with g_object_unref when finished.
 *
 * Returns: Ipatch state group object or %NULL if no groups active. Remember to
 *   unref it when finished using it.
 */
IpatchStateGroup *
ipatch_state_get_active_group (IpatchState *state)
{
  IpatchStateGroup *active;

  g_return_val_if_fail (IPATCH_IS_STATE (state), NULL);

  active = g_private_get (state->active_group_key);
  if (active) g_object_ref (active);
  return (active);
}

/**
 * ipatch_state_record_item:
 * @state: State history object
 * @item: State item to add
 *
 * Add a state item to a state history object and advance the current position.
 */
void
ipatch_state_record_item (IpatchState *state, IpatchStateItem *item)
{
  IpatchStateGroup *active_group;
  GNode *n;

  g_return_if_fail (IPATCH_IS_STATE (state));
  g_return_if_fail (item->node == NULL);
  g_return_if_fail (item->group == NULL);

  active_group = g_private_get (state->active_group_key);

  IPATCH_LOCK_WRITE (state);

  if (state->current_undo) /* called from an active undo? */
    { /* recording redo state, use group of active undo item */
      item->flags = IPATCH_STATE_ITEM_REDO | IPATCH_STATE_ITEM_ACTIVE;
      item->group = state->current_undo->group;
      n = g_node_prepend_data (state->redo_parent, item);
      state->redo_parent = n;	/* in case there are multiple redo actions */
    }
  else				/* normal undo state recording */
    {
      item->flags = IPATCH_STATE_ITEM_UNDO | IPATCH_STATE_ITEM_ACTIVE;
      item->group = active_group;
      n = g_node_prepend_data (state->position, item); /* add item to tree */
      state->position = n;	/* advance current position */
    }

  item->node = n;

  IPATCH_UNLOCK_WRITE (state);

  g_object_ref (item);		/* ++ ref item for item tree */

  if (item->group)     /* add the item to its group item list */
    {
      g_object_ref (item->group); /* ++ ref group for item */

      IPATCH_LOCK_WRITE (item->group);
      item->group->items = g_list_prepend (item->group->items, item);
      IPATCH_UNLOCK_WRITE (item->group);
    }
}

/**
 * ipatch_state_record:
 * @state: State history object
 * @type_name: Name of #IpatchStateItem derived type to record
 * @first_property_name: (nullable): Name of first item property to set or NULL to not
 *   set any properties.
 * @Varargs: Name/value pairs of properties to set on new #IpatchStateItem item.
 *   Variable argument list starts with the value for @first_property_name and
 *   is terminated with a NULL name parameter.
 *
 * Records an action in a state history object. A state item of type
 * `@type_name' is created and property values are set to the passed
 * in values. The state item is then added to the state history object
 * and the current position is advanced.
 */
void
ipatch_state_record (IpatchState *state, const char *type_name,
		    const char *first_property_name, ...)
{
  IpatchStateItem *item;
  va_list args;
  GType type;

  g_return_if_fail (IPATCH_IS_STATE (state));
  type = g_type_from_name (type_name);
  g_return_if_fail (g_type_is_a (type, IPATCH_TYPE_STATE));

  va_start (args, first_property_name);
  item = (IpatchStateItem *)	/* ++ ref new item */
    (g_object_new_valist (type, first_property_name, args));
  va_end (args);

  ipatch_state_record_item (state, item);
  g_object_unref (item);	/* -- unref creator's ref */
}

/**
 * ipatch_state_retract:
 * @state: State history object
 *
 * Undos all actions in the current active sub group, flags the group as
 * retracted, and closes it.
 */
void
ipatch_state_retract (IpatchState *state)
{
  IpatchStateGroup *group;

  g_return_if_fail (IPATCH_IS_STATE (state));

  group = g_private_get (state->active_group_key);
  if (!group)
    {
      g_critical (ERRMSG_NO_ACTIVE_STATE_GROUP);
      return;
    }

  IPATCH_LOCK_WRITE (group);

  ipatch_state_undo (state, group->items);
  group->flags |= IPATCH_STATE_GROUP_RETRACTED;

  IPATCH_UNLOCK_WRITE (group);
}

/**
 * ipatch_state_get_undo_depends:
 * @state: State history object
 * @items: List of #IpatchStateItem items to check undo dependencies of
 *
 * Get dependencies for a list of @items to undo.
 *
 * Returns: A list of #IpatchStateItem items that depend on @items. Only additional
 *   dependencies are returned (not already in @items). %NULL is returned if
 *   there are no more dependencies. List should be freed with
 *   ipatch_state_list_free() when finished with it.
 */
#if 0	/* FIXME - Not finished */
GList *
ipatch_state_get_undo_depends (IpatchState *state, const GList *items)
{
  g_return_val_if_fail (IPATCH_IS_STATE (state), NULL);
  g_return_val_if_fail (items != NULL, NULL);

  
}
#endif

/**
 * ipatch_state_undo:
 * @state: State history object
 * @items: (element-type IpatchStateItem): List of #IpatchStateItem items to undo
 *
 * Undo one or more items in a state history object. All active dependent
 * items are undone as well. All inactive affected items are migrated
 * appropriately.
 */
void
ipatch_state_undo (IpatchState *state, GList *items)
{
  IpatchStateItem *top_item;
  GList *dup_list, *p;
  GNode *n;

  g_return_if_fail (IPATCH_IS_STATE (state));
  g_return_if_fail (items != NULL);

  /* optimized for small items list compared to history tree, we make a copy
     of the items list so we can remove them as they are found in the tree
     so we know when to stop */
  dup_list = g_list_copy (items);

  IPATCH_LOCK_WRITE (state);

  /* locate the item closest to the root of the item tree */
  n = state->position;
  top_item = NULL;
  while (n->data)
    {
      p = dup_list;
      while (p)	      /* see if this tree node matches one in 'items' */
	{
	  if (p->data == n->data)
	    {
	      top_item = p->data;

	      /* set dependent flag */
	      top_item->flags |= IPATCH_STATE_ITEM_DEPENDENT;

	      /* no more items in duplicate list? (all of them found) */
	      if (!(dup_list = g_list_delete_link (dup_list, p)))
		goto outta_here;

	      break;		/* next tree parent node */
	    }
	  p = g_list_next (p);
	}
      n = n->parent;
    }

  g_list_free (dup_list);	/* in case some items weren't found */

 outta_here:

  if (!top_item)		/* no items to undo?! */
    {
      IPATCH_UNLOCK_WRITE (state);
      return;
    }

  /* recursively flag all items dependent on undo 'items' */
  g_node_traverse (top_item->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
		   (GNodeTraverseFunc)traverse_set_depends, NULL);

  /* if we are undoing the current position, move position to first parent
     that isn't being un-done */
  n = state->position;
  while (n)
    {
      if (!n->data || !(((IpatchStateItem *)(n->data))->flags
			& IPATCH_STATE_ITEM_DEPENDENT))
	{
	  state->position = n;
	  break;
	}
      n = n->parent;
    }

  /* undo the dependent items, re-structure tree, etc */
  traverse_undo (top_item,
		 IPATCH_STATE_ITEM (top_item->node->parent->data),
		 IPATCH_STATE_ITEM (top_item->node->parent->data),
		 state);

  /* clear the dependent flag, unfortunately couldn't figure out a way to do
     this in traverse_undo */
  g_node_traverse (top_item->node, G_IN_ORDER, G_TRAVERSE_ALL, -1,
		   (GNodeTraverseFunc)traverse_unset_depends, NULL);

  IPATCH_UNLOCK_WRITE (state);

  return;
}

/* function that traverses over an item tree and marks items that are dependent
   on already dependent marked items */
static gboolean
traverse_set_depends (GNode *node, gpointer data)
{
  IpatchStateItem *item = (IpatchStateItem *)(node->data);

  if (item->flags & IPATCH_STATE_ITEM_DEPENDENT)
    g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
		     (GNodeTraverseFunc)traverse_is_dependent, item);

  return (FALSE);
}

/* traversal function to mark dependencies to an item passed as the 'data'
   parameter */
static gboolean
traverse_is_dependent (GNode *node, gpointer data)
{
  IpatchStateItem *item = (IpatchStateItem *)(node->data);
  IpatchStateItem *dep = (IpatchStateItem *)data;

  /* item is not already marked? (Has the added effect of stopping dependency
     checking of an item with itself). */
  if (!(item->flags & IPATCH_STATE_ITEM_DEPENDENT)
      && ipatch_state_item_depend (item, dep)) /* item is dependent? */
    item->flags |= IPATCH_STATE_ITEM_DEPENDENT;

  return (FALSE);
}

/* traversal function to unset dependent flag of all items */
static gboolean
traverse_unset_depends (GNode *node, gpointer data)
{
  IpatchStateItem *item = (IpatchStateItem *)(node->data);
  item->flags &= ~IPATCH_STATE_ITEM_DEPENDENT;

  return (FALSE);
}

/* traverses an item tree searching for item's flagged as dependent, these
   items are moved to their closest dependent parent and undo items are
   undone */
static void
traverse_undo (IpatchStateItem *item, IpatchStateItem *last_depend,
	       IpatchStateItem *last_nondep, IpatchState *state)
{
  GNode *n;	 /* keep local vars to minimum since recursive func */

  n = item->node->children;
  while (n)
    {
      register GNode *temp = n;	/* register, so no stack is used */
      n = n->next; /* must advance before call, since n might get removed */

      traverse_undo ((IpatchStateItem *)(temp->data),
		     (item->flags & IPATCH_STATE_ITEM_DEPENDENT)
		     ? item : last_depend,
		     (item->flags & IPATCH_STATE_ITEM_DEPENDENT)
		     ? last_nondep : item, state);
    }

  /* return if this item is not dependent */
  if (!(item->flags & IPATCH_STATE_ITEM_DEPENDENT))
    return;

  /* undo item? */
  if ((item->flags & IPATCH_STATE_ITEM_TYPE_MASK) == IPATCH_STATE_ITEM_UNDO)
    {
      /* Set restore vars to catch redo activity in ipatch_state_record_item().
	 Ref not required because we are locked for duration of use. */
      state->current_undo = item;
      state->redo_parent = last_depend->node;	/* parent of new redo state */

      ipatch_state_item_restore (item); /* undo the action */

      state->current_undo = NULL;
      /* state->redo_parent now contains last recorded redo state item */

      /* move the old item's children to appropriate parent */
      n = g_node_last_child (item->node);
      while (n)
	{
	  GNode *n2 = n;
	  n = n->prev;
	  g_node_unlink (n2);

	  if (((IpatchStateItem *)(n->data))->flags & IPATCH_STATE_ITEM_DEPENDENT)
	    g_node_prepend (state->redo_parent, n2);
	  else g_node_prepend (last_nondep->node, n2);
	}

      g_node_unlink (item->node); /* unlink old item node */
      g_object_unref (item); /* -- unref the item (no longer in tree) */
    }
  else				/* dependent redo item */
    {				/* move to last non-dependent node */
      if (last_nondep->node != item->node->parent)
	{
	  g_node_unlink (item->node);
	  g_node_prepend (last_nondep->node, item->node);
	}
    }
}

#if 0
/**
 * ipatch_state_redo:
 * @state: Ipatch state history object
 * @items: List of #IpatchStateItem items to redo
 *
 * Redo one or more items in a state history object. All dependent items are
 * also re-done.
 */
void
ipatch_state_redo (IpatchState *state, GList *items)
{
  IpatchStateItem *top_item;
  IpatchStateItemClass *item_class;
  GList *dup_list = NULL, *p;
  GNode *n;

  g_return_val_if_fail (IPATCH_IS_STATE (state), FALSE);
  g_return_val_if_fail (IPATCH_IS_STATE_ITEM (item), FALSE);
  g_return_val_if_fail (item->node != NULL, FALSE);

  item_class = IPATCH_STATE_ITEM_GET_CLASS (item);
  g_return_val_if_fail (item_class->restore != NULL, FALSE);
  g_return_val_if_fail (item_class->reverse != NULL, FALSE);

  IPATCH_LOCK_WRITE (state);

  dup_list = g_list_copy (items);

  /* flag all redo items, optimized for large state history, we climb up the
     undo state branch traversing its redo state children */
  n = state->position;
  while (n->data)
    {
      traverse_redo_flag_items (n, &dup_list);
      if (!dup_list) break;
      n = n->parent;
    }
  g_list_free (dup_list); /* free remaining un-located items (if any) */

  top_item = n;		/* top most item encompassing all redo items */



  IPATCH_UNLOCK_WRITE (state);
}

static void
traverse_redo_flag_items (GNode *node, GList **items)
{
  register IpatchStateItem *item = (IpatchStateItem *)(node->data);
  GNode *n;

  /* if node is a redo item and not yet flagged, see if its in items */
  if (item->flags & IPATCH_STATE_ITEM_REDO &&
      !(item->flags & IPATCH_STATE_ITEM_DEPENDENT))
    {
      p = *items;
      while (p)
	{
	  if (p->data == (void *)item)
	    {
	      *items = g_list_delete_link (*items, p); /* remove from items */
	      item->flags |= IPATCH_STATE_ITEM_DEPENDENT; /* flag it */
	      break;
	    }
	  p = g_list_next (p);
	}
    }

  if (!*items) return;

  n = node->children;
  while (n)
    {
      if (((IpatchStateItem *)(n->data))->flags & IPATCH_STATE_ITEM_REDO)
	traverse_redo_flag_items (n, items);
      n = n->next;
    }
}
#endif
