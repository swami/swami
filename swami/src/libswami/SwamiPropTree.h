/*
 * SwamiPropTree.h - Header for property tree system
 * A tree of objects with inheritable active values.
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
#ifndef __SWAMI_PROP_TREE_H__
#define __SWAMI_PROP_TREE_H__

#include <glib.h>
#include <glib-object.h>

#include <libinstpatch/libinstpatch.h>

#include <libswami/SwamiControl.h>
#include <libswami/SwamiLock.h>

typedef struct _SwamiPropTree SwamiPropTree;
typedef struct _SwamiPropTreeClass SwamiPropTreeClass;
typedef struct _SwamiPropTreeNode SwamiPropTreeNode;
typedef struct _SwamiPropTreeValue SwamiPropTreeValue;

#define SWAMI_TYPE_PROP_TREE   (swami_prop_tree_get_type ())
#define SWAMI_PROP_TREE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_PROP_TREE, SwamiPropTree))
#define SWAMI_PROP_TREE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_PROP_TREE, SwamiPropTreeClass))
#define SWAMI_IS_PROP_TREE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_PROP_TREE))
#define SWAMI_IS_PROP_TREE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_PROP_TREE))

/* Swami property tree */
struct _SwamiPropTree
{
  SwamiLock parent_instance;	/* derived from SwamiLock */
  GNode *tree;		    /* tree of SwamiPropTreeNode structures */
  GHashTable *object_hash;	/* object->node hash */
};

struct _SwamiPropTreeClass
{
  SwamiLockClass parent_class;
};

/* defines a node of a property tree - an object with a list of node
   property values and cached object property values */
struct _SwamiPropTreeNode
{
  GObject *object;	  /* pointer to the object the node manages */
  GSList *values;	/* list of SwamiPropTreeValue for this node */
  GSList *cache; /* cached values for object (struct in SwamiPropTree.c) */
  guint16 flags;
  guint16 reserved;
};

/* defines an active value in a property tree */
struct _SwamiPropTreeValue
{
  GType prop_type; /* instance type owning property to match (0 = wildcard) */
  char *prop_name;		/* name of property to match */
  SwamiControl *control; /* source value control (defines the value) */
};

GType swami_prop_tree_get_type (void);
SwamiPropTree *swami_prop_tree_new (void);

void swami_prop_tree_set_root (SwamiPropTree *proptree, GObject *root);
void swami_prop_tree_prepend (SwamiPropTree *proptree, GObject *parent,
			      GObject *obj);
#define swami_prop_tree_append(proptree, parent, obj) \
  swami_prop_tree_insert_before (proptree, parent, NULL, obj)
void swami_prop_tree_insert_before (SwamiPropTree *proptree, GObject *parent,
				    GObject *sibling, GObject *obj);
void swami_prop_tree_remove (SwamiPropTree *proptree, GObject *obj);
void swami_prop_tree_remove_recursive (SwamiPropTree *proptree, GObject *obj);
void swami_prop_tree_replace (SwamiPropTree *proptree, GObject *old,
			      GObject *new);
IpatchList *swami_prop_tree_get_children (SwamiPropTree *proptree,
					  GObject *obj);
GNode *swami_prop_tree_object_get_node (SwamiPropTree *proptree, GObject *obj);

void swami_prop_tree_add_value (SwamiPropTree *proptree, GObject *obj,
				GType prop_type, const char *prop_name,
				SwamiControl *control);
void swami_prop_tree_remove_value (SwamiPropTree *proptree, GObject *obj,
				   GType prop_type, const char *prop_name);
#endif
