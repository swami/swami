/*
 * SwamiControl.h - Swami control base object
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
#ifndef __SWAMI_CONTROL_H__
#define __SWAMI_CONTROL_H__

typedef struct _SwamiControl SwamiControl;
typedef struct _SwamiControlClass SwamiControlClass;

#include <sys/time.h>
#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiLock.h>
#include <libswami/SwamiControlQueue.h>
#include <libinstpatch/IpatchList.h>
#include <libswami/SwamiControlEvent.h>
#include <libswami/SwamiParam.h>

#define SWAMI_TYPE_CONTROL  (swami_control_get_type ())
#define SWAMI_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL, SwamiControl))
#define SWAMI_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL, \
   SwamiControlClass))
#define SWAMI_IS_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL))
#define SWAMI_IS_CONTROL_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), SWAMI_TYPE_CONTROL))
#define SWAMI_CONTROL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS (obj, SWAMI_TYPE_CONTROL, SwamiControlClass))


/* Swami control object */
struct _SwamiControl
{
  SwamiLock parent_instance;	/* derived from SwamiLock */

  guint flags;			/* flags field (SwamiControlFlags) */
  GList *active;		/* active event propagations */
  SwamiControlQueue *queue;	/* event queue or NULL if no queuing */
  SwamiControl *master;	/* control to slave parameter spec to or NULL */
  GType value_type;	  /* control value type (or 0 for wildcard) */

  /* lists of SwamiControlConn structures (defined in SwamiControl.c) */
  GSList *inputs;	    /* list of input connections (readable) */
  GSList *outputs;	   /* list of output connections (writable) */
};

/**
 * SwamiControlGetSpecFunc:
 * @control: Control to get parameter spec for
 *
 * A #SwamiControlClass optional method type to get the parameter spec
 * from a control. If this method is not used or it returns %NULL then
 * the control is assumed to accept control events of any type.
 *
 * Note: The control is locked while calling this method. Also, this
 * method should return the parameter spec quickly, to speed up the event
 * emission process.
 *
 * Returns: Should return the parameter specification for the control or
 * %NULL for type independent controls. Reference count should not be modified,
 * this is handled by the caller.
 */
typedef GParamSpec *(*SwamiControlGetSpecFunc)(SwamiControl *control);

/**
 * SwamiControlSetSpecFunc:
 * @control: Control to get parameter spec for
 * @pspec: Parameter specification to assign
 *
 * A #SwamiControlClass optional method type to set the parameter spec
 * for a control.  The method should call g_param_spec_ref() followed by
 * g_param_spec_sink () to take over the reference.  
 * If a control has a specific value type and the
 * #SWAMI_CONTROL_SPEC_NO_CONV is not set then the parameter spec is
 * converted to the parameter spec type used for the given value type and the
 * min/max/default values of the parameter spec will be processed with the
 * set_trans value transform function if assigned,
 * otherwise the parameter spec is passed as is without conversion.
 *
 * Note: The control is locked while calling this method.
 *
 * Returns: Should return %TRUE if parameter spec change was allowed,
 * %FALSE otherwise.
 */
typedef gboolean (*SwamiControlSetSpecFunc)(SwamiControl *control,
					    GParamSpec *pspec);
/**
 * SwamiControlGetValueFunc:
 * @control: Control to get value from
 * @value: Caller supplied value to store control value in
 *
 * A #SwamiControlClass optional method type to get the value from a
 * value control that is readable (#SWAMI_CONTROL_SENDS flag must be
 * set). The @value has been initialized to the native type of the
 * control's parameter spec. If this method is used then
 * #SwamiControlGetSpecFunc must also be set.
 *
 * Note: The control is not locked when calling this method.
 */
typedef void (*SwamiControlGetValueFunc)(SwamiControl *control, GValue *value);

/**
 * SwamiControlSetValueFunc:
 * @control: Control that is receiving an event
 * @event: Control event being received
 * @value: Value which is being received (possibly converted from original
 *   event value depending on the control's settings)
 *
 * A #SwamiControlClass optional method type to receive control
 * values. If the #SWAMI_CONTROL_NO_CONV flag is not set for this
 * control and the #SwamiControlGetSpecFunc returns a parameter spec
 * then @value will be the result of the original event value
 * converted to the control's native type.
 *
 * This method gets called for events received via a control's inputs
 * or when swami_control_set_event() or swami_control_set_value() is called.
 *
 * Note: The control is not locked during this method call.
 */
typedef void (*SwamiControlSetValueFunc)(SwamiControl *control,
					 SwamiControlEvent *event,
					 const GValue *value);

struct _SwamiControlClass
{
  SwamiLockClass parent_class;

  /* signals */
  void (*connect)(SwamiControl *c1, SwamiControl *c2, guint flags);
  void (*disconnect)(SwamiControl *c1, SwamiControl *c2, guint flags);

  /* methods */
  SwamiControlGetSpecFunc get_spec;
  SwamiControlSetSpecFunc set_spec;
  SwamiControlGetValueFunc get_value;
  SwamiControlSetValueFunc set_value;
};

typedef enum
{
  SWAMI_CONTROL_SENDS        = 1 << 0, /* control is readable/sends */
  SWAMI_CONTROL_RECVS        = 1 << 1, /* control is writable/receives */
  SWAMI_CONTROL_NO_CONV      = 1 << 2, /* don't convert incoming values */
  SWAMI_CONTROL_NATIVE       = 1 << 3, /* values of native value type only */
  SWAMI_CONTROL_VALUE        = 1 << 4, /* value control - queue optimization */
  SWAMI_CONTROL_SPEC_NO_CONV = 1 << 5  /* don't convert parameter spec type */
} SwamiControlFlags;

/* mask for user controlled flag bits */
#define SWAMI_CONTROL_FLAGS_USER_MASK 0x7F

/* a convenience value for send/receive controls */
#define SWAMI_CONTROL_SENDRECV  (SWAMI_CONTROL_SENDS | SWAMI_CONTROL_RECVS)

/* 7 bits used, 5 reserved */
#define SWAMI_CONTROL_UNUSED_FLAG_SHIFT 12

/* connection priority ranking (first 2 bits of flags field) */
typedef enum
{
  SWAMI_CONTROL_CONN_PRIORITY_DEFAULT = 0,
  SWAMI_CONTROL_CONN_PRIORITY_LOW     = 1,
  SWAMI_CONTROL_CONN_PRIORITY_MEDIUM  = 2,
  SWAMI_CONTROL_CONN_PRIORITY_HIGH    = 3
} SwamiControlConnPriority;

#define SWAMI_CONTROL_CONN_PRIORITY_MASK  (0x2)
#define SWAMI_CONTROL_CONN_DEFAULT_PRIORITY_VALUE \
  (SWAMI_CONTROL_CONN_PRIORITY_MEDIUM)

typedef enum /*< flags >*/
{
  SWAMI_CONTROL_CONN_INPUT  = 1 << 2, /* set for inputs (used internally) */
  SWAMI_CONTROL_CONN_OUTPUT = 1 << 3, /* set for outputs (used internally) */
  SWAMI_CONTROL_CONN_INIT   = 1 << 4, /* update value on connect */
  SWAMI_CONTROL_CONN_BIDIR  = 1 << 5, /* make a bi-directional connection */
  SWAMI_CONTROL_CONN_SPEC   = 1 << 6  /* synchronize the parameter spec on connect */
} SwamiControlConnFlags;

/* convenience combo flags */

/* #SWAMI_CONTROL_CONN_BIDIR and #SWAMI_CONTROL_CONN_INIT */
#define SWAMI_CONTROL_CONN_BIDIR_INIT \
  (SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_INIT)

/* #SWAMI_CONTROL_CONN_BIDIR, #SWAMI_CONTROL_CONN_SPEC and
 * #SWAMI_CONTROL_CONN_INIT */
#define SWAMI_CONTROL_CONN_BIDIR_SPEC_INIT \
  (SWAMI_CONTROL_CONN_BIDIR | SWAMI_CONTROL_CONN_SPEC | SWAMI_CONTROL_CONN_INIT)


GType swami_control_get_type (void);
SwamiControl *swami_control_new (void);

void swami_control_connect (SwamiControl *src, SwamiControl *dest,
			    guint flags);
void swami_control_connect_transform
  (SwamiControl *src, SwamiControl *dest, guint flags,
   SwamiValueTransform trans1, SwamiValueTransform trans2,
   gpointer data1, gpointer data2,
   GDestroyNotify destroy1, GDestroyNotify destroy2);
void swami_control_connect_item_prop (SwamiControl *dest, GObject *object,
				      GParamSpec *pspec);
void swami_control_disconnect (SwamiControl *src, SwamiControl *dest);
void swami_control_disconnect_all (SwamiControl *control);
void swami_control_disconnect_unref (SwamiControl *control);
IpatchList *swami_control_get_connections (SwamiControl *control,
					   SwamiControlConnFlags dir);
void swami_control_set_transform (SwamiControl *src, SwamiControl *dest,
				  SwamiValueTransform trans, gpointer data,
				  GDestroyNotify destroy);
void swami_control_get_transform (SwamiControl *src, SwamiControl *dest,
				  SwamiValueTransform *trans);

void swami_control_set_flags (SwamiControl *control, int flags);
int swami_control_get_flags (SwamiControl *control);
void swami_control_set_queue (SwamiControl *control, SwamiControlQueue *queue);
SwamiControlQueue *swami_control_get_queue (SwamiControl *control);

GParamSpec *swami_control_get_spec (SwamiControl *control);
gboolean swami_control_set_spec (SwamiControl *control, GParamSpec *pspec);
gboolean swami_control_sync_spec (SwamiControl *control, SwamiControl *source,
				  SwamiValueTransform trans, gpointer data);
GParamSpec *
swami_control_transform_spec (SwamiControl *control, SwamiControl *source,
			      SwamiValueTransform trans, gpointer data);
void swami_control_set_value_type (SwamiControl *control, GType type);
void swami_control_get_value (SwamiControl *control, GValue *value);
void swami_control_get_value_native (SwamiControl *control, GValue *value);
void swami_control_set_value (SwamiControl *control, const GValue *value);
void swami_control_set_value_no_queue (SwamiControl *control,
				       const GValue *value);
void swami_control_set_event (SwamiControl *control, SwamiControlEvent *event);
void swami_control_set_event_no_queue (SwamiControl *control,
				       SwamiControlEvent *event);
void swami_control_set_event_no_queue_loop (SwamiControl *control,
					    SwamiControlEvent *event);
void swami_control_transmit_value (SwamiControl *control, const GValue *value);
void swami_control_transmit_event (SwamiControl *control,
				   SwamiControlEvent *event);
void swami_control_transmit_event_loop (SwamiControl *control,
					SwamiControlEvent *event);
void swami_control_do_event_expiration (void);
SwamiControlEvent *swami_control_new_event (SwamiControl *control,
					    SwamiControlEvent *origin,
					    const GValue *value);
#endif
