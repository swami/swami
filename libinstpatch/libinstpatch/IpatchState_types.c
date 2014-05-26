/*
 * IpatchState_types.c - Builtin IpatchState objects
 *
 * Ipatch
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
#include <string.h>

#include <libinstpatch/libinstpatch.h>

#include "IpatchState_types.h"
#include "ipatch_priv.h"
#include "i18n.h"

enum
{
  PROP_ITEM_ADD_0,
  PROP_ITEM_ADD_ITEM
};

enum
{
  PROP_ITEM_REMOVE_0,
  PROP_ITEM_REMOVE_ITEM,
  PROP_ITEM_REMOVE_PARENT
};

enum
{
  PROP_ITEM_CHANGE_0,
  PROP_ITEM_CHANGE_ITEM,
  PROP_ITEM_CHANGE_PSPEC,
  PROP_ITEM_CHANGE_VALUE
};

static void item_add_class_init (IpatchStateItemAddClass *klass);
static void item_add_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec);
static void item_add_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec);
static void item_add_finalize (GObject *object);
static void item_add_restore (const IpatchStateItem *state);

static void item_remove_class_init (IpatchStateItemRemoveClass *klass);
static void item_remove_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec);
static void item_remove_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec);
static void item_remove_finalize (GObject *object);
static void item_remove_restore (const IpatchStateItem *state);
static gboolean item_remove_depend (const IpatchStateItem *item1,
				     const IpatchStateItem *item2);
static gboolean item_remove_conflict (const IpatchStateItem *item1,
				       const IpatchStateItem *item2);

static void item_change_class_init (IpatchStateItemChangeClass *klass);
static void item_change_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec);
static void item_change_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec);
static void item_change_finalize (GObject *object);
static void item_change_restore (const IpatchStateItem *state);
static gboolean item_change_depend (const IpatchStateItem *item1,
				     const IpatchStateItem *item2);
static gboolean item_change_conflict (const IpatchStateItem *item1,
				       const IpatchStateItem *item2);


GObjectClass *item_add_parent_class = NULL;
GObjectClass *item_remove_parent_class = NULL;
GObjectClass *item_change_parent_class = NULL;


/**
 * _ipatch_state_types_init: (skip)
 *
 * Function to initialize types
 */
void
_ipatch_state_types_init (void)
{
  g_type_class_ref (IPATCH_TYPE_STATE_ITEM_ADD);
  g_type_class_ref (IPATCH_TYPE_STATE_ITEM_REMOVE);
  g_type_class_ref (IPATCH_TYPE_STATE_ITEM_CHANGE);
}

/* -------------------------- */
/* patch item add state type  */
/* -------------------------- */

GType
ipatch_state_item_add_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateItemAddClass), NULL, NULL,
      (GClassInitFunc) item_add_class_init, NULL, NULL,
      sizeof (IpatchStateItemAdd), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_STATE_ITEM,
					"IpatchStateItemAdd",
					&item_info, 0);
  }

  return (item_type);
}

static void
item_add_class_init (IpatchStateItemAddClass *klass)
{
  IpatchStateItemClass *state_class = (IpatchStateItemClass *)klass;
  GObjectClass *gobj_class = (GObjectClass *)klass;

  item_add_parent_class = g_type_class_peek_parent (klass);

  gobj_class->set_property = item_add_set_property;
  gobj_class->get_property = item_add_get_property;
  gobj_class->finalize = item_add_finalize;

  state_class->restore = item_add_restore;
  state_class->depend = NULL;
  state_class->conflict = NULL;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ITEM_ADD_ITEM,
			g_param_spec_object ("item", "Item",
					     "Item that was added",
					     IPATCH_TYPE_ITEM,
					     G_PARAM_READWRITE));
}

static void
item_add_set_property (GObject *object, guint property_id,
			const GValue *value, GParamSpec *pspec)
{
  IpatchStateItemAdd *state;
  state = IPATCH_STATE_ITEM_ADD (object);

  switch (property_id)
    {
    case PROP_ITEM_ADD_ITEM:
      g_return_if_fail (state->item == NULL);
      state->item = g_value_get_object (value);
      g_return_if_fail (IPATCH_IS_ITEM (state->item));
      g_object_ref (state->item);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_add_get_property (GObject *object, guint property_id, GValue *value,
			GParamSpec *pspec)
{
  IpatchStateItemAdd *state;
  state = IPATCH_STATE_ITEM_ADD (object);

  switch (property_id)
    {
    case PROP_ITEM_ADD_ITEM:
      g_value_set_object (value, state->item);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_add_finalize (GObject *object)
{
  IpatchStateItemAdd *state = IPATCH_STATE_ITEM_ADD (object);

  if (state->item)
    {
      g_object_unref (state->item);
      state->item = NULL;	/* to catch busted refs to state object */
    }

  G_OBJECT_CLASS (item_add_parent_class)->finalize (object);
}

static void
item_add_restore (const IpatchStateItem *state)
{
  IpatchStateItemAdd *item_add = IPATCH_STATE_ITEM_ADD (state);

  /* remove the item (the original state) */
  ipatch_item_remove (item_add->item);
}


/* ---------------------------- */
/* patch item remove state type */
/* ---------------------------- */

GType
ipatch_state_item_remove_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateItemRemoveClass), NULL, NULL,
      (GClassInitFunc) item_remove_class_init, NULL, NULL,
      sizeof (IpatchStateItemRemove), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_STATE_ITEM,
					"IpatchStateItemRemove",
					&item_info, 0);
  }

  return (item_type);
}

static void
item_remove_class_init (IpatchStateItemRemoveClass *klass)
{
  IpatchStateItemClass *state_class = (IpatchStateItemClass *)klass;
  GObjectClass *gobj_class = (GObjectClass *)klass;

  item_remove_parent_class = g_type_class_peek_parent (klass);

  gobj_class->set_property = item_remove_set_property;
  gobj_class->get_property = item_remove_get_property;
  gobj_class->finalize = item_remove_finalize;

  state_class->restore = item_remove_restore;
  state_class->depend = item_remove_depend;
  state_class->conflict = item_remove_conflict;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_ITEM_REMOVE_ITEM,
			g_param_spec_object ("item", "Item",
					     "Item that will be removed",
					     IPATCH_TYPE_ITEM,
					     G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_ITEM_REMOVE_PARENT,
			g_param_spec_object ("parent", "Parent",
					     "Parent of item to be removed",
					     IPATCH_TYPE_ITEM,
					     G_PARAM_READWRITE));
}

static void
item_remove_set_property (GObject *object, guint property_id,
			   const GValue *value, GParamSpec *pspec)
{
  IpatchStateItemRemove *state;
  state = IPATCH_STATE_ITEM_REMOVE (object);

  switch (property_id)
    {
    case PROP_ITEM_REMOVE_ITEM:
      g_return_if_fail (state->item == NULL);
      state->item = g_value_get_object (value);
      g_return_if_fail (IPATCH_IS_ITEM (state->item));
      g_object_ref (state->item);
      if (!state->parent) state->parent = ipatch_item_get_parent (state->item);
      break;
    case PROP_ITEM_REMOVE_PARENT:
      if (state->parent) g_object_unref (state->parent);
      state->parent = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_remove_get_property (GObject *object, guint property_id, GValue *value,
			   GParamSpec *pspec)
{
  IpatchStateItemRemove *state;
  state = IPATCH_STATE_ITEM_REMOVE (object);

  switch (property_id)
    {
    case PROP_ITEM_REMOVE_ITEM:
      g_value_set_object (value, state->item);
      break;
    case PROP_ITEM_REMOVE_PARENT:
      g_value_set_object (value, state->parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_remove_finalize (GObject *object)
{
  IpatchStateItemRemove *state = IPATCH_STATE_ITEM_REMOVE (object);

  if (state->item) g_object_unref (state->item);
  if (state->parent) g_object_unref (state->parent);

  state->item = NULL;	    /* to catch busted refs to state object */
  state->parent = NULL;

  G_OBJECT_CLASS (item_remove_parent_class)->finalize (object);
}

static void
item_remove_restore (const IpatchStateItem *state)
{
  IpatchStateItemRemove *item_remove;

  g_return_if_fail (IPATCH_IS_STATE_ITEM_REMOVE (state));
  item_remove = IPATCH_STATE_ITEM_REMOVE (state);

  /* add the item (the original state)
     FIXME - Attempt to return item to its original position */
  ipatch_container_append (IPATCH_CONTAINER (item_remove->parent),
			   IPATCH_ITEM (item_remove->item));
}

static gboolean
item_remove_depend (const IpatchStateItem *item1, const IpatchStateItem *item2)
{
  IpatchStateItemRemove *state;

  g_return_val_if_fail (IPATCH_IS_STATE_ITEM_REMOVE (item1), FALSE);
  state = IPATCH_STATE_ITEM_REMOVE (item1);

  /* "remove" depends on "add" with the same item */
  return (IPATCH_IS_STATE_ITEM_ADD (item2) &&
	  IPATCH_STATE_ITEM_ADD (item2)->item == state->item);
}

static gboolean
item_remove_conflict (const IpatchStateItem *item1, const IpatchStateItem *item2)
{
  IpatchStateItemRemove *state;

  g_return_val_if_fail (IPATCH_IS_STATE_ITEM_REMOVE (item1), FALSE);
  state = IPATCH_STATE_ITEM_REMOVE (item1);

  /* "remove" conflicts with "remove" with the same item */
  return (IPATCH_IS_STATE_ITEM_REMOVE (item2) &&
	  IPATCH_STATE_ITEM_REMOVE (item2)->item == state->item);
}


/* ------------------------------------- */
/* patch item property change state type */
/* ------------------------------------- */

GType
ipatch_state_item_change_get_type (void)
{
  static GType item_type = 0;

  if (!item_type) {
    static const GTypeInfo item_info = {
      sizeof (IpatchStateItemChangeClass), NULL, NULL,
      (GClassInitFunc) item_change_class_init, NULL, NULL,
      sizeof (IpatchStateItemChange), 0,
      (GInstanceInitFunc) NULL,
    };

    item_type = g_type_register_static (IPATCH_TYPE_STATE_ITEM,
					"IpatchStateItemChange",
					&item_info, 0);
  }

  return (item_type);
}

static void
item_change_class_init (IpatchStateItemChangeClass *klass)
{
  IpatchStateItemClass *state_class = (IpatchStateItemClass *)klass;
  GObjectClass *gobj_class = (GObjectClass *)klass;

  item_change_parent_class = g_type_class_peek_parent (klass);

  gobj_class->set_property = item_change_set_property;
  gobj_class->get_property = item_change_get_property;
  gobj_class->finalize = item_change_finalize;

  state_class->restore = item_change_restore;
  state_class->depend = item_change_depend;
  state_class->conflict = item_change_conflict;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_ITEM_CHANGE_ITEM,
			g_param_spec_object ("item", "Item",
					     "Item that has changed",
					     IPATCH_TYPE_ITEM,
					     G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_ITEM_CHANGE_PSPEC,
			g_param_spec_pointer ("pspec", "GParamSpec",
					"GParamSpec of the changed property",
					      G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_ITEM_CHANGE_VALUE,
			g_param_spec_boxed ("value", "Value",
					    "Old value of changed property",
					    G_TYPE_VALUE,
					    G_PARAM_READWRITE));
}

static void
item_change_set_property (GObject *object, guint property_id,
			   const GValue *value, GParamSpec *pspec)
{
  IpatchStateItemChange *state = IPATCH_STATE_ITEM_CHANGE (object);
  GParamSpec *p;
  GValue *invalue;

  switch (property_id)
    {
    case PROP_ITEM_CHANGE_ITEM:
      g_return_if_fail (state->item == NULL);
      state->item = g_value_get_object (value);
      g_return_if_fail (IPATCH_IS_ITEM (state->item));
      g_object_ref (state->item);
      break;
    case PROP_ITEM_CHANGE_PSPEC:
      p = g_value_get_pointer (value);
      g_return_if_fail (G_IS_PARAM_SPEC (p));

      if (state->pspec) g_param_spec_unref (state->pspec); /* -- unref old */
      g_param_spec_ref (p);	/* ++ ref param spec */
      state->pspec = p;
      break;
    case PROP_ITEM_CHANGE_VALUE:
      invalue = g_value_get_boxed (value);
      g_return_if_fail (G_IS_VALUE (invalue));

      if (G_IS_VALUE (&state->value)) g_value_unset (&state->value);
      g_value_init (&state->value, G_VALUE_TYPE (invalue));
      g_value_copy (invalue, &state->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_change_get_property (GObject *object, guint property_id, GValue *value,
			   GParamSpec *pspec)
{
  IpatchStateItemChange *state;
  state = IPATCH_STATE_ITEM_CHANGE (object);

  switch (property_id)
    {
    case PROP_ITEM_CHANGE_ITEM:
      g_value_set_object (value, state->item);
      break;
    case PROP_ITEM_CHANGE_PSPEC:
      g_value_set_pointer (value, state->pspec);
      break;
    case PROP_ITEM_CHANGE_VALUE:
      g_value_set_static_boxed (value, &state->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_change_finalize (GObject *object)
{
  IpatchStateItemChange *state = IPATCH_STATE_ITEM_CHANGE (object);

  if (state->item)
    {
      g_object_unref (state->item);
      state->item = NULL;	    /* to catch busted refs to state object */
    }

  if (state->pspec)		/* unref param spec (if any) */
    {
      g_param_spec_unref (state->pspec);
      state->pspec = NULL;
    }

  g_value_unset (&state->value);

  G_OBJECT_CLASS (item_change_parent_class)->finalize (object);
}

static void
item_change_restore (const IpatchStateItem *state)
{
  IpatchStateItemChange *item_change;

  g_return_if_fail (IPATCH_IS_STATE_ITEM_CHANGE (state));
  item_change = IPATCH_STATE_ITEM_CHANGE (state);

  /* OPTME: Could call item_class->set_property directly */
  g_object_set_property (G_OBJECT (item_change->item),
			 item_change->pspec->name,
			 &item_change->value);
}

static gboolean
item_change_depend (const IpatchStateItem *item1, const IpatchStateItem *item2)
{
  IpatchStateItemChange *state;

  g_return_val_if_fail (IPATCH_IS_STATE_ITEM_CHANGE (item1), FALSE);
  state = IPATCH_STATE_ITEM_CHANGE (item1);

  /* "patch change" depends on "add" with the same item or "patch change" with
     same item and property */
  return ((IPATCH_IS_STATE_ITEM_ADD (item2)
	   && ((IpatchStateItemAdd *)(item2))->item == state->item)
	  || (IPATCH_IS_STATE_ITEM_CHANGE (item2)
	      && ((IpatchStateItemChange *)(item2))->item == state->item
	      && ((IpatchStateItemChange *)(item2))->pspec == state->pspec));
}

static gboolean
item_change_conflict (const IpatchStateItem *item1, const IpatchStateItem *item2)
{
  IpatchStateItemChange *state;

  g_return_val_if_fail (IPATCH_IS_STATE_ITEM_CHANGE (item1), FALSE);
  state = IPATCH_STATE_ITEM_CHANGE (item1);

  /* "patch change" conflicts with "patch change" with same item
     and property */
  return (IPATCH_IS_STATE_ITEM_CHANGE (item2)
	  && ((IpatchStateItemChange *)(item2))->item == state->item
	  && ((IpatchStateItemChange *)(item2))->pspec == state->pspec);
}
