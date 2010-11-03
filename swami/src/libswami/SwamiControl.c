/*
 * SwamiControl.c - Swami control base object
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
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>

#include "SwamiControl.h"
#include "SwamiControlEvent.h"
#include "SwamiControlQueue.h"
#include "SwamiControlFunc.h"
#include "SwamiControlProp.h"
#include "SwamiLog.h"
#include "SwamiParam.h"
#include "swami_priv.h"
#include "util.h"
#include "marshals.h"
#include "config.h"

/* work around for bug in G_BREAKPOINT where SIGTRAP symbol is used on
 * some architectures without including signal.h header. */
#if DEBUG
#include <signal.h>
#endif

/* max number of destination connections per control (for mem optimizations) */
#define MAX_DEST_CONNECTIONS 64

enum
{
  CONNECT_SIGNAL,
  DISCONNECT_SIGNAL,
  SPEC_CHANGED_SIGNAL,
  SIGNAL_COUNT
};

/* a structure defining an endpoint of a connection */
typedef struct _SwamiControlConn
{
  guint flags;		/* SwamiControlConnPriority | SwamiControlConnFlags */
  SwamiControl *control;	/* connection control */

  /* for src -> dest connections only */
  SwamiValueTransform trans;	/* transform func */
  GDestroyNotify destroy;	/* function to call when connection is destroyed */
  gpointer data;		/* user data to pass to transform function */
} SwamiControlConn;

#define swami_control_conn_new()  g_slice_new0 (SwamiControlConn)
#define swami_control_conn_free(conn)  g_slice_free (SwamiControlConn, conn)

/* bag used for transmitting values to destination controls */
typedef struct
{
  SwamiControl *control;
  SwamiValueTransform trans;
  gpointer data;
} CtrlUpdateBag;


static void swami_control_class_init (SwamiControlClass *klass);
static void swami_control_init (SwamiControl *control);
static void swami_control_finalize (GObject *object);
static void
swami_control_connect_real (SwamiControl *src, SwamiControl *dest,
			    SwamiValueTransform trans,
			    gpointer data, GDestroyNotify destroy, guint flags);
static gint GCompare_func_conn_priority (gconstpointer a, gconstpointer b);
static void item_prop_value_transform (const GValue *src, GValue *dest,
				       gpointer data);
static void swami_control_real_disconnect (SwamiControl *c1, SwamiControl *c2,
					   guint flags);
static inline void swami_control_set_event_real (SwamiControl *control,
						 SwamiControlEvent *event);
static inline gboolean swami_control_loop_check (SwamiControl *control,
						 SwamiControlEvent *event);

/* a master list of all controls, used for doing periodic inactive event
   expiration cleanup */
G_LOCK_DEFINE_STATIC (control_list);
static GList *control_list = NULL;

static GObjectClass *parent_class = NULL;
static guint control_signals[SIGNAL_COUNT] = { 0 };

/* debug flag for enabling display of control operations */
#if DEBUG
gboolean swami_control_debug = FALSE;
SwamiControl *swami_control_break = NULL;

#define SWAMI_CONTROL_TEST_BREAK(a, b) \
  if (swami_control_break && (a == swami_control_break || b == swami_control_break)) \
    G_BREAKPOINT ()

/* generate a descriptive control description string, must be freed when
   finished */
static char *
pretty_control (SwamiControl *ctrl)
{
  char *s;

  if (!ctrl) return (g_strdup (""));

  if (SWAMI_IS_CONTROL_FUNC (ctrl))
    {
      SwamiControlFunc *fn = SWAMI_CONTROL_FUNC (ctrl);
      s = g_strdup_printf ("<%s>%p (get=%p, set=%p)", G_OBJECT_TYPE_NAME (fn),
			   fn, fn->get_func, fn->set_func);
    }
  else if (SWAMI_IS_CONTROL_PROP (ctrl))
    {
      SwamiControlProp *pc = SWAMI_CONTROL_PROP (ctrl);
      s = g_strdup_printf ("<%s>%p (object=<%s>%p, property='%s')",
			   G_OBJECT_TYPE_NAME (pc), pc,
			   pc->object ? G_OBJECT_TYPE_NAME (pc->object) : "",
			   pc->object, pc->spec ? pc->spec->name : "");
    }
  else
    s = g_strdup_printf ("<%s>%p", G_OBJECT_TYPE_NAME (ctrl), ctrl);

  return (s);
}
#endif


GType
swami_control_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type) {
    static const GTypeInfo obj_info =
      {
	sizeof (SwamiControlClass), NULL, NULL,
	(GClassInitFunc) swami_control_class_init, NULL, NULL,
	sizeof (SwamiControl), 0,
	(GInstanceInitFunc) swami_control_init
      };

    obj_type = g_type_register_static (SWAMI_TYPE_LOCK, "SwamiControl",
				       &obj_info, 0);
  }

  return (obj_type);
}

static void
swami_control_class_init (SwamiControlClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = swami_control_finalize;

  klass->connect = NULL;
  klass->disconnect = NULL;

  control_signals[CONNECT_SIGNAL] =
    g_signal_new ("connect", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (SwamiControlClass, connect), NULL, NULL,
		  swami_marshal_VOID__OBJECT_UINT, G_TYPE_NONE,
		  2, G_TYPE_OBJECT, G_TYPE_UINT);
  control_signals[DISCONNECT_SIGNAL] =
    g_signal_new ("disconnect", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (SwamiControlClass, disconnect), NULL, NULL,
		  swami_marshal_VOID__OBJECT_UINT, G_TYPE_NONE,
		  2, G_TYPE_OBJECT, G_TYPE_UINT);
  control_signals[SPEC_CHANGED_SIGNAL] =
    g_signal_new ("spec-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
		  g_cclosure_marshal_VOID__PARAM, G_TYPE_NONE, 1,
		  G_TYPE_PARAM);
}

/* creating an instance of SwamiControl creates a send only event control */
static void
swami_control_init (SwamiControl *control)
{
  control->flags = SWAMI_CONTROL_SENDS;

  /* prepend control to master list */
  G_LOCK (control_list);
  control_list = g_list_prepend (control_list, control);
  G_UNLOCK (control_list);
}

static void
swami_control_finalize (GObject *object)
{
  SwamiControl *control = SWAMI_CONTROL (object);
  swami_control_disconnect_all (control);

  G_LOCK (control_list);
  control_list = g_list_remove (control_list, object);
  G_UNLOCK (control_list);
}

/**
 * swami_control_new:
 * 
 * Create a new #SwamiControl instance.  #SwamiControl is the base class for
 * other control types as well.  Creating an instance of a #SwamiControl
 * will create a send only event control.
 * 
 * Returns: New #SwamiControl object, the caller owns a reference.
 */
SwamiControl *
swami_control_new (void)
{
  return (SWAMI_CONTROL (g_object_new (SWAMI_TYPE_CONTROL, NULL)));
}

/**
 * swami_control_connect:
 * @src: Source control to connect (readable)
 * @dest: Destination control to connect (writable)
 * @flags: Flags for this connection (#SwamiControlConnFlags)
 *
 * Connect two controls (i.e., when the @src control's value changes
 * the @dest control is set to this value). Useful flags include the
 * #SWAMI_CONTROL_CONN_INIT flag which will cause @dest to be set to
 * the current value of @src and #SWAMI_CONTROL_CONN_BIDIR which will
 * cause a bi-directional connection to be made (as if 2 calls where
 * made to this function with the @src and @dest swapped the second
 * time). The connection priority can also be set via the flags field
 * by or-ing in #SwamiControlConnPriority values (assumes default
 * priority if not specified). The priority determines the order in
 * which connections are processed.  The #SWAMI_CONTROL_CONN_SPEC flag will
 * cause the parameter spec of the @dest control to be set to that of @src.
 */
void
swami_control_connect (SwamiControl *src, SwamiControl *dest, guint flags)
{
  swami_control_connect_transform (src, dest, flags, NULL, NULL, NULL, NULL,
				   NULL, NULL);
}

/**
 * swami_control_connect_transform:
 * @src: Source control to connect (readable)
 * @dest: Destination control to connect (writable)
 * @flags: Flags for this connection (#SwamiControlConnFlags).
 * @trans1: Value transform function from @src to @dest (or %NULL for no transform)
 * @trans2: Value transform function from @dest to @src
 *   (#SWAMI_CONTROL_CONN_BIDIR only, %NULL for no transform).
 * @data1: User data to pass to @trans1 function.
 * @data2: User data to pass to @trans2 function.
 *   (#SWAMI_CONTROL_CONN_BIDIR only, %NULL for no transform).
 * @destroy1: Optional callback to free @data1 when @trans1 is disconnected.
 * @destroy2: Optional callback to free @data2 when @trans2 is disconnected.
 *
 * Like swami_control_connect() but transform functions can be specified
 * during connect, rather than having to call swami_control_set_transform()
 * later.
 */
void
swami_control_connect_transform (SwamiControl *src, SwamiControl *dest,
				 guint flags,
				 SwamiValueTransform trans1,
				 SwamiValueTransform trans2,
				 gpointer data1, gpointer data2,
				 GDestroyNotify destroy1,
				 GDestroyNotify destroy2)
{
  guint flags2;

  g_return_if_fail (SWAMI_IS_CONTROL (src));
  g_return_if_fail (SWAMI_IS_CONTROL (dest));

  if (flags & SWAMI_CONTROL_CONN_BIDIR)
    {
      swami_control_connect_real (src, dest, trans1, data1, destroy1, flags);

      flags2 = flags & ~(SWAMI_CONTROL_CONN_INIT | SWAMI_CONTROL_CONN_SPEC);
      swami_control_connect_real (dest, src, trans2, data2, destroy2, flags2);
    }
  else swami_control_connect_real (src, dest, trans1, data1, destroy1, flags);

#if DEBUG
  if (swami_control_debug)
    {
      char *s1, *s2;
      s1 = pretty_control (src);
      s2 = pretty_control (dest);
      g_message ("Connect: %s %s %s", s1,
		 (flags & SWAMI_CONTROL_CONN_BIDIR) ? "<-->" : "-->", s2);
      g_free (s1); g_free (s2);

      SWAMI_CONTROL_TEST_BREAK (src, dest);
    }
#endif
}

static void
swami_control_connect_real (SwamiControl *src, SwamiControl *dest,
			    SwamiValueTransform trans,
			    gpointer data, GDestroyNotify destroy, guint flags)
{
  SwamiControlConn *sconn, *dconn;
  GValue value = { 0 }, transval = { 0 };

  /* allocate and init connections */
  sconn = swami_control_conn_new ();
  sconn->flags = (flags & SWAMI_CONTROL_CONN_PRIORITY_MASK)
    | SWAMI_CONTROL_CONN_OUTPUT;
  sconn->control = dest;
  sconn->trans = trans;
  sconn->data = data;
  sconn->destroy = destroy;

  dconn = swami_control_conn_new ();
  dconn->flags = (flags & SWAMI_CONTROL_CONN_PRIORITY_MASK)
    | SWAMI_CONTROL_CONN_INPUT;
  dconn->control = src;

  /* add output connection to source control */
  SWAMI_LOCK_WRITE (src);
  if (swami_log_if_fail (src->flags & SWAMI_CONTROL_SENDS))
    {
      SWAMI_UNLOCK_WRITE (src);
      goto err_src;
    }

  if (g_slist_length (src->outputs) >= MAX_DEST_CONNECTIONS)
    {
      SWAMI_UNLOCK_WRITE (src);
      g_critical ("Maximum number of control connections reached!");
      goto err_src;
    }

  /* add connection to list */
  src->outputs = g_slist_insert_sorted (src->outputs, sconn,
					GCompare_func_conn_priority);
  g_object_ref (dest);	       /* ++ ref dest for source connection */
  SWAMI_UNLOCK_WRITE (src);


  /* add input connection to destination control */
  SWAMI_LOCK_WRITE (dest);
  if (swami_log_if_fail (dest->flags & SWAMI_CONTROL_RECVS))
    {
      SWAMI_UNLOCK_WRITE (dest);
      goto err_dest;
    }
  dest->inputs = g_slist_prepend (dest->inputs, dconn);
  g_object_ref (src);	       /* ++ ref src for destination connection */
  SWAMI_UNLOCK_WRITE (dest);

  /* check if connect parameter spec flag is set for src, and slave the
     parameter spec if so */
  if (flags & SWAMI_CONTROL_CONN_SPEC)
    swami_control_sync_spec (dest, src, trans, data);

  /* initialize destination control from current source value? */
  if (flags & SWAMI_CONTROL_CONN_INIT)
    {
      swami_control_get_value_native (src, &value);

      if (trans)	/* transform function provided? */
	{
	  g_value_init (&transval, dest->value_type ? dest->value_type
			: G_VALUE_TYPE (&value));
	  trans (&value, &transval, data);
	  swami_control_set_value (dest, &transval);
	  g_value_unset (&transval);
	}
      else swami_control_set_value (dest, &value);

      g_value_unset (&value);
    }

  /* emit connect signals */
  g_signal_emit (src, control_signals[CONNECT_SIGNAL], 0, dest,
		 flags | SWAMI_CONTROL_CONN_OUTPUT);
  g_signal_emit (dest, control_signals[CONNECT_SIGNAL], 0, src,
		 flags | SWAMI_CONTROL_CONN_INPUT);
  return;

 err_dest:
  /* error occured after src already connected, undo */
  SWAMI_LOCK_WRITE (src);
  {
    GSList **list, *p, *prev = NULL;

    list = &src->outputs;
    p = *list;
    while (p)
      {
	if (p->data == sconn)
	  {
	    if (!prev) *list = p->next;
	    else prev->next = p->next;
	    g_slist_free_1 (p);
	    g_object_unref (dest); /* -- unref dest from source connection */
	    break;
	  }
	prev = p;
	p = g_slist_next (p);
      }
  }
  SWAMI_UNLOCK_WRITE (src);
  /* fall through */

 err_src:
  /* OK to free connections unlocked since they aren't used anymore */
  swami_control_conn_free (sconn);
  swami_control_conn_free (dconn);
}

/* a priority comparison function for lists of #SwamiControlConn objects */
static gint
GCompare_func_conn_priority (gconstpointer a, gconstpointer b)
{
  SwamiControlConn *aconn = (SwamiControlConn *)a;
  SwamiControlConn *bconn = (SwamiControlConn *)b;
  return ((aconn->flags & SWAMI_CONTROL_CONN_PRIORITY_MASK)
	  - (bconn->flags & SWAMI_CONTROL_CONN_PRIORITY_MASK));
}

/**
 * swami_control_connect_item_prop:
 * @dest: Destination control
 * @object: Object of source parameter to convert to user units
 * @pspec: Parameter spec of property of @object to connect to
 *
 * An ultra-convenience function to connect an existing control to a synthesis
 * property of an object (#IpatchItem property for example).  The connection is
 * bi-directional and transform functions are used to convert between the two
 * unit types as needed.
 */
void
swami_control_connect_item_prop (SwamiControl *dest, GObject *object,
				 GParamSpec *pspec)
{
  SwamiControl *src;
  gpointer data1, data2;
  guint src_unit, dest_unit;
  IpatchUnitInfo *info;
  GParamSpec *destspec;

  g_return_if_fail (SWAMI_IS_CONTROL (dest));
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  /* get/create control for source item synthesis parameter */
  src = swami_get_control_prop (object, pspec);		/* ++ ref */
  g_return_if_fail (src != NULL);

  /* get the synthesis unit type for this parameter */
  ipatch_param_get (pspec, "unit-type", &src_unit, NULL);

  if (swami_log_if_fail (src_unit != 0))	/* error if no unit type */
    {
      g_object_unref (src);	/* -- unref */
      return;
    }

  /* get the user unit type to convert to */
  info = ipatch_unit_class_lookup_map (IPATCH_UNIT_CLASS_USER, src_unit);

  if (info)	/* use the user unit type if found and not the same as src type */
    {
      dest_unit = info->id;
      if (src_unit == dest_unit) dest_unit = 0;
    }
  else dest_unit = 0;

  if (dest_unit)
    { /* pass unit types to item_prop_value_transform */
      data1 = GUINT_TO_POINTER (src_unit | (dest_unit << 16));
      data2 = GUINT_TO_POINTER (dest_unit | (src_unit << 16));

      /* transform the parameter spec if necessary */
      destspec = swami_control_transform_spec (dest, src,  /* !! floating ref */
					       item_prop_value_transform, data1);
      g_return_if_fail (destspec != NULL);

      ipatch_param_set (destspec, "unit-type", dest_unit, NULL);
      swami_control_set_spec (dest, destspec);

      swami_control_connect_transform (src, dest, SWAMI_CONTROL_CONN_BIDIR_INIT,
				       item_prop_value_transform,
				       item_prop_value_transform,
				       data1, data2, NULL, NULL);
    }
  else
    swami_control_connect_transform (src, dest, SWAMI_CONTROL_CONN_BIDIR_SPEC_INIT,
				     NULL, NULL, NULL, NULL, NULL, NULL);
}

/* value transform function for swami_control_connect_item_prop() */
static void
item_prop_value_transform (const GValue *src, GValue *dest, gpointer data)
{
  guint src_unit, dest_unit;

  src_unit = GPOINTER_TO_UINT (data);
  dest_unit = src_unit >> 16;
  src_unit &= 0xFFFF;

  /* do the unit conversion */
  ipatch_unit_convert (src_unit, dest_unit, src, dest);
}

/**
 * swami_control_disconnect:
 * @src: Source control of an existing connection
 * @dest: Destination control of an existing connection
 *
 * Disconnects a connection specified by its @src and @dest controls.
 */
void
swami_control_disconnect (SwamiControl *src, SwamiControl *dest)
{
  guint flags = 0;
  GSList *p;

  g_return_if_fail (SWAMI_IS_CONTROL (src));
  g_return_if_fail (SWAMI_IS_CONTROL (dest));

  /* check and see if connection exists */
  SWAMI_LOCK_READ (dest);
  p = dest->inputs; /* use single dest input list to simplify things */
  while (p)			/* look for matching connection */
    {
      SwamiControlConn *conn = (SwamiControlConn *)(p->data);
      if (src == conn->control)
	{
	  flags = conn->flags;	/* get flags under lock */
	  break;
	}
      p = g_slist_next (p);
    }
  SWAMI_UNLOCK_READ (dest);

  /* adjust flags for src control connection */
  flags &= ~SWAMI_CONTROL_CONN_INPUT;
  flags |= SWAMI_CONTROL_CONN_OUTPUT;

  if (p)	      /* connection found? - emit disconnect signal */
    {
      g_signal_emit (src, control_signals[DISCONNECT_SIGNAL], 0, dest, flags); 
      swami_control_real_disconnect (src, dest, flags);
   }
}

/**
 * swami_control_disconnect_all:
 * @control: Swami control object
 *
 * Disconnect all connections from a control.
 */
void
swami_control_disconnect_all (SwamiControl *control)
{
  SwamiControl *src, *dest = NULL;
  SwamiControlConn *conn;
  guint flags = 0;

  g_return_if_fail (SWAMI_IS_CONTROL (control));

  do
    {
      /* look for a connection to disconnect */
      SWAMI_LOCK_READ (control);

      if (control->inputs)	/* any more input connections? */
	{
	  conn = (SwamiControlConn *)(control->inputs->data);
	  src = g_object_ref (conn->control); /* ++ ref connection source */
	  dest = control;
	  flags = conn->flags;
	}
      else if (control->outputs) /* any more direct output connections? */
	{
	  conn = (SwamiControlConn *)(control->outputs->data);
	  dest = g_object_ref (conn->control); /* ++ ref connection dest */
	  src = control;
	  flags = conn->flags;
	}
      else src = NULL;		/* no more connections */

      SWAMI_UNLOCK_READ (control);

      if (src)
	{
	  /* adjust flags for src control connection */
	  flags &= ~SWAMI_CONTROL_CONN_INPUT;
	  flags |= SWAMI_CONTROL_CONN_OUTPUT;

	  g_signal_emit (src, control_signals[DISCONNECT_SIGNAL], 0,
			 dest, flags);
	  swami_control_real_disconnect (src, dest, flags);

	  /* -- unref connection control */
	  if (control != src) g_object_unref (src);
	  else g_object_unref (dest);
	}
    }
  while (src);
}

/**
 * swami_control_disconnect_unref:
 * @control: Swami control object
 * 
 * A convenience function to disconnect all connections of a control and
 * unref it. This is often useful for GDestroyNotify callbacks where a
 * control's creator wishes to destroy the control. The control is only
 * destroyed if it is not referenced by anything else.
 */
void
swami_control_disconnect_unref (SwamiControl *control)
{
  g_return_if_fail (SWAMI_IS_CONTROL (control));
  swami_control_disconnect_all (control);
  g_object_unref (control);
}

/* real disconnect routine, the default class method */
static void
swami_control_real_disconnect (SwamiControl *c1, SwamiControl *c2, guint flags)
{
  GSList **list, *p, *prev = NULL;
  GDestroyNotify destroy = NULL;
  gpointer data = NULL;
  SwamiControlConn *conn;

  SWAMI_LOCK_WRITE (c1);

  if (flags & SWAMI_CONTROL_CONN_OUTPUT) /* output conn (source control)? */
    list = &c1->outputs; /* use output connection list */
  else list = &c1->inputs;	/* destination control - use input conn list */

  p = *list;
  while (p)			/* search for connection in list */
    {
      conn = (SwamiControlConn *)(p->data);
      if (c2 == conn->control)	/* connection found? destroy it */
	{
	  if (prev) prev->next = p->next;
	  else *list = p->next;
	  g_slist_free_1 (p);
	  g_object_unref (conn->control); /* -- unref control from conn */

	  /* store destroy notify if output conn */
	  if (flags & SWAMI_CONTROL_CONN_OUTPUT)
	    {
	      destroy = conn->destroy;
	      data = conn->data;
	    }

	  swami_control_conn_free (conn); /* free the connection */
	  break;
	}
      prev = p;
      p = g_slist_next (p);
    }
  SWAMI_UNLOCK_WRITE (c1);

  /* call the destroy notify for the transform user data if any */
  if (destroy && data) destroy (data);

  /* chain disconnect signal to destination control (if source control) */
  if (flags & SWAMI_CONTROL_CONN_OUTPUT)
    {

#if DEBUG
      if (swami_control_debug)
	{
	  char *s1, *s2;
	  s1 = pretty_control (c1);
	  s2 = pretty_control (c2);
	  g_message ("Disconnect: %s =X= %s", s1, s2);
	  g_free (s1); g_free (s2);
	}

      SWAMI_CONTROL_TEST_BREAK (c1, c2);
#endif

      /* adjust flags for input connection (destination control) */
      flags &= ~SWAMI_CONTROL_CONN_OUTPUT;
      flags |= SWAMI_CONTROL_CONN_INPUT;
      g_signal_emit (c2, control_signals[DISCONNECT_SIGNAL], 0, c1, flags);
      swami_control_real_disconnect (c2, c1, flags);
    }
}

/**
 * swami_control_get_connections:
 * @control: Control to get connections of
 * @dir: Direction of connections to get, #SWAMI_CONTROL_CONN_INPUT for incoming
 *   connections, #SWAMI_CONTROL_CONN_OUTPUT for outgoing (values can be OR'd
 *   to return all connections).
 *
 * Get a list of connections to a control.
 * 
 * Returns: List of #SwamiControl objects connected to @control or %NULL if
 *   none. Caller owns reference to returned list and should unref when done.
 */
IpatchList *
swami_control_get_connections (SwamiControl *control, SwamiControlConnFlags dir)
{
  SwamiControlConn *conn;
  IpatchList *listobj;
  GList *list = NULL;
  GSList *p;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), NULL);

  SWAMI_LOCK_READ (control);

  if (dir & SWAMI_CONTROL_CONN_INPUT)
    {
      for (p = control->inputs; p; p = g_slist_next (p))
	{
	  conn = (SwamiControlConn *)(p->data);
	  list = g_list_prepend (list, g_object_ref (conn->control));
	}
    }

  if (dir & SWAMI_CONTROL_CONN_OUTPUT)
    {
      for (p = control->outputs; p; p = g_slist_next (p))
	{
	  conn = (SwamiControlConn *)(p->data);
	  list = g_list_prepend (list, g_object_ref (conn->control));
	}
    }

  SWAMI_UNLOCK_READ (control);

  if (list)
    {
      listobj = ipatch_list_new (); /* ++ ref new list object */
      listobj->items = g_list_reverse (list); /* reverse to priority order */
      return (listobj);		/* !! caller takes over reference */
    }
  else return (NULL);
}

/**
 * swami_control_set_transform:
 * @src: Source control of the connection
 * @dest: Destination control of the connection
 * @trans: Transform function to assign to the connection (or %NULL for none)
 * @data: User data to pass to @trans function (or %NULL).
 * @destroy: Optional callback to free @data when @trans is disconnected.
 *
 * Assigns a value transform function to an existing control connection.  The
 * connection is specified by @src and @dest controls.  The transform function
 * will receive values from the @src control and should transform them
 * appropriately for the @dest.
 */
void
swami_control_set_transform (SwamiControl *src, SwamiControl *dest,
			     SwamiValueTransform trans, gpointer data,
			     GDestroyNotify destroy)
{
  gboolean conn_found = FALSE;
  GDestroyNotify oldnotify = NULL;
  gpointer olddata = NULL;
  GSList *p;

  g_return_if_fail (SWAMI_IS_CONTROL (src));
  g_return_if_fail (SWAMI_IS_CONTROL (dest));

  SWAMI_LOCK_READ (src);

  /* look for matching connection */
  for (p = src->outputs; p; p = p->next)
    {
      SwamiControlConn *conn = (SwamiControlConn *)(p->data);

      if (dest == conn->control)
	{
	  oldnotify = conn->destroy;
	  olddata = conn->data;

	  conn->trans = trans;
	  conn->data = data;
	  conn->destroy = destroy;
	  conn_found = TRUE;
	  break;
	}
    }

  SWAMI_UNLOCK_READ (src);

  /* if there already was a transform with destroy function, call it on the user data */
  if (oldnotify && olddata) oldnotify (olddata);

  g_return_if_fail (conn_found);
}

/**
 * swami_control_get_transform:
 * @src: Source control of the connection
 * @dest: Destination control of the connection
 * @trans: Pointer to store transform function of the connection
 *
 * Gets the current value transform function to an existing control connection.
 * The connection is specified by @src and @dest controls.  The value stored to
 * @trans may be %NULL if no transform function is assigned.
 */
void
swami_control_get_transform (SwamiControl *src, SwamiControl *dest,
			     SwamiValueTransform *trans)
{
  gboolean conn_found = FALSE;
  GSList *p;

  g_return_if_fail (SWAMI_IS_CONTROL (src));
  g_return_if_fail (SWAMI_IS_CONTROL (dest));
  g_return_if_fail (trans != NULL);

  SWAMI_LOCK_READ (src);

  /* look for matching connection */
  for (p = src->outputs; p; p = p->next)
    {
      SwamiControlConn *conn = (SwamiControlConn *)(p->data);

      if (dest == conn->control)
	{
	  *trans = conn->trans;
	  conn_found = TRUE;
	  break;
	}
    }

  SWAMI_UNLOCK_READ (src);

  g_return_if_fail (conn_found);
}

/**
 * swami_control_set_flags:
 * @control: Control object
 * @flags: Value to assign to control flags (#SwamiControlFlags)
 *
 * Set flags of a control object. Flags can only be set for controls
 * that don't have any connections yet.
 */
void
swami_control_set_flags (SwamiControl *control, int flags)
{
  g_return_if_fail (SWAMI_IS_CONTROL (control));

  SWAMI_LOCK_WRITE (control);

  if (swami_log_if_fail (!(control->inputs || control->outputs)))
    {
      SWAMI_UNLOCK_WRITE (control);
      return;
    }

  flags &= SWAMI_CONTROL_FLAGS_USER_MASK;
  control->flags &= ~SWAMI_CONTROL_FLAGS_USER_MASK;
  control->flags |= flags;

  SWAMI_UNLOCK_WRITE (control);
}

/**
 * swami_control_get_flags:
 * @control: Control object
 * 
 * Get the flags from a control object.
 * 
 * Returns: Flags value (#SwamiControlFlags) for @control
 */
int
swami_control_get_flags (SwamiControl *control)
{
  int flags;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), 0);

  SWAMI_LOCK_READ (control);
  flags = control->flags;
  SWAMI_UNLOCK_READ (control);

  return (flags);
}

/**
 * swami_control_set_queue:
 * @control: Control object
 * @queue: Queue to use for control or %NULL to disable queuing
 *
 * Set the queue used by a control object. When a control has a queue all
 * receive/set events are sent to the queue. This queue can then be processed
 * at a later time (a GUI thread for example).
 */
void
swami_control_set_queue (SwamiControl *control, SwamiControlQueue *queue)
{
  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (!queue || SWAMI_IS_CONTROL_QUEUE (queue));

  SWAMI_LOCK_WRITE (control);
  if (queue) g_object_ref (queue); /* ++ ref new queue */
  if (control->queue) g_object_unref (control->queue); /* -- unref old queue */
  control->queue = queue;
  SWAMI_UNLOCK_WRITE (control);
}

/**
 * swami_control_get_queue:
 * @control: Control object
 *
 * Get the queue used by a control object.
 *
 * Returns: The queue assigned to a control or %NULL if it does not have one.
 *   The caller owns a reference to the returned queue object.
 */
SwamiControlQueue *
swami_control_get_queue (SwamiControl *control)
{
  SwamiControlQueue *queue = NULL;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), NULL);

  SWAMI_LOCK_READ (control);
  if (control->queue) queue = g_object_ref (control->queue); /* ++ ref queue */
  SWAMI_UNLOCK_WRITE (control);

  return (queue);		/* !! caller takes over reference */
}

/**
 * swami_control_get_spec:
 * @control: Control object
 *
 * Get a control object's parameter specification.
 *
 * Returns: The parameter spec or %NULL on error or if the given
 * control is type independent. The returned spec is used internally
 * and should not be modified or freed, however the caller does own a
 * reference to it and should unref it with g_param_spec_unref when
 * finished.
 */
GParamSpec *
swami_control_get_spec (SwamiControl *control)
{
  SwamiControlClass *klass;
  GParamSpec *pspec = NULL;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), NULL);

  klass = SWAMI_CONTROL_GET_CLASS (control);
  g_return_val_if_fail (klass->get_spec != NULL, NULL);

  SWAMI_LOCK_READ (control);
  if (klass->get_spec)
    pspec = (*klass->get_spec)(control);
  if (pspec) g_param_spec_ref (pspec);
  SWAMI_UNLOCK_READ (control);

  return (pspec);
}

/**
 * swami_control_set_spec:
 * @control: Control object
 * @pspec: The parameter specification to assign (used directly)
 *
 * Set a control object's parameter specification. This function uses
 * the @pspec directly (its refcount is incremented and ownership is
 * taken). The parameter spec is not permitted to be of a different
 * type than the previous parameter spec.
 *
 * Returns: %TRUE if parameter specification successfully set, %FALSE
 *   otherwise (in which case the @pspec is unreferenced).
 */
gboolean
swami_control_set_spec (SwamiControl *control, GParamSpec *pspec)
{
  SwamiControlClass *klass;
  GParamSpec *newspec;
  GType value_type;
  gboolean retval;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);

  klass = SWAMI_CONTROL_GET_CLASS (control);
  g_return_val_if_fail (klass->get_spec != NULL, FALSE);
  g_return_val_if_fail (klass->set_spec != NULL, FALSE);

  value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

  /* use derived type if GBoxed or GObject type */
  if (value_type == G_TYPE_BOXED || value_type == G_TYPE_OBJECT)
    value_type = pspec->value_type;

  /* if control's value type doesn't match the param spec value type and
     "no conversion" flag isn't set, then convert parameter spec */
  if (control->value_type && control->value_type != value_type
      && !(control->flags & SWAMI_CONTROL_SPEC_NO_CONV))
    {
      newspec = swami_param_convert_new (pspec, control->value_type);

      /* take control of the old pspec and then destroy it */
      g_param_spec_ref (pspec);
      g_param_spec_sink (pspec);
      g_param_spec_unref (pspec);

      if (!newspec) return (FALSE);

      pspec = newspec;
    }

  SWAMI_LOCK_WRITE (control);
  retval = (*klass->set_spec)(control, pspec);
  SWAMI_UNLOCK_WRITE (control);

  if (retval)
    g_signal_emit (control, control_signals[SPEC_CHANGED_SIGNAL], 0, pspec);

  return (retval);
}

/**
 * swami_control_set_value_type:
 * @control: Control to set value type of
 * @type: Value type to assign to control
 *
 * Usually only called by #SwamiControl derived types from within
 * their instance init function. Sets the parameter spec value type
 * for @control and should be called once for specific type value
 * based controls before being used and cannot be changed
 * afterwards. If the type is not set then the control is a wild card
 * control (can handle any value). For GBoxed and GObject based values
 * use the specific GType for @type.
 */
void
swami_control_set_value_type (SwamiControl *control, GType type)
{
  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (type != 0);

  SWAMI_LOCK_WRITE (control);

  /* make sure type is not already set */
  if (control->value_type)
    {
      if (swami_log_if_fail (control->value_type == type))
	{
	  SWAMI_UNLOCK_WRITE (control);
	  return;
	}
    }
  else control->value_type = type;

  SWAMI_UNLOCK_WRITE (control);
}

/**
 * swami_control_sync_spec:
 * @control: Control to set parameter spec control of
 * @source: The source control to get the parameter spec from
 * @trans: Optional value transform function
 * @data: User data to pass to transform function
 *
 * Synchronizes a @control object's parameter spec to the @source control.
 * The @source control must already have an assigned parameter spec and the
 * @control should have an assigned value type.  The optional @trans parameter
 * can be used to specify a transform function, which is executed on the
 * min, max and default components of the parameter spec when creating the new
 * param spec.
 *
 * Returns: %TRUE on success, %FALSE otherwise (param spec conversion error)
 */
gboolean
swami_control_sync_spec (SwamiControl *control, SwamiControl *source,
			 SwamiValueTransform trans, gpointer data)
{
  GParamSpec *pspec;
  int retval;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), FALSE);
  g_return_val_if_fail (SWAMI_IS_CONTROL (source), FALSE);

  if (trans)	/* !! floating reference */
    pspec = swami_control_transform_spec (control, source, trans, data);
  else pspec = swami_control_get_spec (source);	/* ++ ref spec */

  if (!pspec)
  {
    g_debug ("pspec == NULL");
    return (FALSE);
  }

  /* set the param spec for the control */
  retval = swami_control_set_spec (control, pspec);

  /* only unref if swami_control_get_spec() was used, was floating otherwise */
  if (!trans) g_param_spec_unref (pspec);		/* -- unref */

  return (retval);
}

/**
 * swami_control_transform_spec:
 * @control: Destination control for the transformed parameter spec
 * @source: Source control with the parameter spec to transform
 * @trans: Transform function
 * @data: User defined data to pass to transform function
 *
 * Transforms a parameter spec from a @source control in preperation for
 * assignment to @control (but doesn't actually assign it).
 *
 * Returns: Transformed parameter spec or %NULL on error, the reference count
 *   is 1 and floating, which means it should be taken over by calling
 *   g_param_spec_ref() followed by g_param_spec_sink().
 */
GParamSpec *
swami_control_transform_spec (SwamiControl *control, SwamiControl *source,
			      SwamiValueTransform trans, gpointer data)
{
  GParamSpec *srcspec, *transform_spec;
  GType type;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), NULL);
  g_return_val_if_fail (SWAMI_IS_CONTROL (source), NULL);
  g_return_val_if_fail (trans != NULL, NULL);

  /* get the master control parameter spec */
  srcspec = swami_control_get_spec (source); /* ++ ref spec */
  g_return_val_if_fail (srcspec != NULL, NULL);

  type = control->value_type ? control->value_type
    : G_PARAM_SPEC_VALUE_TYPE (srcspec);

  /* !! floating ref, transform the parameter spec */
  transform_spec = swami_param_transform_new (srcspec, type, trans, data);
  g_param_spec_unref (srcspec);		/* -- unref */

  g_return_val_if_fail (transform_spec != NULL, NULL);

  return (transform_spec);	/* !! caller takes over floating reference */
}

/**
 * swami_control_get_value:
 * @control: Control object of type #SWAMI_CONTROL_VALUE
 * @value: Caller supplied initialized GValue structure to store the value in.
 *
 * Get the current value of a value control object. The @control must
 * have the #SWAMI_CONTROL_SENDS flag set and be a value type control
 * with an assigned parameter spec. The @value parameter should be
 * initialized to the type of the control or a transformable type.
 */
void
swami_control_get_value (SwamiControl *control, GValue *value)
{
  SwamiControlClass *klass;
  GValue *get_value, tmp_value = { 0 };

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (G_IS_VALUE (value));

  klass = SWAMI_CONTROL_GET_CLASS (control);

  /* some sanity checking */
  g_return_if_fail (klass->get_value != NULL);
  g_return_if_fail (control->flags & SWAMI_CONTROL_SENDS);
  g_return_if_fail (control->value_type != 0);

  if (G_VALUE_HOLDS (value, control->value_type))
    {			  /* same type, just reset value and use it */
      g_value_reset (value);
      get_value = value;
    }
  else if (!g_value_type_transformable (control->value_type,
					G_VALUE_TYPE (value)))
    {
      g_critical ("%s: Failed to transform value type '%s' to type '%s'",
		  G_STRLOC, g_type_name (control->value_type),
		  g_type_name (G_VALUE_TYPE (value)));
      return;
    }
  else	/* @value is not the same type, but is transformable */
    {
      g_value_init (&tmp_value, control->value_type);
      get_value = &tmp_value;
    }

  /* get_value method responsible for locking, if needed */
  (*klass->get_value)(control, get_value);

  if (get_value == &tmp_value)	/* transform the value if needed */
    {
      g_value_transform (get_value, value);
      g_value_unset (&tmp_value);
    }
}

/**
 * swami_control_get_value_native:
 * @control: Control object of type #SWAMI_CONTROL_VALUE
 * @value: Caller supplied uninitalized GValue structure to store the value in.
 *
 * Like swami_control_get_value() but forces @value to be the native type of
 * the control, rather than transforming the value to the initialized @value
 * type. Therefore @value should be un-initialized, contrary to
 * swami_control_get_value().
 */
void
swami_control_get_value_native (SwamiControl *control, GValue *value)
{
  SwamiControlClass *klass;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (value != NULL);

  klass = SWAMI_CONTROL_GET_CLASS (control);

  /* some sanity checking */
  g_return_if_fail (klass->get_value != NULL);
  g_return_if_fail (control->flags & SWAMI_CONTROL_SENDS);
  g_return_if_fail (control->value_type != 0);

  g_value_init (value, control->value_type);

  /* get_value method responsible for locking, if needed */
  (*klass->get_value)(control, value);
}

/**
 * swami_control_set_value:
 * @control: Control object
 * @value: Value to set control to
 *
 * Sets/sends a value to a control object. If the control has a queue
 * assigned to it then the value is queued. The @value parameter
 * should be of a type transformable to the type used by @control (see
 * g_value_transformable). The @control must have the
 * #SWAMI_CONTROL_RECVS flag set.
 */
void
swami_control_set_value (SwamiControl *control, const GValue *value)
{
  SwamiControlEvent *event;
  SwamiControlQueue *queue;
  SwamiControlQueueTestFunc test_func;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (G_IS_VALUE (value));

  event = swami_control_new_event (control, NULL, value); /* ++ ref new */

  swami_control_event_active_ref (event); /* ++ active ref the event */
  swami_control_event_ref (event); /* ++ ref event for control active list */

  /* prepend the event to the active list */
  SWAMI_LOCK_WRITE (control);
  control->active = g_list_prepend (control->active, event);
  SWAMI_UNLOCK_WRITE (control);

  queue = swami_control_get_queue (control); /* ++ ref queue */
  if (queue)	   /* if queue, then add event to the queue */
    { /* run queue test function (if any) */
      test_func = queue->test_func; /* should be atomic */
      if (!test_func || test_func (queue, control, event))
	{
	  swami_control_queue_add_event (queue, control, event);
	  g_object_unref (queue);	/* -- unref queue */
	  swami_control_event_active_unref (event); /* -- active unref event */
	  swami_control_event_unref (event); /* -- unref creator's ref */
	  return;
	}

      /* queue has a test function and it returned FALSE (no queue) */

      g_object_unref (queue);	/* -- unref queue */
    }

  swami_control_set_event_real (control, event);

  swami_control_event_active_unref (event); /* -- active unref the event */
  swami_control_event_unref (event); /* -- unref creator's ref */
}

/**
 * swami_control_set_value_no_queue:
 * @control: Control object
 * @value: Value to set control to
 *
 * Sets/sends a value to a control object bypassing the control's
 * queue if it has one assigned to it. The @value parameter should be
 * of a type transformable to the type used by @control (see
 * g_value_transformable). The @control must have the
 * #SWAMI_CONTROL_RECVS flag set. Normally the swami_control_set_value()
 * function should be used instead to send a value to a control, use this
 * only if you know what you are doing since some controls are sensitive to
 * when they are processed (within a GUI thread for instance).
 */
void
swami_control_set_value_no_queue (SwamiControl *control, const GValue *value)
{
  SwamiControlEvent *event;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (G_IS_VALUE (value));

  event = swami_control_new_event (control, NULL, value); /* ++ ref new */

  swami_control_event_active_ref (event); /* ++ active ref the event */
  swami_control_event_ref (event); /* ++ ref event for control active list */

  /* prepend the event to the active list */
  SWAMI_LOCK_WRITE (control);
  control->active = g_list_prepend (control->active, event);
  SWAMI_UNLOCK_WRITE (control);

  swami_control_set_event_real (control, event);

  swami_control_event_active_unref (event); /* -- active unref the event */
  swami_control_event_unref (event); /* -- unref creator's ref */
}

/**
 * swami_control_set_event:
 * @control: Control object
 * @event: Event to set control to
 *
 * Sets the value of a control object (value controls) or sends an
 * event (stream controls). This is like swami_control_set_value() but
 * uses an existing event, rather than creating a new one. The
 * @control must have the #SWAMI_CONTROL_RECVS flag set. If the
 * control has a queue then the event will be added to the queue.
 */
void
swami_control_set_event (SwamiControl *control, SwamiControlEvent *event)
{
  SwamiControlEvent *origin;
  SwamiControlQueue *queue;
  SwamiControlQueueTestFunc test_func;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);

  origin = event->origin ? event->origin : event;
  swami_control_event_active_ref (event); /* ++ active ref the event */

  SWAMI_LOCK_WRITE (control);

  /* check for event looping (only if control can send) */
  if (!swami_control_loop_check (control, event))
    {
      SWAMI_UNLOCK_WRITE (control);
      swami_control_event_active_unref (event); /* -- decrement active ref */
      return;
    }

  /* prepend the event origin to the active list */
  control->active = g_list_prepend (control->active, origin);

  SWAMI_UNLOCK_WRITE (control);

  swami_control_event_ref (origin); /* ++ ref event for control active list */

  queue = swami_control_get_queue (control); /* ++ ref queue */
  if (queue)	   /* if queue, then add event to the queue */
    { /* run queue test function (if any) */
      test_func = queue->test_func; /* should be atomic */
      if (!test_func || test_func (queue, control, event))
	{
	  swami_control_queue_add_event (queue, control, event);
	  g_object_unref (queue);	/* -- unref queue */
	  swami_control_event_active_unref (event); /* -- active unref */
	  return;
	}

      /* queue has a test function and it returned FALSE (no queue) */
      g_object_unref (queue);	/* -- unref queue */
    }

  swami_control_set_event_real (control, event);

  swami_control_event_active_unref (event); /* -- decrement active ref */
}

/**
 * swami_control_set_event_no_queue:
 * @control: Control object
 * @event: Event to set control to
 *
 * Like swami_control_set_event() but bypasses any queue that a
 * control might have. The @control must have the #SWAMI_CONTROL_RECVS
 * flag set.
 */
void
swami_control_set_event_no_queue (SwamiControl *control,
				  SwamiControlEvent *event)
{
  SwamiControlEvent *origin;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->active > 0);

  origin = event->origin ? event->origin : event;
  swami_control_event_active_ref (event); /* ++ active ref the event */

  SWAMI_LOCK_WRITE (control);

  /* check for event looping (only if control can send) */
  if (!swami_control_loop_check (control, event))
    {
      SWAMI_UNLOCK_WRITE (control);
      swami_control_event_active_unref (event); /* -- decrement active ref */
      return;
    }

  /* prepend the event origin to the active list */
  control->active = g_list_prepend (control->active, origin);

  SWAMI_UNLOCK_WRITE (control);

  swami_control_event_ref (origin); /* ++ ref event for control active list */

  swami_control_set_event_real (control, event);

  swami_control_event_active_unref (event); /* -- decrement active ref */
}

/**
 * swami_control_set_event_no_queue_loop:
 * @control: Control object
 * @event: Event to set control to
 *
 * Like swami_control_set_event_no_queue() but doesn't do an event
 * loop check. This function is usually only used by
 * #SwamiControlQueue objects.  The @control must have the
 * #SWAMI_CONTROL_RECVS flag set.
 */
void
swami_control_set_event_no_queue_loop (SwamiControl *control,
				       SwamiControlEvent *event)
{
  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);
  g_return_if_fail (event->active > 0);

  swami_control_event_active_ref (event); /* ++ active ref the event */
  swami_control_set_event_real (control, event);
  swami_control_event_active_unref (event); /* -- decrement active ref */
}

/* the real set event routine */
static inline void
swami_control_set_event_real (SwamiControl *control, SwamiControlEvent *event)
{
  SwamiControlClass *klass;
  GValue temp = { 0 }, *value;

  klass = SWAMI_CONTROL_GET_CLASS (control);

  /* some sanity checking */
  g_return_if_fail (klass->set_value != NULL);
  g_return_if_fail (control->flags & SWAMI_CONTROL_RECVS);

  /* parameter conversion or specific type required? */
  if (klass->get_spec && control->value_type != 0
      && (!(control->flags & SWAMI_CONTROL_NO_CONV)
	  || (control->flags & SWAMI_CONTROL_NATIVE)))
    {
      value = &event->value;

      if (control->flags & SWAMI_CONTROL_NATIVE) /* native type only? */
	{
	  if (!G_VALUE_HOLDS (value, control->value_type))
	    {
	      g_critical ("%s: Control requires value type '%s' got '%s'",
			  G_STRLOC, g_type_name (control->value_type),
			  g_type_name (G_VALUE_TYPE (value)));
	      return;
	    }
	} /* transform the value if needed */
      else if (!G_VALUE_HOLDS (value, control->value_type))
	{
	  g_value_init (&temp, control->value_type);
	  if (!g_value_transform (value, &temp))
	    {
	      g_value_unset (&temp);

	      /* FIXME - probably should just set a flag or inc a counter */

	      g_critical ("%s: Failed to transform value type '%s' to"
			  " type '%s'", G_STRLOC,
			  g_type_name (G_VALUE_TYPE (value)),
			  g_type_name (control->value_type));
	      return;
	    }
	  value = &temp;
	}
    }
  else value = &event->value; /* No conversion necessary */

#if DEBUG
  if (swami_control_debug)
    {
      char *valstr = g_strdup_value_contents (value);
      char *s1 = pretty_control (control);
      g_message ("Set: %s EV:%p ORIGIN:%p VAL:<%s>='%s'",
		 s1, event, event->origin, G_VALUE_TYPE_NAME (value), valstr);
      g_free (s1);
      g_free (valstr);

      SWAMI_CONTROL_TEST_BREAK (control, NULL);
    }
#endif

  /* set_value method is responsible for locking, if needed */
  (*klass->set_value)(control, event, value);

  if (value == &temp) g_value_unset (&temp);
}

/* Check if an event is already visited a control. Also purges old active
   events. Control must be locked by caller.
   Returns: TRUE if not looped, FALSE otherwise */
static inline gboolean
swami_control_loop_check (SwamiControl *control, SwamiControlEvent *event)
{
  SwamiControlEvent *ev, *origin;
  GList *p, *temp;

  /* if control only sends or only receives, don't do loop check.
   * FIXME - Is that right? */
  if ((control->flags & SWAMI_CONTROL_SENDRECV) != SWAMI_CONTROL_SENDRECV)
    return (TRUE);

  origin = event->origin ? event->origin : event;

  /* look through active events to stop loops and to cleanup old entries */
  p = control->active;
  while (p)
    {
      ev = (SwamiControlEvent *)(p->data);
      if (ev == origin)		/* event loop catch */
	{

#if DEBUG
	  if (swami_control_debug)
	    {
	      char *s1 = pretty_control (control);
	      g_message ("Loop killer: %s EV:%p ORIGIN:%p", s1, event, origin);
	      g_free (s1);
	    }

	  SWAMI_CONTROL_TEST_BREAK (control, NULL);
#endif

	  return (FALSE);	/* return immediately, looped */
	}

      if (!ev->active)		/* event still active? */
	{	/* no, remove from list */
	  temp = p;
	  p = g_list_next (p);
	  control->active = g_list_delete_link (control->active, temp);
	  swami_control_event_unref (ev); /* -- unref inactive event */
	}
      else p = g_list_next (p);
    }

  return (TRUE);
}

/**
 * swami_control_transmit_value:
 * @control: Control object
 * @value: Value to transmit or %NULL (readable value controls only, sends
 *    value changed event).
 *
 * Sends a value to all output connections of @control. Usually only
 * used by #SwamiControl object implementations. The @value is used if
 * not %NULL or the control's value is used (readable value controls
 * only). This is a convenience function that creates a
 * #SwamiControlEvent based on the new transmit value, use
 * swami_control_transmit_event() to send an existing event.
 */
void
swami_control_transmit_value (SwamiControl *control, const GValue *value)
{
  CtrlUpdateBag update_ctrls[MAX_DEST_CONNECTIONS + 1];
  CtrlUpdateBag *bag;
  SwamiControlEvent *event, *transevent;
  SwamiControlConn *conn;
  GSList *p;
  int uc = 0;

  g_return_if_fail (SWAMI_IS_CONTROL (control));

  event = swami_control_new_event (control, NULL, value); /* ++ ref new */

  swami_control_event_active_ref (event); /* ++ active ref event */
  swami_control_event_ref (event); /* ++ ref event for control active list */

  SWAMI_LOCK_WRITE (control);

  /* prepend the event origin to the active list */
  control->active = g_list_prepend (control->active, event);

  /* copy destination controls to an array under lock, which is then used
     outside of lock to avoid recursive dead locks */

  /* loop over destination connections */
  for (p = control->outputs; p; p = p->next)
    {
      conn = (SwamiControlConn *)(p->data);

      /* ++ ref dest control and add to array, copy transform func also */
      update_ctrls[uc].control = g_object_ref (conn->control);
      update_ctrls[uc].trans = conn->trans;
      update_ctrls[uc++].data = conn->data;
    }

  update_ctrls[uc].control = NULL;

  SWAMI_UNLOCK_WRITE (control);
  
#if DEBUG
  if (swami_control_debug)
    {
      char *s1 = pretty_control (control);
      g_message ("Transmit to %d dests: %s EV:%p", uc, s1, event);
      g_free (s1);
    }

  SWAMI_CONTROL_TEST_BREAK (control, NULL);
#endif

  bag = update_ctrls;
  while (bag->control)  /* send event to destination controls */
    {
      if (bag->trans)
	{ /* transform event using transform function */
	  transevent = swami_control_event_transform		/* ++ ref */
	    (event, bag->control->value_type, bag->trans, bag->data);

	  swami_control_set_event (bag->control, transevent);
	  swami_control_event_unref (transevent);	/* -- unref */
	}
      else swami_control_set_event (bag->control, event);

      g_object_unref (bag->control);	/* -- unref control from update array */
      bag++;
    }

  swami_control_event_active_unref (event); /* -- active unref */
  swami_control_event_unref (event); /* -- unref creator's reference */
}

/**
 * swami_control_transmit_event:
 * @control: Control object
 * @event: Event to send to output connections of @control
 *
 * This function sends an event to all destination connected
 * controls. Usually only used by #SwamiControl object
 * implementations.
 */
void
swami_control_transmit_event (SwamiControl *control, SwamiControlEvent *event)
{
  CtrlUpdateBag update_ctrls[MAX_DEST_CONNECTIONS + 1];
  SwamiControlEvent *transevent;
  CtrlUpdateBag *bag;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);

  swami_control_event_active_ref (event); /* ++ inc active ref count */

  {			 /* recursive function, save on stack space */
    SwamiControlConn *conn;
    SwamiControlEvent *origin;
    GSList *p;
    int uc = 0;

    origin = event->origin ? event->origin : event;

    swami_control_event_ref (origin); /* ++ ref event for active list */

    SWAMI_LOCK_WRITE (control);

    /* check for event looping (only if control can send) */
    if (!swami_control_loop_check (control, event))
      {
	SWAMI_UNLOCK_WRITE (control);
	swami_control_event_active_unref (event); /* -- decrement active ref */
	return;
      }

    control->active = g_list_prepend (control->active, origin);

    /* copy destination controls to an array under lock, which is then used
       outside of lock to avoid recursive dead locks */

    /* loop over destination connections */
    for (p = control->outputs; p; p = p->next)
      {
	conn = (SwamiControlConn *)(p->data);

	/* ++ ref dest control and add to array, copy transform func also */
	update_ctrls[uc].control = g_object_ref (conn->control);
	update_ctrls[uc].trans = conn->trans;
	update_ctrls[uc++].data = conn->data;
      }

    update_ctrls[uc].control = NULL;

    SWAMI_UNLOCK_WRITE (control);

  } /* stack space saver */

#if DEBUG
  if (swami_control_debug)
    {
      char *s1 = pretty_control (control);
      g_message ("Transmit: %s EV:%p ORIGIN:%p", s1, event, event->origin);
      g_free (s1);
    }

  SWAMI_CONTROL_TEST_BREAK (control, NULL);
#endif

  bag = update_ctrls;
  while (bag->control)  /* send event to destination controls */
    {
      if (bag->trans)
	{ /* transform event using transform function */
	  transevent = swami_control_event_transform		/* ++ ref */
	    (event, bag->control->value_type, bag->trans, bag->data);

	  swami_control_set_event (bag->control, transevent);
	  swami_control_event_unref (transevent);	/* -- unref */
	}
      else swami_control_set_event (bag->control, event);

      g_object_unref (bag->control);	/* -- unref control from update array */
      bag++;
    }

  swami_control_event_active_unref (event); /* -- decrement active ref */
}

/**
 * swami_control_transmit_event_loop:
 * @control: Control object
 * @event: Event to send to output connections of @control
 *
 * Like swami_control_transmit_event() but doesn't do an event loop check.
 * This is useful for control implementations that receive an event and
 * want to re-transmit it (swami_control_transmit_event() would stop it,
 * since the event is already in the active list for the control).
 */
void
swami_control_transmit_event_loop (SwamiControl *control,
				   SwamiControlEvent *event)
{
  CtrlUpdateBag update_ctrls[MAX_DEST_CONNECTIONS + 1];
  SwamiControlEvent *transevent;
  CtrlUpdateBag *bag;

  g_return_if_fail (SWAMI_IS_CONTROL (control));
  g_return_if_fail (event != NULL);

  swami_control_event_active_ref (event); /* ++ inc active ref count */

  {			 /* recursive function, save on stack space */
    SwamiControlConn *conn;
    SwamiControlEvent *origin;
    GSList *p;
    int uc = 0;

    origin = event->origin ? event->origin : event;

    SWAMI_LOCK_WRITE (control);

    /* check for event in active list (only if control can send) */
    if (swami_control_loop_check (control, event))
      { /* not already in list, prepend the event origin to the active list */
	control->active = g_list_prepend (control->active, origin);
	swami_control_event_ref (origin); /* ++ ref event for active list */
      }

    /* copy destination controls to an array under lock, which is then used
       outside of lock to avoid recursive dead locks */

    /* loop over destination connections */
    for (p = control->outputs; p; p = p->next)
      {
	conn = (SwamiControlConn *)(p->data);

	/* ++ ref dest control and add to array, copy transform func also */
	update_ctrls[uc].control = g_object_ref (conn->control);
	update_ctrls[uc].trans = conn->trans;
	update_ctrls[uc++].data = conn->data;
      }

    update_ctrls[uc].control = NULL;

    SWAMI_UNLOCK_WRITE (control);

  } /* stack space saver */

#if DEBUG
  if (swami_control_debug)
    {
      char *s1 = pretty_control (control);
      g_message ("Transmit: %s EV:%p ORIGIN:%p", s1, event, event->origin);
      g_free (s1);
    }

  SWAMI_CONTROL_TEST_BREAK (control, NULL);
#endif

  bag = update_ctrls;
  while (bag->control)  /* send event to destination controls */
    {
      if (bag->trans)
	{ /* transform event using transform function */
	  transevent = swami_control_event_transform		/* ++ ref */
	    (event, bag->control->value_type, bag->trans, bag->data);

	  swami_control_set_event (bag->control, transevent);
	  swami_control_event_unref (transevent);	/* -- unref */
	}
      else swami_control_set_event (bag->control, event);

      g_object_unref (bag->control);	/* -- unref control from update array */
      bag++;
    }

  swami_control_event_active_unref (event); /* -- decrement active ref */
}

/**
 * swami_control_do_event_expiration:
 * 
 * Processes all controls in search of inactive expired events.  This should
 * be called periodically to expire events in controls that don't receive any
 * events for a long period after previous activity.  Note that currently this
 * is handled automatically if swami_init() is called.
 */
void
swami_control_do_event_expiration (void)
{
  GList *cp, *ep, *temp;
  SwamiControlEvent *ev;
  SwamiControl *control;

  G_LOCK (control_list);

  for (cp = control_list; cp; cp = cp->next) /* loop over controls */
    {
      control = (SwamiControl *)(cp->data);

      SWAMI_LOCK_WRITE (control);

      ep = control->active;
      while (ep)		/* loop over active event list */
	{
	  ev = (SwamiControlEvent *)(ep->data);
	  if (!ev->active)		/* event still active? */
	    {	/* no, remove from list */
	      temp = ep;
	      ep = g_list_next (ep);
	      control->active = g_list_delete_link (control->active, temp);
	      swami_control_event_unref (ev); /* -- unref inactive event */
	    }
	  else ep = g_list_next (ep);
	}

      SWAMI_UNLOCK_WRITE (control);
    }

  G_UNLOCK (control_list);
}

/**
 * swami_control_new_event:
 * @control: Control object
 * @origin: Origin event or %NULL if new event is origin
 * @value: Value to use as event value or %NULL to use @control as the
 *   value (a value change event)
 *
 * Create an event for @control. If @value is non %NULL then
 * it is used as the value of the event otherwise the @control is used as
 * the value of the event (a value changed event).
 *
 * Returns: New event with a refcount of 1 which the caller owns. Remember
 *   that a #SwamiControlEvent is not a GObject and has its own reference
 *   counting routines swami_control_event_ref() and
 *   swami_control_event_unref().
 */
SwamiControlEvent *
swami_control_new_event (SwamiControl *control, SwamiControlEvent *origin,
			 const GValue *value)
{
  SwamiControlEvent *event;

  g_return_val_if_fail (SWAMI_IS_CONTROL (control), NULL);

  event = swami_control_event_new (TRUE); /* ++ ref new event */
  if (origin) swami_control_event_set_origin (event, origin);

  if (value)			/* if value supplied, use it */
    {
      g_value_init (&event->value, G_VALUE_TYPE (value));
      g_value_copy (value, &event->value);
    }
  else			    /* create a value change event */
    {
      g_value_init (&event->value, G_TYPE_OBJECT);
      g_value_set_object (&event->value, control);
    }

  return (event);		/* !! caller owns reference */
}
