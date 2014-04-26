/*
 * SwamiControlProp.c - GObject property control object
 * Special support for IpatchItem properties (don't use "notify")
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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

#include <libinstpatch/IpatchItem.h>
#include <libswami/SwamiLog.h>

#include "SwamiControlProp.h"
#include "libswami.h"
#include "swami_priv.h"


/* hash key used for cache of SwamiControlProp by Object:GParamSpec */
typedef struct
{
  GObject *object;	/* object, no ref is held, weak notify instead */
  GParamSpec *pspec;	/* property param spec of the control */
} ControlPropKey;

/* defined in libswami.c */
extern void _swami_set_patch_prop_origin_event (SwamiControlEvent *origin);


static void control_prop_weak_notify (gpointer data, GObject *was_object);
static guint control_prop_hash_func (gconstpointer key);
static gboolean control_prop_equal_func (gconstpointer a, gconstpointer b);
static void control_prop_key_free_func (gpointer data);

static void swami_control_prop_class_init (SwamiControlPropClass *klass);
static void swami_control_prop_finalize (GObject *object);
static GParamSpec *control_prop_get_spec_method (SwamiControl *control);
static void control_prop_get_value_method (SwamiControl *control,
					   GValue *value);
static void control_prop_set_value_method (SwamiControl *control,
					   SwamiControlEvent *event,
					   const GValue *value);
static void swami_control_prop_item_cb_notify (IpatchItemPropNotify *notify);
static void swami_control_prop_item_cb_notify_event (IpatchItemPropNotify *notify);
static void swami_control_prop_cb_notify (GObject *object, GParamSpec *pspec,
					  SwamiControlProp *ctrlprop);
static void swami_control_prop_cb_notify_event (GObject *object,
						GParamSpec *pspec,
						SwamiControlProp *ctrlprop);
static void control_prop_object_weak_notify (gpointer user_data, GObject *object);


static GObjectClass *parent_class = NULL;

/* hash of GObject:GParamSpec -> SwamiControlProp objects. Only 1 control
   needed for an object property. */
G_LOCK_DEFINE_STATIC (control_prop_hash);
static GHashTable *control_prop_hash = NULL;

/* reverse hash for quick lookup by control (on control removal)
 * SwamiControlProp -> ControlPropKey (uses key from control_prop_hash) */
static GHashTable *control_prop_reverse_hash = NULL;

/* thread private variable for preventing IpatchItem property loops,
   stores current origin SwamiControlEvent for property changes during
   IpatchItem property set, NULL when not in use */
static GStaticPrivate prop_notify_origin = G_STATIC_PRIVATE_INIT;


/**
 * swami_get_control_prop:
 * @object: Object to get property control from (%NULL for wildcard,
 *   #IpatchItem only)
 * @pspec: Property parameter spec of @object to get control for (%NULL for
 *   wildcard, #IpatchItem only)
 *
 * Gets the #SwamiControlProp object associated with an object's GObject
 * property by @pspec.  If a control for the given @object and @pspec
 * does not yet exist, then it is created and returned.  Passing
 * %NULL for @object and/or @pspec create's a wildcard control which receives
 * property change events for a specific property of all items (@object is
 * %NULL), any property of a specifc item  (@pspec is %NULL) or all item property
 * changes (both are %NULL).  Note that wildcard property controls only work
 * for #IpatchItem derived objects currently.
 *
 * Returns: The control associated with the @object and property @pspec.
 *   Caller owns a reference to the returned object and should unref it when
 *   finished.
 */
SwamiControl *
swami_get_control_prop (GObject *object, GParamSpec *pspec)
{
  ControlPropKey key, *newkey;
  SwamiControlProp *control, *beatus;

  key.object = object;
  key.pspec = pspec;

  G_LOCK (control_prop_hash);
  control = (SwamiControlProp *)g_hash_table_lookup (control_prop_hash, &key);
  if (control) g_object_ref (control);
  G_UNLOCK (control_prop_hash);

  if (!control)
    { /* ++ ref control (caller takes over reference) */
      control = swami_control_prop_new (object, pspec);
      g_return_val_if_fail (control != NULL, NULL);

      newkey = g_slice_new (ControlPropKey);
      newkey->object = object;
      newkey->pspec = pspec;


      G_LOCK (control_prop_hash);

      /* double check that another thread didn't create the same control between
       * these two locks (beat us to it) */
      beatus = (SwamiControlProp *)g_hash_table_lookup (control_prop_hash, &key);

      if (!beatus)
	{
	  g_hash_table_insert (control_prop_hash, newkey, control);
	  g_hash_table_insert (control_prop_reverse_hash, control, newkey);
	}
      else g_object_ref (beatus);

      G_UNLOCK (control_prop_hash);


      if (beatus)	/* if another thread created control, cleanup properly */
	{
	  g_slice_free (ControlPropKey, newkey);
	  g_object_unref (control);	/* -- unref */
	  control = beatus;
	}
      else /* passively watch the control, to remove it from hash when destroyed */
	g_object_weak_ref (G_OBJECT (control), control_prop_weak_notify, NULL);
    }

  return ((SwamiControl *)control);
}

/* weak notify to remove control from hash when it gets destroyed */
static void
control_prop_weak_notify (gpointer data, GObject *was_object)
{
  ControlPropKey *key;

  G_LOCK (control_prop_hash);

  /* lookup key in reverse hash */
  key = g_hash_table_lookup (control_prop_reverse_hash, was_object);

  if (key)	/* still in hash? (this func may be called multiple times) */
    {
      g_hash_table_remove (control_prop_reverse_hash, was_object);
      g_hash_table_remove (control_prop_hash, key);
    }

  G_UNLOCK (control_prop_hash);
}

/**
 * swami_get_control_prop_by_name:
 * @object: Object to get property control from
 * @name: A property of @object to get the control for (or %NULL for wildcard,
 *   #IpatchItem derived objects only)
 *
 * Like swami_get_control_prop() but takes a property name
 * instead.  It is also therefore not possible to specify a wildcard @object
 * (%NULL).
 *
 * Returns: The control associated with the @object and property @name.
 *   Caller owns a reference to the returned object and should unref it when
 *   finished.
 */
SwamiControl *
swami_get_control_prop_by_name (GObject *object, const char *name)
{
  GParamSpec *pspec;
  GObjectClass *klass;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  if (name)
    {
      klass = G_OBJECT_GET_CLASS (object);
      g_return_val_if_fail (klass != NULL, NULL);
    
      pspec = g_object_class_find_property (klass, name);
      g_return_val_if_fail (pspec != NULL, NULL);
    }
  else pspec = NULL;

  return (swami_get_control_prop (object, pspec));
}

/**
 * swami_control_prop_connect_objects:
 * @src: Source object
 * @propname1: Property of source object (and @dest if @propname2 is %NULL).
 * @dest: Destination object
 * @propname2: Property of destination object (%NULL to use @propname1).
 * @flags: Same flags as for swami_control_connect().
 *
 * Connect the properties of two objects together using #SwamiControlProp
 * controls.
 */
void
swami_control_prop_connect_objects (GObject *src, const char *propname1,
				    GObject *dest, const char *propname2,
				    guint flags)
{
  SwamiControl *sctrl, *dctrl;

  g_return_if_fail (G_IS_OBJECT (src));
  g_return_if_fail (propname1 != NULL);
  g_return_if_fail (G_IS_OBJECT (dest));

  sctrl = swami_get_control_prop_by_name (src, propname1);	/* ++ ref */
  g_return_if_fail (sctrl != NULL);

  /* ++ ref */
  dctrl = swami_get_control_prop_by_name (dest, propname2 ? propname2 : propname1);
  if (swami_log_if_fail (dctrl != NULL))
    {
      g_object_unref (sctrl);	/* -- unref */
      return;
    }

  swami_control_connect (sctrl, dctrl, flags);

  g_object_unref (sctrl);	/* -- unref */
  g_object_unref (dctrl);	/* -- unref */
}

/**
 * swami_control_prop_connect_to_control:
 * @src: Object with property to connect as source
 * @propname: Property of @object to use as source control
 * @dest: Destination control to connect the object property to
 * @flags: Same flags as for swami_control_connect().
 *
 * A convenience function to connect an object property as the source control
 * to another #SwamiControl.
 */
void
swami_control_prop_connect_to_control (GObject *src, const char *propname,
				       SwamiControl *dest, guint flags)
{
  SwamiControl *sctrl;

  g_return_if_fail (G_IS_OBJECT (src));
  g_return_if_fail (propname != NULL);
  g_return_if_fail (SWAMI_IS_CONTROL (dest));

  sctrl = swami_get_control_prop_by_name (src, propname);	/* ++ ref */
  g_return_if_fail (sctrl != NULL);

  swami_control_connect (sctrl, dest, flags);

  g_object_unref (sctrl);	/* -- unref */
}

/**
 * swami_control_prop_connect_from_control:
 * @src: Source control to connect to an object property
 * @dest: Object with property to connect to
 * @propname: Property of @object to use as destination control
 * @flags: Same flags as for swami_control_connect().
 *
 * A convenience function to connect a #SwamiControl to an object property as
 * the destination control.
 */
void
swami_control_prop_connect_from_control (SwamiControl *src, GObject *dest,
					 const char *propname, guint flags)
{
  SwamiControl *dctrl;

  g_return_if_fail (SWAMI_IS_CONTROL (src));
  g_return_if_fail (G_IS_OBJECT (dest));
  g_return_if_fail (propname != NULL);

  dctrl = swami_get_control_prop_by_name (dest, propname);	/* ++ ref */
  g_return_if_fail (dctrl != NULL);

  swami_control_connect (src, dctrl, flags);

  g_object_unref (dctrl);	/* -- unref */
}

static guint
control_prop_hash_func (gconstpointer key)
{
  ControlPropKey *pkey = (ControlPropKey *)key;

  return (GPOINTER_TO_UINT (pkey->object) + GPOINTER_TO_UINT (pkey->pspec));
}

static gboolean
control_prop_equal_func (gconstpointer a, gconstpointer b)
{
  ControlPropKey *akey = (ControlPropKey *)a;
  ControlPropKey *bkey = (ControlPropKey *)b;

  return (akey->object == bkey->object && akey->pspec == bkey->pspec);
}

static void
control_prop_key_free_func (gpointer data)
{
  g_slice_free (ControlPropKey, data);
}

GType
swami_control_prop_get_type (void)
{
  static GType otype = 0;

  if (!otype)
    {
      static const GTypeInfo type_info =
	{
	  sizeof (SwamiControlPropClass), NULL, NULL,
	  (GClassInitFunc) swami_control_prop_class_init,
	  (GClassFinalizeFunc) NULL, NULL,
	  sizeof (SwamiControlProp), 0,
	  (GInstanceInitFunc) NULL
	};

      control_prop_hash =
	g_hash_table_new_full (control_prop_hash_func,
			       control_prop_equal_func,
			       control_prop_key_free_func, NULL);

      control_prop_reverse_hash = g_hash_table_new (NULL, NULL);

      otype = g_type_register_static (SWAMI_TYPE_CONTROL, "SwamiControlProp",
				      &type_info, 0);
    }

  return (otype);
}

static void
swami_control_prop_class_init (SwamiControlPropClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  SwamiControlClass *control_class = SWAMI_CONTROL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = swami_control_prop_finalize;

  control_class->get_spec = control_prop_get_spec_method;
  control_class->set_spec = NULL;
  control_class->get_value = control_prop_get_value_method;
  control_class->set_value = control_prop_set_value_method;
}

static void
swami_control_prop_finalize (GObject *object)
{
  SwamiControlProp *ctrlprop = SWAMI_CONTROL_PROP (object);

  SWAMI_LOCK_WRITE (ctrlprop);

  if (ctrlprop->object)
    {
      if (ctrlprop->item_handler_id)
	ipatch_item_prop_disconnect (ctrlprop->item_handler_id);
      else if (g_signal_handler_is_connected (ctrlprop->object,
					      ctrlprop->notify_handler_id))
	g_signal_handler_disconnect (ctrlprop->object,
				     ctrlprop->notify_handler_id);

      g_object_weak_unref (ctrlprop->object, control_prop_object_weak_notify,
			   ctrlprop);

      ctrlprop->object = NULL;
    }
  if (ctrlprop->spec) g_param_spec_unref (ctrlprop->spec);

  SWAMI_UNLOCK_WRITE (ctrlprop);

  if (parent_class->finalize)
    parent_class->finalize (object);
}

/* control is locked by caller */
static GParamSpec *
control_prop_get_spec_method (SwamiControl *control)
{
  SwamiControlProp *ctrlprop = SWAMI_CONTROL_PROP (control);
  return (ctrlprop->spec);
}

/* NOT locked by caller */
static void
control_prop_get_value_method (SwamiControl *control, GValue *value)
{
  SwamiControlProp *ctrlprop = SWAMI_CONTROL_PROP (control);
  GObject *object;
  GParamSpec *spec;

  SWAMI_LOCK_READ (ctrlprop);

  if (!ctrlprop->object || !ctrlprop->spec)
    {
      SWAMI_UNLOCK_READ (ctrlprop);
      return;
    }

  spec = ctrlprop->spec;	/* no need to ref, since owner object ref'd */

  object = g_object_ref (ctrlprop->object); /* ++ ref object */

  SWAMI_UNLOCK_READ (ctrlprop);

  /* OPTME - Faster, but doesn't work for overridden properties (wrong param_id) */
  /* klass->get_property (object, SWAMI_PARAM_SPEC_ID (spec), value, spec); */

  g_object_get_property (object, spec->name, value);

  g_object_unref (object);	/* -- unref object */
}

/* NOT locked by caller */
static void
control_prop_set_value_method (SwamiControl *control, SwamiControlEvent *event,
			       const GValue *value)
{
  SwamiControlProp *ctrlprop = SWAMI_CONTROL_PROP (control);
  guint notify_handler_id, item_handler_id;
  GParamSpec *spec;
  GObject *object;

  SWAMI_LOCK_READ (ctrlprop);

  if (!ctrlprop->object || !ctrlprop->spec)
    {
      SWAMI_UNLOCK_READ (ctrlprop);
      return;
    }

  object = g_object_ref (ctrlprop->object); /* ++ ref object */
  spec = ctrlprop->spec;	/* no need to ref since we ref'd owner obj */
  item_handler_id = ctrlprop->item_handler_id;
  notify_handler_id = ctrlprop->notify_handler_id;

  SWAMI_UNLOCK_READ (ctrlprop);


  if (item_handler_id) /* IpatchItem object? */
    { 
      /* set current thread IpatchItem origin event to prevent event loops */
      g_static_private_set (&prop_notify_origin, event->origin ? event->origin : event, NULL);

      /* OPTME - Faster but can't use for overridden properties (wrong param_id) */
      // klass->set_property (object, SWAMI_PARAM_SPEC_ID (spec), value, spec);

      g_object_set_property (object, spec->name, value);

      /* IpatchItem set property no longer active for this thread */
      g_static_private_set (&prop_notify_origin, NULL, NULL);
    }
  else				/* non IpatchItem object */
    {
      /* block handler to avoid property set/notify loop (object "notify") */
      g_signal_handler_block (object, notify_handler_id);

      /* OPTME - Faster but can't use for overridden properties (wrong param_id) */
      // klass->set_property (object, SWAMI_PARAM_SPEC_ID (spec), value, spec);

      g_object_set_property (object, spec->name, value);

      g_signal_handler_unblock (object, notify_handler_id);
    }

  g_object_unref (object);	/* -- unref the object */

  /* propagate to outputs - FIXME: Should all controls do this? */
  swami_control_transmit_event_loop (control, event);
}

/**
 * swami_control_prop_new:
 * @object: Object with property to control or %NULL for wildcard
 * @pspec: Parameter spec of property to control or %NULL for wildcard
 *
 * Create a new GObject property control.  Note that swami_get_control_prop()
 * is likely more desireable to use, since it will return an existing control
 * if one already exists for the given @object and @pspec.
 * 
 * If one of @object or @pspec is %NULL then it acts as a wildcard and the
 * control will send only (transmit changes for matching properties).  If both
 * are %NULL however, the control has no active property to control (use
 * swami_get_control_prop() if a #IpatchItem entirely wildcard callback is
 * desired).  If @object and/or @pspec is wildcard then the control will use
 * #SwamiEventPropChange events instead of just the property value.
 *
 * Returns: New GObject property control with a refcount of 1 which the caller
 * owns.
 */
SwamiControlProp *
swami_control_prop_new (GObject *object, GParamSpec *pspec)
{
  SwamiControlProp *ctrlprop;

  ctrlprop = g_object_new (SWAMI_TYPE_CONTROL_PROP, NULL);
  swami_control_prop_assign (ctrlprop, object, pspec,
			     (!object || !pspec) && (object || pspec));

  return (ctrlprop);
}

/**
 * swami_control_prop_assign:
 * @ctrlprop: Swami property control object
 * @object: Object containing the property to control (%NULL = wildcard)
 * @pspec: Parameter spec of the property of @object to control (%NULL = wildcard)
 * @send_events: Set to %TRUE to send/receive #SwamiEventPropChange events
 *   for the @ctrlprop, %FALSE will use just the property value.
 *
 * Assign the object property to control for a #SwamiControlProp object.
 * If a property is already set for the control the new object property must
 * honor existing input/output connections by being writable/readable
 * respectively and have the same value type.
 *
 * If one of @object or @pspec is %NULL then it acts as a wildcard and the
 * control will send only (transmit changes for matching properties).  If both
 * are %NULL however, the control has no active property to control.
 */
void
swami_control_prop_assign (SwamiControlProp *ctrlprop,
			   GObject *object, GParamSpec *pspec,
			   gboolean send_events)
{
  SwamiControl *control;
  GType value_type;

  g_return_if_fail (SWAMI_IS_CONTROL_PROP (ctrlprop));
  g_return_if_fail (!object || pspec || IPATCH_IS_ITEM (object));

  control = SWAMI_CONTROL (ctrlprop);

  if (pspec && !send_events)
    {
      value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      /* use derived type if GBoxed or GObject parameter */
      if (value_type == G_TYPE_BOXED || value_type == G_TYPE_OBJECT)
	value_type = pspec->value_type;    
    }
  else value_type = SWAMI_TYPE_EVENT_PROP_CHANGE;

  /* set control value type */
  swami_control_set_value_type (control, value_type);
  g_return_if_fail (control->value_type == value_type);

  SWAMI_LOCK_WRITE (ctrlprop);

  /* spec must be supplied and be writable if control has input connections */
  if (control->inputs && !(pspec && (pspec->flags & G_PARAM_WRITABLE)))
    {
      g_critical ("%s: Invalid writable property control object change",
		  G_STRLOC);
      SWAMI_UNLOCK_WRITE (ctrlprop);
      return;
    }

  /* spec can be wildcard or be readable if control has output connections */
  if (control->outputs && (pspec && !(pspec->flags & G_PARAM_READABLE)))
    {
      g_critical ("%s: Invalid readable property control object change",
		  G_STRLOC);
      SWAMI_UNLOCK_WRITE (ctrlprop);
      return;
    }

  if (ctrlprop->object)
    {
      if (ctrlprop->item_handler_id)
	  ipatch_item_prop_disconnect (ctrlprop->item_handler_id);
      else if (g_signal_handler_is_connected (ctrlprop->object,
					      ctrlprop->notify_handler_id))
	g_signal_handler_disconnect (ctrlprop->object,
				     ctrlprop->notify_handler_id);

      ctrlprop->item_handler_id = 0;
      ctrlprop->notify_handler_id = 0;
      g_object_weak_unref (ctrlprop->object, control_prop_object_weak_notify,
			   ctrlprop);
    }

  if (ctrlprop->spec) g_param_spec_unref (ctrlprop->spec);

  ctrlprop->object = object;
  ctrlprop->spec = pspec;
  ctrlprop->send_events = send_events;

  if (object) g_object_weak_ref (object, control_prop_object_weak_notify, ctrlprop);
  if (pspec) g_param_spec_ref (pspec);

  /* set readable/writable control flags to reflect new object property */
  if (pspec && (pspec->flags & G_PARAM_WRITABLE))
    control->flags |= SWAMI_CONTROL_RECVS;
  else control->flags &= ~SWAMI_CONTROL_RECVS;

  if (!pspec || (pspec->flags & G_PARAM_READABLE))
    control->flags |= SWAMI_CONTROL_SENDS;
  else control->flags &= ~SWAMI_CONTROL_SENDS;

  /* IpatchItems are handled differently, wildcard is #IpatchItem only */
  if (!object || !pspec || IPATCH_IS_ITEM (object))
    { /* add a IpatchItem change callback for the given property */
      ctrlprop->item_handler_id
	= ipatch_item_prop_connect ((IpatchItem *)object, pspec,
				    send_events
				    ? swami_control_prop_item_cb_notify_event
				    : swami_control_prop_item_cb_notify,
				    NULL, /* disconnect func */
				    ctrlprop);
    }
  else				/* regular object (not IpatchItem) */
    { /* connect signal to property change notify */
      char *s = g_strconcat ("notify::", pspec->name, NULL);
      ctrlprop->notify_handler_id =
	g_signal_connect (object, s, G_CALLBACK
			  (send_events ? swami_control_prop_cb_notify_event
			   : swami_control_prop_cb_notify), ctrlprop);
      g_free (s);
    }

  SWAMI_UNLOCK_WRITE (ctrlprop);
}

/**
 * swami_control_prop_assign_by_name:
 * @ctrlprop: Swami property control object
 * @object: Object containing the property to control (or %NULL to un-assign)
 * @prop_name: Name of property to assign to property control (or %NULL for
 *   wildcard, #IpatchItem types only, or to un-assign if @object is also %NULL)
 *
 * Like swami_control_prop_assign() but accepts a name of a property instead
 * of the param spec.  Note also that @object may not be wildcard, contrary
 * to the other function.
 */
void
swami_control_prop_assign_by_name (SwamiControlProp *ctrlprop,
				   GObject *object, const char *prop_name)
{
  GParamSpec *pspec = NULL;

  g_return_if_fail (SWAMI_IS_CONTROL_PROP (ctrlprop));
  g_return_if_fail (prop_name == NULL || object != NULL);

  if (prop_name)
    {
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
					   prop_name);
      if (!pspec)
	{
	  g_warning ("%s: object class `%s' has no property named `%s'",
		     G_STRLOC, G_OBJECT_TYPE_NAME (object), prop_name);
	  return;
	}
    }

  swami_control_prop_assign (ctrlprop, object, pspec,
			     object && !prop_name);
}

/* IpatchItem property change notify callback */
static void
swami_control_prop_item_cb_notify (IpatchItemPropNotify *notify)
{
  SwamiControl *ctrlprop = (SwamiControl *)(notify->user_data);
  SwamiControlEvent *ctrlevent, *origin;

  /* copy changed value to a new event */
  ctrlevent = swami_control_event_new (TRUE); /* ++ ref new event */

  g_value_init (&ctrlevent->value, G_VALUE_TYPE (notify->new_value));
  g_value_copy (notify->new_value, &ctrlevent->value);

  /* IpatchItem property loop prevention, get current IpatchItem
     property origin event for this thread (if any) */
  if ((origin = g_static_private_get (&prop_notify_origin)))
    swami_control_event_set_origin (ctrlevent, origin);

  /* transmit the new event to the controls destinations */
  swami_control_transmit_event (ctrlprop, ctrlevent);

  swami_control_event_unref (ctrlevent);	/* -- unref creator's ref */
}

/* used instead of swami_control_prop_item_cb_notify() to send the value as
 * a SwamiEventPropChange event. */
static void
swami_control_prop_item_cb_notify_event (IpatchItemPropNotify *notify)
{
  SwamiControl *ctrlprop = (SwamiControl *)(notify->user_data);
  SwamiControlEvent *ctrlevent, *origin;
  SwamiEventPropChange *propevent;

  propevent = swami_event_prop_change_new ();	/* create prop change event */

  /* load values of property change structure */
  propevent->object = g_object_ref (notify->item);
  propevent->pspec = g_param_spec_ref (notify->pspec);

  g_value_init (&propevent->value, G_VALUE_TYPE (notify->new_value));
  g_value_copy (notify->new_value, &propevent->value);

  /* create the control event */
  ctrlevent = swami_control_event_new (TRUE); /* ++ ref new event */

  g_value_init (&ctrlevent->value, SWAMI_TYPE_EVENT_PROP_CHANGE);
  g_value_take_boxed (&ctrlevent->value, propevent);

  /* IpatchItem property loop prevention, get current IpatchItem
     property origin event for this thread (if any) */
  if ((origin = g_static_private_get (&prop_notify_origin)))
    swami_control_event_set_origin (ctrlevent, origin);

  /* transmit the new event to the controls destinations */
  swami_control_transmit_event (ctrlprop, ctrlevent);

  swami_control_event_unref (ctrlevent);	/* -- unref creator's ref */
}

/* property change notify signal callback */
static void
swami_control_prop_cb_notify (GObject *object, GParamSpec *pspec,
			      SwamiControlProp *ctrlprop)
{
  GValue value = { 0 };

  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

  /* OPTME - Faster, but doesn't work with overriden properties */
  /* klass->get_property (object, SWAMI_PARAM_SPEC_ID (spec), &value,
		       ctrlprop->spec); */

  g_object_get_property (object, pspec->name, &value);

  swami_control_transmit_value ((SwamiControl *)ctrlprop, &value);
  g_value_unset (&value);
}

/* property change notify signal callback (sends event instead of prop value) */
static void
swami_control_prop_cb_notify_event (GObject *object, GParamSpec *pspec,
				    SwamiControlProp *ctrlprop)
{
  SwamiEventPropChange *event;
  GValue value = { 0 };

  /* create property change event structure and load fields */
  event = swami_event_prop_change_new ();

  event->object = g_object_ref (object);
  event->pspec = g_param_spec_ref (pspec);

  g_value_init (&event->value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_object_get_property (object, pspec->name, &event->value);

  /* init value and assign prop change event */
  g_value_init (&value, SWAMI_TYPE_EVENT_PROP_CHANGE);
  g_value_take_boxed (&value, event);

  swami_control_transmit_value ((SwamiControl *)ctrlprop, &value);
  g_value_unset (&value);
}

/* catches object finalization passively */
static void
control_prop_object_weak_notify (gpointer user_data, GObject *object)
{
  SwamiControlProp *ctrlprop = SWAMI_CONTROL_PROP (user_data);
  ControlPropKey key;

  SWAMI_LOCK_WRITE (ctrlprop);

  key.object = object;
  key.pspec = ctrlprop->spec;

  if (ctrlprop->item_handler_id)
    ipatch_item_prop_disconnect (ctrlprop->item_handler_id);

  ctrlprop->item_handler_id = 0;
  ctrlprop->notify_handler_id = 0;
  ctrlprop->object = NULL;

  if (ctrlprop->spec) g_param_spec_unref (ctrlprop->spec);
  ctrlprop->spec = NULL;

  SWAMI_UNLOCK_WRITE (ctrlprop);
  
  /* remove control prop hash entry if any */
  G_LOCK (control_prop_hash);
  g_hash_table_remove (control_prop_reverse_hash, ctrlprop);
  g_hash_table_remove (control_prop_hash, &key);
  G_UNLOCK (control_prop_hash);
}
