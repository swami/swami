/*
 * SwamiguiControlAdj.c - GtkAdjustment control object
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

#include "SwamiguiControlAdj.h"
#include "SwamiguiRoot.h"

static void swamigui_control_adj_class_init (SwamiguiControlAdjClass *klass);
static void swamigui_control_adj_init (SwamiguiControlAdj *ctrladj);
static void swamigui_control_adj_finalize (GObject *object);
static GParamSpec *control_adj_get_spec_method (SwamiControl *control);
static gboolean control_adj_set_spec_method (SwamiControl *control,
					     GParamSpec *pspec);
static void control_adj_get_value_method (SwamiControl *control,
					  GValue *value);
static void control_adj_set_value_method (SwamiControl *control,
					  SwamiControlEvent *event,
					  const GValue *value);
static void swamigui_control_adj_cb_value_changed (GtkAdjustment *adj,
						  SwamiguiControlAdj *ctrladj);
static void swamigui_adj_cb_destroy (GtkObject *object, gpointer user_data);

static GObjectClass *parent_class = NULL;

GType
swamigui_control_adj_get_type (void)
{
  static GType otype = 0;

  if (!otype)
    {
      static const GTypeInfo type_info =
	{
	  sizeof (SwamiguiControlAdjClass), NULL, NULL,
	  (GClassInitFunc) swamigui_control_adj_class_init,
	  (GClassFinalizeFunc) NULL, NULL,
	  sizeof (SwamiguiControlAdj),
	  0,
	  (GInstanceInitFunc) swamigui_control_adj_init
	};

      otype = g_type_register_static (SWAMI_TYPE_CONTROL, "SwamiguiControlAdj",
				      &type_info, 0);
    }

  return (otype);
}

static void
swamigui_control_adj_class_init (SwamiguiControlAdjClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  SwamiControlClass *control_class = SWAMI_CONTROL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = swamigui_control_adj_finalize;

  control_class->get_spec = control_adj_get_spec_method;
  control_class->set_spec = control_adj_set_spec_method;
  control_class->get_value = control_adj_get_value_method;
  control_class->set_value = control_adj_set_value_method;
}

static void
swamigui_control_adj_init (SwamiguiControlAdj *ctrladj)
{
  SwamiControl *control = SWAMI_CONTROL (ctrladj);

  swami_control_set_queue (control, swamigui_root->ctrl_queue);
  swami_control_set_flags (control, SWAMI_CONTROL_SENDRECV | SWAMI_CONTROL_VALUE);
  swami_control_set_value_type (control, G_TYPE_DOUBLE);

  ctrladj->adj = NULL;
}

static void
swamigui_control_adj_finalize (GObject *object)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (object);

  SWAMI_LOCK_WRITE (ctrladj);

  if (ctrladj->adj)
    {
      g_signal_handler_disconnect (ctrladj->adj,
				   ctrladj->value_change_id);
      g_signal_handlers_disconnect_by_func (ctrladj->adj,
					    swamigui_adj_cb_destroy, ctrladj);
      g_object_unref (ctrladj->adj);
    }
  if (ctrladj->pspec) g_param_spec_unref (ctrladj->pspec);

  SWAMI_UNLOCK_WRITE (ctrladj);

  if (parent_class->finalize)
    parent_class->finalize (object);
}

/* control is locked by caller */
static GParamSpec *
control_adj_get_spec_method (SwamiControl *control)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (control);
  return (ctrladj->pspec);
}

/* control is locked by caller */
static gboolean
control_adj_set_spec_method (SwamiControl *control, GParamSpec *pspec)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (control);
  GParamSpecDouble *pspec_dbl;

  if (ctrladj->pspec) g_param_spec_unref (ctrladj->pspec);
  ctrladj->pspec = g_param_spec_ref (pspec);
  g_param_spec_sink (pspec); /* take ownership of the parameter spec */

  if (ctrladj->adj)
    {
      pspec_dbl = G_PARAM_SPEC_DOUBLE (pspec);
      ctrladj->adj->lower = pspec_dbl->minimum;
      ctrladj->adj->upper = pspec_dbl->maximum;
      gtk_adjustment_changed (ctrladj->adj);
    }

  return (TRUE);
}

/* control is NOT locked */
static void
control_adj_get_value_method (SwamiControl *control, GValue *value)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (control);

  SWAMI_LOCK_READ (ctrladj);

  if (!ctrladj->adj)
    {
      SWAMI_UNLOCK_READ (ctrladj);
      return;
    }

  g_value_set_double (value, ctrladj->adj->value);

  SWAMI_UNLOCK_READ (ctrladj);
}

static void
control_adj_set_value_method (SwamiControl *control, SwamiControlEvent *event,
			      const GValue *value)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (control);
  GtkAdjustment *adj;
  guint value_change_id;
  double d;

  /* reference the adjustment under lock, to minimize lock time and prevent
     possible deadlocks with gtk_adjustment_value_changed callbacks */
  SWAMI_LOCK_READ (ctrladj);
  adj = ctrladj->adj;
  if (adj) g_object_ref (adj);	/* ++ ref adjustment */
  value_change_id = ctrladj->value_change_id;
  SWAMI_UNLOCK_READ (ctrladj);

  if (!adj)
    {
      g_object_unref (adj);		/* -- unref adjustment */
      return;
    }

  d = g_value_get_double (value);

  if (adj->value != d)		/* value is different? */
    {
      /* block handler to avoid value set/notify loop */
      g_signal_handler_block (adj, value_change_id);
      adj->value = g_value_get_double (value);
      gtk_adjustment_value_changed (adj);
      g_signal_handler_unblock (adj, value_change_id);
    }

  g_object_unref (adj);		/* -- unref adjustment */
}

/**
 * swamigui_control_adj_new:
 * @adj: GtkAdjustment to use as a control or %NULL to set later
 *
 * Create a new GtkAdjustment control object.
 *
 * Returns: New GtkAdjustment control with a refcount of 1 which the caller
 * owns.
 */
SwamiguiControlAdj *
swamigui_control_adj_new (GtkAdjustment *adj)
{
  SwamiguiControlAdj *ctrladj;

  ctrladj = g_object_new (SWAMIGUI_TYPE_CONTROL_ADJ, NULL);
  if (adj) swamigui_control_adj_set (ctrladj, adj);

  return (ctrladj);
}

/**
 * swamigui_control_adj_set:
 * @ctrladj: Swami GtkAdjustment control object
 * @adj: GtkAdjustment to assign to @ctrladj
 *
 * Set the GtkAdjustment of a adjustment control.
 */
void
swamigui_control_adj_set (SwamiguiControlAdj *ctrladj, GtkAdjustment *adj)
{
  GParamSpec *pspec;

  g_return_if_fail (SWAMIGUI_IS_CONTROL_ADJ (ctrladj));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

  /* ++ ref new spec */
  pspec = g_param_spec_double ("value", NULL, NULL, adj->lower, adj->upper,
			       adj->value, G_PARAM_READWRITE);

  SWAMI_LOCK_WRITE (ctrladj);

  if (ctrladj->adj)
    { /* disconnect value change and destroy handlers */
      g_signal_handler_disconnect (ctrladj->adj, ctrladj->value_change_id);
      g_signal_handlers_disconnect_by_func (ctrladj->adj,
					    swamigui_adj_cb_destroy, ctrladj);
      g_object_unref (ctrladj->adj); /* -- unref old adjustment */
    }

  if (ctrladj->pspec) g_param_spec_unref (ctrladj->pspec);

  ctrladj->adj = g_object_ref (adj); /* ++ ref new adjustment */
  ctrladj->pspec = pspec;	/* we take over creator's ref */

  /* connect signal to adjustment change signals */
  ctrladj->value_change_id
    = g_signal_connect (adj, "value-changed",
			G_CALLBACK (swamigui_control_adj_cb_value_changed),
			ctrladj);

  /* connect to the adjustment's destroy signal */
  g_signal_connect (adj, "destroy", G_CALLBACK (swamigui_adj_cb_destroy),
		    ctrladj);

  SWAMI_UNLOCK_WRITE (ctrladj);
}

/**
 * swamigui_control_adj_block_changes:
 * @ctrladj: Swami GtkAdjustment control object
 * 
 * Stop GtkAdjustment change signals from sending control events.
 */
void
swamigui_control_adj_block_changes (SwamiguiControlAdj *ctrladj)
{
  g_return_if_fail (SWAMIGUI_IS_CONTROL_ADJ (ctrladj));

  SWAMI_LOCK_READ (ctrladj);

  if (ctrladj->adj)
    g_signal_handler_block (ctrladj->adj, ctrladj->value_change_id);

  SWAMI_UNLOCK_READ (ctrladj);
}

/**
 * swamigui_control_adj_block_changes:
 * @ctrladj: Swami GtkAdjustment control object
 * 
 * Unblock a previous call to swamigui_control_adj_block_changes().
 */
void
swamigui_control_adj_unblock_changes (SwamiguiControlAdj *ctrladj)
{
  g_return_if_fail (SWAMIGUI_IS_CONTROL_ADJ (ctrladj));

  SWAMI_LOCK_READ (ctrladj);

  if (ctrladj->adj)
    g_signal_handler_unblock (ctrladj->adj, ctrladj->value_change_id);

  SWAMI_UNLOCK_READ (ctrladj);
}

/* adjustment value changed signal callback */
static void
swamigui_control_adj_cb_value_changed (GtkAdjustment *adj,
				       SwamiguiControlAdj *ctrladj)
{
  GValue value = { 0 };

  g_value_init (&value, G_TYPE_DOUBLE);
  g_value_set_double (&value, adj->value);
  swami_control_transmit_value ((SwamiControl *)ctrladj, &value);
  g_value_unset (&value);
}

/* GtkAdjustment destroy callback - releases reference */
static void
swamigui_adj_cb_destroy (GtkObject *object, gpointer user_data)
{
  SwamiguiControlAdj *ctrladj = SWAMIGUI_CONTROL_ADJ (user_data);

  SWAMI_LOCK_WRITE (ctrladj);

  if (ctrladj->adj)
    { /* disconnect value change and destroy handlers */
      g_signal_handler_disconnect (ctrladj->adj, ctrladj->value_change_id);
      g_signal_handlers_disconnect_by_func (ctrladj->adj,
					    swamigui_adj_cb_destroy, ctrladj);
      g_object_unref (ctrladj->adj); /* -- unref old adjustment */
    }

  if (ctrladj->pspec)
    {
      g_param_spec_unref (ctrladj->pspec);
      ctrladj->pspec = NULL;
    }

  SWAMI_UNLOCK_WRITE (ctrladj);
}
