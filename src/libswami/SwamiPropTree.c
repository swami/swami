/*
 * SwamiPropTree.c - Swami property tree object
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
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "SwamiPropTree.h"
#include "SwamiControlProp.h"
#include "SwamiLog.h"
#include "swami_priv.h"


/* a cached active property value */
typedef struct
{
    GParamSpec *pspec;	 /* parameter spec for this cached property */
    SwamiControl *prop_ctrl; /* object property control for this cache */
    SwamiPropTreeValue *value; /* tree value connected to prop_ctrl */
    SwamiPropTreeNode *value_node; /* node containing @value */
} CacheValue;


/* unlocked chunk alloc/free macros (requires external locking) */
#define swami_prop_tree_new_node_L() \
  g_slice_new (SwamiPropTreeNode)
#define swami_prop_tree_free_node_L(node) \
  g_slice_free (SwamiPropTreeNode, node)

#define swami_prop_tree_new_value_L() \
  g_slice_new (SwamiPropTreeValue)
#define swami_prop_tree_free_value_L(value) \
  g_slice_free (SwamiPropTreeValue, value)

#define TREE_CACHE_PREALLOC 64
#define swami_prop_tree_new_cache_L() \
  g_slice_new (CacheValue)
#define swami_prop_tree_free_cache_L(cache) \
  g_slice_free (CacheValue, cache)

static void swami_prop_tree_class_init(SwamiPropTreeClass *klass);
static void swami_prop_tree_init(SwamiPropTree *proptree);
static void swami_prop_tree_finalize(GObject *object);

static void swami_prop_tree_object_weak_notify(gpointer user_data,
        GObject *object);
static inline void swami_prop_tree_node_reset_L(SwamiPropTree *proptree,
        SwamiPropTreeNode *treenode);
static inline void swami_prop_tree_node_clear_cache_L
(SwamiPropTreeNode *treenode);

static void recursive_remove_nodes(GNode *node, SwamiPropTree *proptree);

static void resolve_object_props_L(SwamiPropTree *proptree,
                                   GNode *object_node, GList *speclist);
static GList *object_spec_list(GObject *object);
static void refresh_value_nodes_L(GNode *node, SwamiPropTreeValue *treeval);
static void refresh_value_nodes_list_L(GNode *node, GSList *treevals);
static inline void refresh_cache_value_L(GNode *node, CacheValue *cache);

static GObjectClass *parent_class = NULL;


GType
swami_prop_tree_get_type(void)
{
    static GType otype = 0;

    if(!otype)
    {
        static const GTypeInfo type_info =
        {
            sizeof(SwamiPropTreeClass), NULL, NULL,
            (GClassInitFunc) swami_prop_tree_class_init,
            (GClassFinalizeFunc) NULL, NULL,
            sizeof(SwamiPropTree), 0,
            (GInstanceInitFunc) swami_prop_tree_init
        };

        otype = g_type_register_static(SWAMI_TYPE_LOCK, "SwamiPropTree",
                                       &type_info, 0);
    }

    return (otype);
}

static void
swami_prop_tree_class_init(SwamiPropTreeClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    obj_class->finalize = swami_prop_tree_finalize;
}

static void
swami_prop_tree_init(SwamiPropTree *proptree)
{
    proptree->tree = NULL;
    proptree->object_hash = g_hash_table_new(NULL, NULL);
}

static void
swami_prop_tree_finalize(GObject *object)
{
    SwamiPropTree *proptree = SWAMI_PROP_TREE(object);

    SWAMI_LOCK_WRITE(proptree);

    if(proptree->tree)
    {
        recursive_remove_nodes(proptree->tree, proptree);    /* recursive remove */
    }

    g_hash_table_destroy(proptree->object_hash);

    SWAMI_UNLOCK_WRITE(proptree);

    if(parent_class->finalize)
    {
        parent_class->finalize(object);
    }
}

/**
 * swami_prop_tree_new:
 *
 * Create a new property tree object.
 *
 * Returns: New property tree object with a refcount of 1.
 */
SwamiPropTree *
swami_prop_tree_new(void)
{
    return (SWAMI_PROP_TREE(g_object_new(SWAMI_TYPE_PROP_TREE, NULL)));
}

/**
 * swami_prop_tree_set_root:
 * @proptree: Property tree object
 * @root: Object to make the root object of the tree
 *
 * Set the root object of a property tree. Should only be set once.
 */
void
swami_prop_tree_set_root(SwamiPropTree *proptree, GObject *root)
{
    SwamiPropTreeNode *node;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(root));

    SWAMI_LOCK_WRITE(proptree);

    if(swami_log_if_fail(proptree->tree == NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    node = swami_prop_tree_new_node_L();
    node->object = root;
    node->values = NULL;
    node->cache = NULL;
    proptree->tree = g_node_new(node);

    /* add to object => GNode hash */
    g_hash_table_insert(proptree->object_hash, root, proptree->tree);

    /* weak ref to passively catch objects demise */
    g_object_weak_ref(root, swami_prop_tree_object_weak_notify, proptree);

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_prepend:
 * @proptree: Property tree object
 * @parent: Object in @proptree to parent to
 * @obj: Object to prepend to @proptree
 *
 * Prepends an object to a property tree.
 */
void
swami_prop_tree_prepend(SwamiPropTree *proptree, GObject *parent,
                        GObject *obj)
{
    SwamiPropTreeNode *treenode;
    GNode *parent_node, *newnode;
    GList *speclist;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(parent));
    g_return_if_fail(G_IS_OBJECT(obj));

    speclist = object_spec_list(obj);

    SWAMI_LOCK_WRITE(proptree);

    parent_node = g_hash_table_lookup(proptree->object_hash, parent);

    if(swami_log_if_fail(parent_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    treenode = swami_prop_tree_new_node_L();
    treenode->object = obj;
    treenode->values = NULL;
    treenode->cache = NULL;
    newnode = g_node_prepend_data(parent_node, treenode);

    g_hash_table_insert(proptree->object_hash, obj, newnode);

    /* weak ref to passively catch objects demise */
    g_object_weak_ref(obj, swami_prop_tree_object_weak_notify, proptree);

    /* resolve properties if any (speclist is freed) */
    if(speclist)
    {
        resolve_object_props_L(proptree, newnode, speclist);
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_insert_before:
 * @proptree: Property tree object
 * @parent: Object in @proptree to parent to
 * @sibling: Object in @proptree to insert before or %NULL to append
 * @obj: Object to prepend to @proptree
 *
 * Inserts an object to a property tree before @sibling and parented to
 * @parent.
 */
void
swami_prop_tree_insert_before(SwamiPropTree *proptree, GObject *parent,
                              GObject *sibling, GObject *obj)
{
    SwamiPropTreeNode *treenode;
    GNode *parent_node, *sibling_node = NULL, *newnode;
    GList *speclist;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(parent));
    g_return_if_fail(!sibling || G_IS_OBJECT(sibling));
    g_return_if_fail(G_IS_OBJECT(obj));

    speclist = object_spec_list(obj);

    SWAMI_LOCK_WRITE(proptree);

    parent_node = g_hash_table_lookup(proptree->object_hash, parent);

    if(swami_log_if_fail(parent_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    if(sibling)
    {
        sibling_node = g_hash_table_lookup(proptree->object_hash, sibling);

        if(swami_log_if_fail(sibling_node != NULL))
        {
            SWAMI_UNLOCK_WRITE(proptree);
            return;
        }
    }

    treenode = swami_prop_tree_new_node_L();
    treenode->object = obj;
    treenode->values = NULL;
    treenode->cache = NULL;
    newnode = g_node_insert_data_before(parent_node, sibling_node, treenode);

    g_hash_table_insert(proptree->object_hash, obj, newnode);

    /* weak ref to passively catch objects demise */
    g_object_weak_ref(obj, swami_prop_tree_object_weak_notify, proptree);

    /* resolve properties if any (speclist is freed) */
    if(speclist)
    {
        resolve_object_props_L(proptree, newnode, speclist);
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_remove:
 * @proptree: Property tree object
 * @obj: Object in @proptree to remove
 *
 * Removes an @obj, and all values bound to it, from a property tree.
 * All child nodes are moved up to the next parent node.
 */
void
swami_prop_tree_remove(SwamiPropTree *proptree, GObject *obj)
{
    SwamiPropTreeNode *treenode;
    GNode *obj_node, *newparent, *n, *temp;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(obj));

    SWAMI_LOCK_WRITE(proptree);

    obj_node = g_hash_table_lookup(proptree->object_hash, obj);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    newparent = obj_node->parent;

    if(swami_log_if_fail(newparent != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    treenode = (SwamiPropTreeNode *)(obj_node->data);
    g_node_unlink(obj_node);	/* unlink the GNode */

    n = g_node_last_child(obj_node);

    while(n)  /* move children of removed node to parent of it and refresh */
    {
        temp = n;
        n = n->prev;
        g_node_prepend(newparent, temp);

        /* recursive refresh */
        if(treenode->values)
        {
            refresh_value_nodes_list_L(n, treenode->values);
        }
    }

    obj_node->children = NULL;
    g_node_destroy(obj_node);	/* destroy the GNode */

    /* reset and free the tree node */
    swami_prop_tree_node_reset_L(proptree, treenode);
    swami_prop_tree_free_node_L(treenode);

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_remove_recursive:
 * @proptree: Property tree object
 * @obj: Object in @proptree to recursively remove
 *
 * Recursively removes an @object, and all values bound to it, from a property
 * tree.
 */
void
swami_prop_tree_remove_recursive(SwamiPropTree *proptree, GObject *obj)
{
    GNode *obj_node;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(obj));

    SWAMI_LOCK_WRITE(proptree);

    obj_node = g_hash_table_lookup(proptree->object_hash, obj);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    recursive_remove_nodes(obj_node, proptree);  /* recursive remove */

    if(obj_node == proptree->tree)
    {
        proptree->tree = NULL;
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/* called when an object in the tree is destroyed */
static void
swami_prop_tree_object_weak_notify(gpointer user_data, GObject *object)
{
    SwamiPropTree *proptree = SWAMI_PROP_TREE(user_data);
    GNode *obj_node;

    SWAMI_LOCK_WRITE(proptree);

    obj_node = g_hash_table_lookup(proptree->object_hash, object);

    if(obj_node && proptree->tree)
    {
        ((SwamiPropTreeNode *)(obj_node->data))->object = NULL;
        recursive_remove_nodes(obj_node, proptree);  /* recursive remove */

        if(obj_node == proptree->tree)
        {
            proptree->tree = NULL;
        }
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/* reset a SwamiPropTreeNode (remove all of its innards) */
static inline void
swami_prop_tree_node_reset_L(SwamiPropTree *proptree,
                             SwamiPropTreeNode *treenode)
{
    GSList *p;

    if(treenode->object)	        /* clear object->treenode hash entry */
    {
        g_object_weak_unref(treenode->object,  /* -- remove the weak ref */
                            swami_prop_tree_object_weak_notify, proptree);
        g_hash_table_remove(proptree->object_hash, treenode->object);
    }

    p = treenode->values;

    while(p)			/* destroy values */
    {
        swami_prop_tree_free_value_L(p->data);
        p = g_slist_delete_link(p, p);
    }

    swami_prop_tree_node_clear_cache_L(treenode);
}

/* clear cache of a property tree node */
static inline void
swami_prop_tree_node_clear_cache_L(SwamiPropTreeNode *treenode)
{
    CacheValue *cache;
    GSList *p;

    p = treenode->cache;

    while(p)			/* destroy cache */
    {
        cache = (CacheValue *)(p->data);

        if(cache->prop_ctrl)     /* destroy property control */
        {
            swami_control_disconnect_all((SwamiControl *)(cache->prop_ctrl));
            g_object_unref(cache->prop_ctrl);  /* -- unref from cache */
        }

        swami_prop_tree_free_cache_L(p->data);  /* free cache value */
        p = g_slist_delete_link(p, p);
    }

    treenode->cache = NULL;
}

/* recursively remove SwamiPropTreeNodes */
static void
recursive_remove_nodes(GNode *node, SwamiPropTree *proptree)
{
    GNode *n;

    n = node->children;

    while(n)
    {
        recursive_remove_nodes(n, proptree);
        n = n->next;
    }

    {
        SwamiPropTreeNode *treenode = (SwamiPropTreeNode *)(node->data);

        swami_prop_tree_node_reset_L(proptree, treenode);
        swami_prop_tree_free_node_L(treenode);  /* free node */
        g_node_destroy(node);
    }
}

/**
 * swami_prop_tree_replace:
 * @proptree: Property tree object
 * @old: Old object in @proptree to replace
 * @new: New object to replace @old object with
 *
 * Replaces an @old object with a @new object in a property tree.
 */
void
swami_prop_tree_replace(SwamiPropTree *proptree, GObject *old, GObject *new)
{
    SwamiPropTreeNode *treenode;
    GNode *obj_node;
    GList *speclist;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(old));
    g_return_if_fail(G_IS_OBJECT(new));

    speclist = object_spec_list(new);  /* get GParamSpec list for new object */

    SWAMI_LOCK_WRITE(proptree);

    obj_node = g_hash_table_lookup(proptree->object_hash, old);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        g_list_free(speclist);
        return;
    }

    treenode = (SwamiPropTreeNode *)(obj_node->data);

    /* clear old cache and remove old object hash entry */
    swami_prop_tree_node_clear_cache_L(treenode);
    g_hash_table_remove(proptree->object_hash, old);

    g_hash_table_insert(proptree->object_hash, new, obj_node);

    /* weak ref to passively catch objects demise */
    g_object_weak_ref(new, swami_prop_tree_object_weak_notify, proptree);

    /* re-resolve properties if any (speclist is freed) */
    if(speclist)
    {
        resolve_object_props_L(proptree, obj_node, speclist);
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_get_children:
 * @proptree: Property tree object
 * @obj: Object in @proptree to get children of
 *
 * Gets the list of GObject children of @obj in a property tree.
 *
 * Returns: A new object list populated with the children of @obj in @proptree.
 * The new list has a reference count of 1 which the caller owns, remember to
 * unref it when finished.
 */
IpatchList *
swami_prop_tree_get_children(SwamiPropTree *proptree, GObject *obj)
{
    IpatchList *list;
    SwamiPropTreeNode *treenode;
    GNode *obj_node, *n;

    g_return_val_if_fail(SWAMI_IS_PROP_TREE(proptree), NULL);
    g_return_val_if_fail(G_IS_OBJECT(obj), NULL);

    list = ipatch_list_new();	/* ++ ref new list */

    SWAMI_LOCK_READ(proptree);
    obj_node = g_hash_table_lookup(proptree->object_hash, obj);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_READ(proptree);
        g_object_unref(list);	/* -- unref list */
        return (NULL);
    }

    n = obj_node->children;

    while(n)
    {
        treenode = (SwamiPropTreeNode *)(n->data);
        g_object_ref(treenode->object);  /* ++ ref for list */
        list->items = g_list_prepend(list->items, treenode->object);
        n = n->next;
    }

    SWAMI_UNLOCK_READ(proptree);

    list->items = g_list_reverse(list->items);

    return (list);
}

/**
 * swami_prop_tree_get_node:
 * @proptree: Property tree object
 * @obj: Object in @proptree to get GNode of
 *
 * Gets the GNode of an object in a property tree. This should only be done
 * for objects which are sure to remain in the property tree for the duration
 * of GNode use.
 *
 * Returns: GNode of @obj in property tree. GNode can only be used for as
 * long as the object is in the tree.
 */
GNode *
swami_prop_tree_object_get_node(SwamiPropTree *proptree, GObject *obj)
{
    GNode *node;

    g_return_val_if_fail(SWAMI_IS_PROP_TREE(proptree), NULL);
    g_return_val_if_fail(G_IS_OBJECT(obj), NULL);

    SWAMI_LOCK_READ(proptree);
    node = g_hash_table_lookup(proptree->object_hash, obj);
    SWAMI_UNLOCK_READ(proptree);

    return (node);
}

/**
 * swami_prop_tree_add_value:
 * @proptree: Property tree object
 * @obj: Object in @proptree
 * @prop_type: GObject derived type the value should match (0 = wildcard)
 * @prop_name: Property name to match
 * @control: Active value control
 *
 * Adds a value to an object in a property tree. If a value already exists
 * with the same @prop_type and @prop_name its @control value is replaced.
 */
void
swami_prop_tree_add_value(SwamiPropTree *proptree, GObject *obj,
                          GType prop_type, const char *prop_name,
                          SwamiControl *control)
{
    SwamiPropTreeValue *treeval;
    SwamiPropTreeNode *treenode;
    GNode *obj_node;
    GSList *p;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(obj));
    g_return_if_fail(!prop_type || g_type_is_a(prop_type, G_TYPE_OBJECT));
    g_return_if_fail(prop_name != NULL && *prop_name != '\0');
    g_return_if_fail(SWAMI_IS_CONTROL(control));

    SWAMI_LOCK_WRITE(proptree);

    obj_node = g_hash_table_lookup(proptree->object_hash, obj);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    treenode = (SwamiPropTreeNode *)(obj_node->data);

    g_object_ref(control);	/* ++ ref control for tree value */

    p = treenode->values;

    while(p)		   /* look for existing duplicate tree value */
    {
        treeval = (SwamiPropTreeValue *)(p->data);

        if(treeval->prop_type == prop_type
                && strcmp(treeval->prop_name, prop_name) == 0)
        {
            g_object_unref(control);  /* -- unref old control */
            break;
        }

        p = g_slist_next(p);
    }

    /* if a matching value doesn't already exist, allocate a new one */
    if(!p)
    {
        treeval = swami_prop_tree_new_value_L();
        treeval->prop_type = prop_type;
        treeval->prop_name = g_strdup(prop_name);
        treenode->values = g_slist_prepend(treenode->values, treeval);
    }

    treeval->control = control;

    refresh_value_nodes_L(obj_node, treeval);  /* refresh nodes */

    SWAMI_UNLOCK_WRITE(proptree);
}

/**
 * swami_prop_tree_remove_value:
 * @proptree: Property tree object
 * @obj: Object in @proptree
 * @prop_type: GObject derived type field of existing value
 * @prop_name: Property name field of existing value
 *
 * Removes a value from an object in a property tree. The @prop_type and
 * @prop_name parameters are used to find the value to remove.
 */
void
swami_prop_tree_remove_value(SwamiPropTree *proptree, GObject *obj,
                             GType prop_type, const char *prop_name)
{
    SwamiPropTreeValue *treeval;
    SwamiPropTreeNode *treenode;
    GNode *obj_node;
    GSList *p, *prev = NULL;

    g_return_if_fail(SWAMI_IS_PROP_TREE(proptree));
    g_return_if_fail(G_IS_OBJECT(obj));
    g_return_if_fail(!prop_type || g_type_is_a(prop_type, G_TYPE_OBJECT));
    g_return_if_fail(prop_name != NULL && *prop_name != '\0');

    obj_node = g_hash_table_lookup(proptree->object_hash, obj);

    if(swami_log_if_fail(obj_node != NULL))
    {
        SWAMI_UNLOCK_WRITE(proptree);
        return;
    }

    treenode = (SwamiPropTreeNode *)(obj_node->data);

    p = treenode->values;

    while(p)			/* loop over values */
    {
        treeval = (SwamiPropTreeValue *)(p->data);

        if(treeval->prop_type == prop_type  /* tree value matches? */
                && strcmp(treeval->prop_name, prop_name) == 0)
        {
            break;
        }

        prev = p;
        p = g_slist_next(p);
    }

    if(p)			/* if a match was found */
    {
        /* quick remove the value from the list */
        if(prev)
        {
            prev->next = p->next;
        }
        else
        {
            treenode->values = p->next;
        }

        refresh_value_nodes_L(obj_node, treeval);  /* refresh nodes */

        /* free the tree value */
        g_free(treeval->prop_name);
        g_object_unref(treeval->control);  /* -- unref control */
        swami_prop_tree_free_value_L(treeval);
    }

    SWAMI_UNLOCK_WRITE(proptree);
}

/* one time property resolve and cache function */
static void
resolve_object_props_L(SwamiPropTree *proptree, GNode *object_node,
                       GList *speclist)
{
    SwamiPropTreeNode *treenode, *obj_treenode;
    SwamiPropTreeValue *treeval;
    SwamiControl *prop_ctrl;
    CacheValue *cache;
    GParamSpec *pspec;
    GType obj_type;
    GNode *n;
    GList *sp, *temp;
    GSList *p;
    int flags;

    obj_treenode = (SwamiPropTreeNode *)(object_node->data);
    obj_type = G_TYPE_FROM_INSTANCE(obj_treenode->object);

    n = object_node;

    do
    {
        /* loop over tree ancestry */
        treenode = (SwamiPropTreeNode *)(n->data);
        p = treenode->values;

        while(p)			/* loop over variables in each node */
        {
            treeval = (SwamiPropTreeValue *)(p->data);

            /* object type matches? */
            if(!treeval->prop_type || (treeval->prop_type == obj_type))
            {
                sp = speclist;

                while(sp)   /* loop over remaining object param specs */
                {
                    pspec = (GParamSpec *)(sp->data);

                    if(strcmp(pspec->name, treeval->prop_name) == 0)
                    {
                        /* property name matches */
                        /* create a new object property control */
                        prop_ctrl = swami_get_control_prop_by_name
                                    (obj_treenode->object, pspec->name);

                        flags = SWAMI_CONTROL_CONN_INIT;

                        if(swami_control_get_flags(prop_ctrl)
                                & SWAMI_CONTROL_SENDS)
                        {
                            flags |= SWAMI_CONTROL_CONN_BIDIR;
                        }

                        /* connect the tree value control to the property control
                        and initialize the property to the tree value */
                        swami_control_connect(treeval->control,
                                              (SwamiControl *)prop_ctrl, flags);

                        /* create cache value and add it to object tree node */
                        cache = swami_prop_tree_new_cache_L();
                        cache->pspec = pspec;
                        cache->prop_ctrl = prop_ctrl;
                        cache->value = treeval;
                        cache->value_node = treenode;
                        obj_treenode->cache = g_slist_prepend
                                              (obj_treenode->cache, cache);

                        temp = sp;
                        sp = g_list_next(sp);

                        /* delete param spec from find spec list */
                        speclist = g_list_delete_link(speclist, temp);

                        if(!speclist)
                        {
                            goto done;    /* no more properties? */
                        }
                    }
                    else
                    {
                        sp = g_list_next(sp);
                    }
                }
            }

            p = g_slist_next(p);
        }
    }
    while((n = n->parent));

done:

    while(speclist)		/* loop over remaining parameter specs */
    {
        pspec = (GParamSpec *)(speclist->data);

        /* create an "unset" cache value */
        cache = swami_prop_tree_new_cache_L();
        cache->pspec = pspec;
        cache->prop_ctrl = NULL;
        cache->value = NULL;
        cache->value_node = NULL;
        obj_treenode->cache = g_slist_prepend(obj_treenode->cache, cache);

        /* free each node of the spec list */
        speclist = g_list_delete_link(speclist, speclist);
    }
}

/* create a GList of parameter specs for a given object */
static GList *
object_spec_list(GObject *object)
{
    GParamSpec **pspecs, **spp;
    GList *speclist = NULL;

    pspecs = g_object_class_list_properties(G_OBJECT_GET_CLASS(object), NULL);

    spp = pspecs;

    while(*spp)			/* create a param spec list */
    {
        speclist = g_list_prepend(speclist, *spp);
        spp++;
    }

    g_free(pspecs);		/* no longer need array */

    return (speclist);
}

/* recursively refresh cache values affected by a tree value */
static void
refresh_value_nodes_L(GNode *node, SwamiPropTreeValue *treeval)
{
    GNode *n;

    n = node->children;

    while(n)
    {
        refresh_value_nodes_L(n, treeval);
        n = n->next;
    }

    {
        SwamiPropTreeNode *treenode = (SwamiPropTreeNode *)(node->data);
        GType obj_type = G_TYPE_FROM_INSTANCE(treenode->object);
        char *prop_name = treeval->prop_name;
        CacheValue *cache;
        GSList *p;

        /* object type matches type criteria of treeval? */
        if(treeval->prop_type && treeval->prop_type != obj_type)
        {
            return;
        }

        p = treenode->cache;

        while(p)			/* loop over the node's cache */
        {
            cache = (CacheValue *)(p->data);

            if(strcmp(prop_name, cache->pspec->name) == 0)
            {
                break;
            }

            p = g_slist_next(p);
        }

        if(p)
        {
            refresh_cache_value_L(node, cache);
        }
    }
}

/* recursively refresh cache values affected by a list of tree values */
static void
refresh_value_nodes_list_L(GNode *node, GSList *treevals)
{
    GNode *n;

    n = node->children;

    while(n)
    {
        refresh_value_nodes_list_L(n, treevals);
        n = n->next;
    }

    {
        SwamiPropTreeNode *treenode = (SwamiPropTreeNode *)(node->data);
        GType obj_type = G_TYPE_FROM_INSTANCE(treenode->object);
        SwamiPropTreeValue *treeval;
        CacheValue *cache;
        GSList *p, *p2;

        p = treevals;

        while(p)  /* loop over list of tree values to match for refresh */
        {
            treeval = (SwamiPropTreeValue *)(p->data);

            /* treeval matches type criteria of treeval? */
            if(!treeval->prop_type || treeval->prop_type == obj_type)
            {
                p2 = treenode->cache;

                while(p2)			/* loop over the node's cache */
                {
                    cache = (CacheValue *)(p2->data);

                    /* prop name matches? - refresh cache val */
                    if(strcmp(treeval->prop_name, cache->pspec->name) == 0)
                    {
                        refresh_cache_value_L(node, cache);
                    }

                    p2 = g_slist_next(p2);
                }
            }

            p = g_slist_next(p);
        }
    }
}

/* refreshes a single cache value in a tree node */
static inline void
refresh_cache_value_L(GNode *node, CacheValue *cache)
{
    SwamiPropTreeNode *treenode, *obj_treenode;
    SwamiPropTreeValue *treeval;
    const char *prop_name;
    GType obj_type;
    GSList *p;
    int flags;

    obj_treenode = (SwamiPropTreeNode *)(node->data);
    obj_type = G_TYPE_FROM_INSTANCE(obj_treenode->object);
    prop_name = cache->pspec->name;

    do
    {
        /* loop over tree ancestry */
        treenode = (SwamiPropTreeNode *)(node->data);
        p = treenode->values;

        while(p)			/* loop over variables in each node */
        {
            treeval = (SwamiPropTreeValue *)(p->data);

            /* property type and name matches? */
            if((!treeval->prop_type || (treeval->prop_type == obj_type))
                    && strcmp(prop_name, treeval->prop_name) == 0)
            {
                /* property name matches */
                if(cache->value == treeval)
                {
                    return;    /* cache is correct? */
                }

                if(!cache->prop_ctrl)  /* no existing property control? */
                    cache->prop_ctrl = /* ++ ref new property control */
                        swami_get_control_prop_by_name(obj_treenode->object,
                                                       prop_name);
                else	     /* disconnect current property control */
                    swami_control_disconnect_all
                    ((SwamiControl *)(cache->prop_ctrl));

                /* update the cached tree value */
                cache->value = treeval;
                cache->value_node = treenode;

                flags = SWAMI_CONTROL_CONN_INIT;

                if(swami_control_get_flags(cache->prop_ctrl)
                        & SWAMI_CONTROL_SENDS)
                {
                    flags |= SWAMI_CONTROL_CONN_BIDIR;
                }

                /* connect tree value control to property control and initialize
                to current value (connect bi-direction if prop_ctrl sends) */
                swami_control_connect(treeval->control,
                                      (SwamiControl *)(cache->prop_ctrl),
                                      flags);
                return;
            }

            p = g_slist_next(p);
        }
    }
    while((node = node->parent));

    /* no tree value found to satisfy cache property criteria */

    if(cache->prop_ctrl)	/* destroy unused property control? */
    {
        swami_control_disconnect_all((SwamiControl *)(cache->prop_ctrl));
        g_object_unref(cache->prop_ctrl);  /* -- unref prop ctrl from cache */
        cache->prop_ctrl = NULL;
    }

    /* set cached value to "unset" state */
    cache->value = NULL;
    cache->value_node = NULL;
}
