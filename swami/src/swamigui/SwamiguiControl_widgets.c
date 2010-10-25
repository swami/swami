/*
 * SwamiguiControl_widgets.c - Builtin GtkWidget control handlers
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

#include "SwamiguiControl.h"
#include "SwamiguiControlAdj.h"
#include "SwamiguiSpinScale.h"
#include "SwamiguiNoteSelector.h"
#include "i18n.h"


static void func_control_cb_widget_destroy (GtkObject *object,
					    gpointer user_data);

static SwamiControl *adjustment_control_handler (GObject *widget,
						 GType value_type,
						 GParamSpec *pspec,
						 SwamiguiControlFlags flags);
static void adjustment_control_cb_spec_changed (SwamiControl *control,
						GParamSpec *pspec,
						gpointer user_data);
static SwamiControl *entry_control_handler (GObject *widget,
					    GType value_type,
					    GParamSpec *pspec,
					    SwamiguiControlFlags flags);
static void entry_control_get_func (SwamiControl *control, GValue *value);
static void entry_control_set_func (SwamiControl *control,
				    SwamiControlEvent *event,
				    const GValue *value);
static void entry_control_cb_changed (GtkEditable *editable,
				      gpointer user_data);
static SwamiControl *combo_box_entry_control_handler (GObject *widget,
						      GType value_type,
						      GParamSpec *pspec,
						      SwamiguiControlFlags flags);

static SwamiControl *text_view_control_handler (GObject *widget,
						GType value_type,
						GParamSpec *pspec,
						SwamiguiControlFlags flags);
static SwamiControl *text_buffer_control_handler (GObject *widget,
						  GType value_type,
						  GParamSpec *pspec,
						  SwamiguiControlFlags flags);
static void text_buffer_control_get_func (SwamiControl *control,
					  GValue *value);
static void text_buffer_control_set_func (SwamiControl *control,
					  SwamiControlEvent *event,
					  const GValue *value);
static void text_buffer_control_cb_changed (GtkTextBuffer *txtbuf,
					    gpointer user_data);
static void text_buffer_widget_weak_notify (gpointer data,
					    GObject *where_the_object_was);
static SwamiControl *file_chooser_control_handler (GObject *widget,
						   GType value_type,
						   GParamSpec *pspec,
						   SwamiguiControlFlags flags);
static void file_chooser_control_get_func (SwamiControl *control, GValue *value);
static void file_chooser_control_set_func (SwamiControl *control,
					   SwamiControlEvent *event,
					   const GValue *value);
static void file_chooser_control_cb_changed (GtkFileChooser *chooser,
					     gpointer user_data);
 
static SwamiControl *label_control_handler (GObject *widget,
					    GType value_type,
					    GParamSpec *pspec,
					    SwamiguiControlFlags flags);
static void label_control_get_func (SwamiControl *control, GValue *value);
static void label_control_set_func (SwamiControl *control,
				    SwamiControlEvent *event,
				    const GValue *value);

static SwamiControl *toggle_button_control_handler (GObject *widget,
						    GType value_type,
						    GParamSpec *pspec,
						    SwamiguiControlFlags flags);
static void toggle_button_control_get_func (SwamiControl *control,
					    GValue *value);
static void toggle_button_control_set_func (SwamiControl *control,
					    SwamiControlEvent *event,
					    const GValue *value);
static void toggle_button_control_toggled (GtkToggleButton *btn,
					   gpointer user_data);
static SwamiControl *combo_box_string_control_handler (GObject *widget,
						       GType value_type,
						       GParamSpec *pspec,
						       SwamiguiControlFlags flags);
static void combo_box_string_control_get_func (SwamiControl *control, GValue *value);
static void combo_box_string_control_set_func (SwamiControl *control,
					       SwamiControlEvent *event,
					       const GValue *value);
static void combo_box_string_control_changed (GtkComboBox *combo, gpointer user_data);
static SwamiControl *combo_box_enum_control_handler (GObject *widget,
						     GType value_type,
						     GParamSpec *pspec,
						     SwamiguiControlFlags flags);
static void combo_box_enum_control_get_func (SwamiControl *control, GValue *value);
static void combo_box_enum_control_set_func (SwamiControl *control,
					     SwamiControlEvent *event,
					     const GValue *value);
static void combo_box_enum_control_changed (GtkComboBox *combo, gpointer user_data);
static SwamiControl *combo_box_gtype_control_handler (GObject *widget,
						      GType value_type,
						      GParamSpec *pspec,
						      SwamiguiControlFlags flags);
static void combo_box_gtype_control_get_func (SwamiControl *control, GValue *value);
static void combo_box_gtype_control_set_func (SwamiControl *control,
					      SwamiControlEvent *event,
					      const GValue *value);
static void combo_box_gtype_control_changed (GtkComboBox *combo, gpointer user_data);


void
_swamigui_control_widgets_init (void)
{
  /* GtkAdjustment based GUI controls */
  swamigui_control_register (GTK_TYPE_SPIN_BUTTON, G_TYPE_DOUBLE,
			     adjustment_control_handler,
			     SWAMIGUI_CONTROL_RANK_HIGH);
  swamigui_control_register (SWAMIGUI_TYPE_SPIN_SCALE, G_TYPE_DOUBLE,
			     adjustment_control_handler, 0);
  swamigui_control_register (GTK_TYPE_HSCALE, G_TYPE_DOUBLE,
			     adjustment_control_handler, 0);
  swamigui_control_register (GTK_TYPE_VSCALE, G_TYPE_DOUBLE,
			     adjustment_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_HSCROLLBAR, G_TYPE_DOUBLE,
			     adjustment_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_VSCROLLBAR, G_TYPE_DOUBLE,
			     adjustment_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (SWAMIGUI_TYPE_NOTE_SELECTOR, G_TYPE_DOUBLE,
			     adjustment_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);

  /* string type controls */
  swamigui_control_register (GTK_TYPE_ENTRY, G_TYPE_STRING,
			     entry_control_handler,
			     SWAMIGUI_CONTROL_RANK_HIGH);
  swamigui_control_register (GTK_TYPE_COMBO_BOX_ENTRY, G_TYPE_STRING,
			     combo_box_entry_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_TEXT_VIEW, G_TYPE_STRING,
			     text_view_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_TEXT_BUFFER, G_TYPE_STRING,
			     text_buffer_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_FILE_CHOOSER_BUTTON, G_TYPE_STRING,
			     file_chooser_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_LABEL, G_TYPE_STRING,
			     label_control_handler,
			     SWAMIGUI_CONTROL_VIEW
			     | SWAMIGUI_CONTROL_RANK_LOW);

  /* button controls */

  swamigui_control_register (GTK_TYPE_CHECK_BUTTON, G_TYPE_BOOLEAN,
			     toggle_button_control_handler,
			     SWAMIGUI_CONTROL_RANK_HIGH);
  swamigui_control_register (GTK_TYPE_TOGGLE_BUTTON, G_TYPE_BOOLEAN,
			     toggle_button_control_handler, 0);

  /* combo box controls */
  swamigui_control_register (GTK_TYPE_COMBO_BOX, G_TYPE_STRING,
			     combo_box_string_control_handler,
			     SWAMIGUI_CONTROL_RANK_HIGH);
  swamigui_control_register (GTK_TYPE_COMBO_BOX, G_TYPE_ENUM,
			     combo_box_enum_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);
  swamigui_control_register (GTK_TYPE_COMBO_BOX, G_TYPE_GTYPE,
			     combo_box_gtype_control_handler,
			     SWAMIGUI_CONTROL_RANK_LOW);

  /* Additional possible GUI controls: GtkProgressBar, GtkButton */
}


/* function used by function control handlers to catch widget "destroy"
   signal and remove widget reference */
static void
func_control_cb_widget_destroy (GtkObject *object, gpointer user_data)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (user_data);
  gpointer data;

  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data)			/* destroy may be called multiple times */
    {
      SWAMI_LOCK_WRITE (control);
      SWAMI_CONTROL_FUNC_DATA (control) = NULL;
      g_object_unref (object);
      SWAMI_UNLOCK_WRITE (control);
    }
}


static SwamiControl *
adjustment_control_handler (GObject *widget, GType value_type,
			    GParamSpec *pspec,
			    SwamiguiControlFlags flags)
{
  GtkAdjustment *adj;
  SwamiControl *control = NULL;
  IpatchUnitInfo *unitinfo;
  gdouble min, max, def;
  gboolean isint = FALSE;
  guint units;
  guint digits = 2;	/* default digits */

  g_object_get (widget, "adjustment", &adj, NULL);
  g_return_val_if_fail (adj != NULL, NULL);

  if (GTK_IS_SPIN_BUTTON (widget) && !SWAMIGUI_IS_NOTE_SELECTOR (widget))
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), TRUE);

  if (pspec && swami_param_get_limits (pspec, &min, &max, &def, &isint))
    { /* set the number of significant decimal digits */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "digits"))
	{
	  if (!isint)
	    {	/* if unit-type is set.. */
	      ipatch_param_get (pspec, "unit-type", &units, NULL);
	      if (units)
		{
		  unitinfo = ipatch_unit_lookup (units);
		  digits = unitinfo->digits;
		}
	      else ipatch_param_get (pspec, "float-digits", &digits, NULL);
	    }
	  else digits = 0;	/* int type, no decimal digits */

	  g_object_set (widget, "digits", digits, NULL);
	}

      adj->lower = min;
      adj->upper = max;
      adj->value = def;
    }
  else
    {
      adj->lower = 0.0;
      adj->upper = G_MAXINT;
      adj->value = 0.0;
    }

  adj->step_increment = 1.0;
  adj->page_increment = 10.0;	// FIXME - Smarter?

  gtk_adjustment_changed (adj);
  gtk_adjustment_value_changed (adj);

  if (!(flags & SWAMIGUI_CONTROL_NO_CREATE))
  {
    control = SWAMI_CONTROL (swamigui_control_adj_new (adj));

    /* if the pspec is not an integer type and widget has the digits property
     * watch for pspec changes (to update number of decimal digits) */
    if (!isint && g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "digits"))
      g_signal_connect (control, "spec-changed",
			G_CALLBACK (adjustment_control_cb_spec_changed),
			widget);
  }

  return (control);
}

/* updates digits if control parameter spec changes */
static void
adjustment_control_cb_spec_changed (SwamiControl *control, GParamSpec *pspec,
				    gpointer user_data)
{
  GObject *widget = G_OBJECT (user_data);
  IpatchUnitInfo *unitinfo;
  guint digits, units;

  ipatch_param_get (pspec, "unit-type", &units, NULL);
  if (units)
    {
      unitinfo = ipatch_unit_lookup (units);
      digits = unitinfo->digits;
    }
  else ipatch_param_get (pspec, "float-digits", &digits, NULL);

  /* changing digits causes control value to change, we block events here */
  swamigui_control_adj_block_changes (SWAMIGUI_CONTROL_ADJ (control));
  g_object_set (widget, "digits", digits, NULL);
  swamigui_control_adj_unblock_changes (SWAMIGUI_CONTROL_ADJ (control));
}

/*
 * Entry control handler
 */

static SwamiControl *
entry_control_handler (GObject *widget, GType value_type,
		       GParamSpec *pspec, SwamiguiControlFlags flags)
{
  SwamiControlFunc *control = NULL;

  if (!(flags & SWAMIGUI_CONTROL_NO_CREATE))
    {
      g_object_ref (widget);     /* ++ ref the GtkEntry for the control */

      control = swami_control_func_new ();
      swami_control_set_value_type (SWAMI_CONTROL (control), G_TYPE_STRING);

      if (!pspec) pspec = g_param_spec_string ("value", "value", "value",
					       NULL, G_PARAM_READWRITE);
      else g_param_spec_ref (pspec);	/* ++ ref for swami_control_set_spec */

      swami_control_set_spec (SWAMI_CONTROL (control), pspec);
      swami_control_func_assign_funcs (control, entry_control_get_func,
				       entry_control_set_func, NULL, widget);

      g_signal_connect (widget, "destroy",
			G_CALLBACK (func_control_cb_widget_destroy), control);
    }

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
    {
      gtk_editable_set_editable (GTK_EDITABLE (widget), TRUE);

      /* set entry max length */
      if (pspec)
	{
	  guint maxlen;

	  ipatch_param_get (pspec, "string-max-length", &maxlen, NULL);
	  gtk_entry_set_max_length (GTK_ENTRY (widget), maxlen);
	}

      if (control)
	g_signal_connect (widget, "changed",
			  G_CALLBACK (entry_control_cb_changed), control);
    } /* not controllable */
  else gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);

  return (SWAMI_CONTROL (control));
}

/* GtkEntry handler SwamiControl get value function */
static void
entry_control_get_func (SwamiControl *control, GValue *value)
{
  gpointer data;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) g_value_set_string (value, gtk_entry_get_text (GTK_ENTRY (data)));
  SWAMI_UNLOCK_READ (control);
}

/* GtkEntry handler SwamiControl set value function */
static void
entry_control_set_func (SwamiControl *control, SwamiControlEvent *event,
			const GValue *value)
{
  GtkEntry *entry = NULL;
  gpointer data;
  const char *s;

  /* minimize lock and ref the entry */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) entry = GTK_ENTRY (g_object_ref (data)); /* ++ ref entry */
  SWAMI_UNLOCK_READ (control);

  if (entry)
    {
      g_signal_handlers_block_by_func (entry, entry_control_cb_changed,
				       control);
      s = g_value_get_string (value);
      gtk_entry_set_text (entry, s ? s : "");
      g_signal_handlers_unblock_by_func (entry, entry_control_cb_changed,
					 control);
      g_object_unref (entry);	/* -- unref entry */
    }
}

/* callback for GtkEntry control to propagate text changes */
static void
entry_control_cb_changed (GtkEditable *editable, gpointer user_data)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (user_data);
  GValue value = { 0 };

  /* transmit the entry change */
  g_value_init (&value, G_TYPE_STRING);
  g_value_take_string (&value, gtk_editable_get_chars (editable, 0, -1));
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}

static SwamiControl *
combo_box_entry_control_handler (GObject *widget, GType value_type,
				 GParamSpec *pspec, SwamiguiControlFlags flags)
{
  GtkWidget *entry;

  entry = gtk_bin_get_child (GTK_BIN (widget));

  return (entry_control_handler (G_OBJECT (entry), value_type, pspec, flags));
}

/*
 * Text view control handler (uses text buffer control handler)
 */
static SwamiControl *
text_view_control_handler (GObject *widget, GType value_type,
			   GParamSpec *pspec, SwamiguiControlFlags flags)
{
  GtkTextView *txtview = GTK_TEXT_VIEW (widget);
  GObject *txtbuf;

  txtbuf = G_OBJECT (gtk_text_view_get_buffer (txtview));
  return (text_buffer_control_handler (txtbuf, value_type, pspec, flags));
}

/*
 * Text buffer control handler
 */

static SwamiControl *
text_buffer_control_handler (GObject *widget, GType value_type,
			     GParamSpec *pspec, SwamiguiControlFlags flags)
{
  SwamiControlFunc *control = NULL;
  GtkTextBuffer *txtbuf = GTK_TEXT_BUFFER (widget);
  GtkTextIter start, end;
  GtkTextTag *tag;

  if (!(flags & SWAMIGUI_CONTROL_NO_CREATE))
    {
      g_object_ref (widget); /* ++ ref the GtkTextBuffer for the control */

      control = swami_control_func_new ();
      swami_control_set_spec ((SwamiControl *)control,
			      g_param_spec_string ("value", "value", "value",
						   NULL, G_PARAM_READWRITE));
      swami_control_func_assign_funcs (control, text_buffer_control_get_func,
				       text_buffer_control_set_func,
				       NULL, widget);

      if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
	g_signal_connect (widget, "changed",
			G_CALLBACK (text_buffer_control_cb_changed), control);

      /* not a GtkObject (no "destroy") so we add a weak reference instead */
      g_object_weak_ref (widget, text_buffer_widget_weak_notify, control);
    }

  if (!(flags & SWAMIGUI_CONTROL_CTRL)) /* view only? */
    {				/* make text buffer read only */
      tag = gtk_text_buffer_create_tag (txtbuf, "read_only",
					"editable", FALSE,
					NULL);
      gtk_text_buffer_get_bounds (txtbuf, &start, &end);
      gtk_text_buffer_apply_tag (txtbuf, tag, &start, &end);
    }

  return (control ? SWAMI_CONTROL (control) : NULL);
}

/* GtkTextBuffer handler SwamiControlFunc get value function */
static void
text_buffer_control_get_func (SwamiControl *control, GValue *value)
{
  gpointer data;
  GtkTextBuffer *txtbuf;
  GtkTextIter start, end;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) 
    {
      txtbuf = GTK_TEXT_BUFFER (data);
      gtk_text_buffer_get_bounds (txtbuf, &start, &end);
      g_value_set_string (value, gtk_text_buffer_get_text (txtbuf, &start,
							   &end, FALSE));
    }
  SWAMI_UNLOCK_READ (control);
}

/* GtkTextBuffer handler SwamiControlFunc set value function */
static void
text_buffer_control_set_func (SwamiControl *control,
			      SwamiControlEvent *event,
			      const GValue *value)
{
  GtkTextBuffer *txtbuf = NULL;
  gpointer data;
  const char *s;

  /* minimize lock and ref the text buffer */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) txtbuf = GTK_TEXT_BUFFER (g_object_ref (data)); /* ++ ref */
  SWAMI_UNLOCK_READ (control);

  if (txtbuf)
    {
      g_signal_handlers_block_by_func (txtbuf, text_buffer_control_cb_changed,
				       control);
      s = g_value_get_string (value);
      gtk_text_buffer_set_text (txtbuf, s ? s : "", -1);
      g_signal_handlers_unblock_by_func (txtbuf,
					 text_buffer_control_cb_changed,
					 control);
      g_object_unref (txtbuf);	/* -- unref text buffer */
    }
}

/* GtkTextBuffer changed callback */
static void
text_buffer_control_cb_changed (GtkTextBuffer *txtbuf, gpointer user_data)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (user_data);
  GtkTextIter start, end;
  GValue value = { 0 };

  gtk_text_buffer_get_bounds (txtbuf, &start, &end);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value,
		      gtk_text_buffer_get_text (txtbuf, &start, &end, FALSE));
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}

/* weak notify when text buffer is destroyed */
static void
text_buffer_widget_weak_notify (gpointer data, GObject *where_the_object_was)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (data);

  SWAMI_LOCK_WRITE (control);
  SWAMI_CONTROL_FUNC_DATA (control) = NULL;
  SWAMI_UNLOCK_WRITE (control);
}

/*
 * file chooser control handler
 */

static SwamiControl *
file_chooser_control_handler (GObject *widget, GType value_type,
			      GParamSpec *pspec, SwamiguiControlFlags flags)
{
  SwamiControlFunc *control = NULL;

  if (!(flags & SWAMIGUI_CONTROL_NO_CREATE))
  {
    g_object_ref (widget);     /* ++ ref the GtkEntry for the control */

    control = swami_control_func_new ();
    swami_control_set_spec (SWAMI_CONTROL (control),
			    g_param_spec_string ("value", "value", "value",
						 NULL, G_PARAM_READWRITE));
    swami_control_func_assign_funcs (control, file_chooser_control_get_func,
				     file_chooser_control_set_func, NULL, widget);

    g_signal_connect (widget, "destroy",
		      G_CALLBACK (func_control_cb_widget_destroy), control);
  }

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
  {
    gtk_widget_set_sensitive ((GtkWidget *)widget, TRUE);

    if (control)
      g_signal_connect (widget, "file-set",
			G_CALLBACK (file_chooser_control_cb_changed), control);
  } /* not controllable */
  else gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);

  return (SWAMI_CONTROL (control));
}

/* file chooser handler SwamiControl get value function */
static void
file_chooser_control_get_func (SwamiControl *control, GValue *value)
{
  GtkFileChooser *chooser;
  gpointer data;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) chooser = GTK_FILE_CHOOSER (g_object_ref (data));	/* ++ ref file chooser */
  SWAMI_UNLOCK_READ (control);

  if (chooser)
  {
    g_value_take_string (value, gtk_file_chooser_get_filename (chooser));
    g_object_unref (chooser);			/* -- unref file chooser */
  }
}

/* file chooser handler SwamiControl set value function */
static void
file_chooser_control_set_func (SwamiControl *control, SwamiControlEvent *event,
			       const GValue *value)
{
  GtkFileChooser *chooser = NULL;
  gpointer data;
  const char *s;

  /* minimize lock and ref the file chooser */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) chooser = GTK_FILE_CHOOSER (g_object_ref (data)); /* ++ ref file chooser */
  SWAMI_UNLOCK_READ (control);

  if (chooser)
  {
    g_signal_handlers_block_by_func (chooser, file_chooser_control_cb_changed,
				     control);
    s = g_value_get_string (value);
    gtk_file_chooser_set_filename (chooser, s ? s : "");
    g_signal_handlers_unblock_by_func (chooser, file_chooser_control_cb_changed,
				       control);

    g_object_unref (chooser);	/* -- unref file chooser */
  }
}

/* callback for file chooser control to propagate text changes */
static void
file_chooser_control_cb_changed (GtkFileChooser *chooser, gpointer user_data)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (user_data);
  GValue value = { 0 };

  /* transmit the file chooser change */
  g_value_init (&value, G_TYPE_STRING);
  g_value_take_string (&value, gtk_file_chooser_get_filename (chooser));
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}

/*
 * label control handler (display only)
 */

static SwamiControl *
label_control_handler (GObject *widget, GType value_type, GParamSpec *pspec,
		       SwamiguiControlFlags flags)
{
  SwamiControlFunc *control;

  if (flags & SWAMIGUI_CONTROL_NO_CREATE)
    return (NULL);

  g_object_ref (widget);	/* ++ ref widget for control */

  control = swami_control_func_new ();
  swami_control_set_spec ((SwamiControl *)control,
			  g_param_spec_string ("value", "value", "value",
					       NULL, G_PARAM_READWRITE));
  swami_control_func_assign_funcs (control, label_control_get_func,
				   label_control_set_func, NULL, widget);

  g_signal_connect (widget, "destroy",
		    G_CALLBACK (func_control_cb_widget_destroy), control);

  return (SWAMI_CONTROL (control));
}

/* GtkLabel handler SwamiControlFunc get value function */
static void
label_control_get_func (SwamiControl *control, GValue *value)
{
  gpointer data;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) g_value_set_string (value, gtk_label_get_text (GTK_LABEL (data)));
  SWAMI_UNLOCK_READ (control);
}

/* GtkLabel handler SwamiControlFunc set value function */
static void
label_control_set_func (SwamiControl *control, SwamiControlEvent *event,
			const GValue *value)
{
  GtkLabel *label = NULL;
  gpointer data;
  const char *s;

  /* minimize lock time, ref and do the work outside of lock */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) label = GTK_LABEL (g_object_ref (data)); /* -- ref label */
  SWAMI_UNLOCK_READ (control);

  if (label)
    {
      s = g_value_get_string (value);
      gtk_label_set_text (label, s ? s : "");
      g_object_unref (label);	/* -- unref label */
    }
}

/*
 *  toggle button control handler
 */


static SwamiControl *
toggle_button_control_handler (GObject *widget, GType value_type,
			       GParamSpec *pspec,
			       SwamiguiControlFlags flags)
{
  SwamiControlFunc *control;

  if (flags & SWAMIGUI_CONTROL_NO_CREATE)
    return (NULL);

  g_object_ref (widget);	/* ++ ref button widget for control */

  control = swami_control_func_new ();
  swami_control_set_spec ((SwamiControl *)control,
			  g_param_spec_boolean ("value", "value", "value",
						FALSE, G_PARAM_READWRITE));
  swami_control_func_assign_funcs (control, toggle_button_control_get_func,
				   toggle_button_control_set_func,
				   NULL, widget);

  g_signal_connect (widget, "destroy",
		    G_CALLBACK (func_control_cb_widget_destroy), control);

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
    {
      gtk_widget_set_sensitive ((GtkWidget *)widget, TRUE);
      g_signal_connect (widget, "toggled",
			G_CALLBACK (toggle_button_control_toggled), control);
    }
  else gtk_widget_set_sensitive ((GtkWidget *)widget, FALSE);

  return (SWAMI_CONTROL (control));
}

/* toggle button handler SwamiControlFunc get value function */
static void
toggle_button_control_get_func (SwamiControl *control, GValue *value)
{
  gpointer data;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data)
    g_value_set_boolean (value, gtk_toggle_button_get_active
			 (GTK_TOGGLE_BUTTON (data)));
  SWAMI_UNLOCK_READ (control);
}

/* toggle button handler SwamiControlFunc set value function */
static void
toggle_button_control_set_func (SwamiControl *control,
				SwamiControlEvent *event,
				const GValue *value)
{
  GtkToggleButton *btn = NULL;
  gpointer data;

  /* minimize lock time - ref the button */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) btn = GTK_TOGGLE_BUTTON (g_object_ref (data)); /* ++ ref btn */
  SWAMI_UNLOCK_READ (control);

  if (btn)
    {
      g_signal_handlers_block_by_func (btn, toggle_button_control_toggled,
				       control);
      gtk_toggle_button_set_active (btn, g_value_get_boolean (value));
      g_signal_handlers_unblock_by_func (btn, toggle_button_control_toggled,
					 control);
      g_object_unref (btn);	/* -- unref button */
    }
}

/* toggle callback to transmit control change to connected controls */
static void
toggle_button_control_toggled (GtkToggleButton *btn, gpointer user_data)
{
  SwamiControlFunc *control = SWAMI_CONTROL_FUNC (user_data);
  GValue value = { 0 };

  /* transmit the toggle button change */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, gtk_toggle_button_get_active (btn));
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}


/*
 *  String combo box control handler
 */


static SwamiControl *
combo_box_string_control_handler (GObject *widget, GType value_type, GParamSpec *pspec,
				  SwamiguiControlFlags flags)
{
  SwamiControlFunc *control;

  g_return_val_if_fail (g_value_type_transformable (value_type, G_TYPE_STRING), NULL);

  if (flags & SWAMIGUI_CONTROL_NO_CREATE) return (NULL);

  g_object_ref (widget);	/* ++ ref widget for control */

  /* Create parameter spec if none or not transformable to string */
  if (!pspec || !swami_param_type_transformable (G_PARAM_SPEC_TYPE (pspec),
						 G_TYPE_PARAM_STRING))
    pspec = g_param_spec_string ("value", "value", "value",
				 NULL, G_PARAM_READWRITE);
  else g_param_spec_ref (pspec);	/* ++ ref for swami_control_set_spec */

  control = swami_control_func_new ();
  swami_control_set_value_type (SWAMI_CONTROL (control), G_TYPE_STRING);
  swami_control_set_spec ((SwamiControl *)control, pspec);
  swami_control_func_assign_funcs (control, combo_box_string_control_get_func,
				   combo_box_string_control_set_func,
				   NULL, widget);

  g_signal_connect (widget, "destroy",
		    G_CALLBACK (func_control_cb_widget_destroy), control);

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
  {
    gtk_widget_set_sensitive ((GtkWidget *)widget, TRUE);
    g_signal_connect (widget, "changed",
		      G_CALLBACK (combo_box_string_control_changed), control);
  }
  else gtk_widget_set_sensitive ((GtkWidget *)widget, FALSE);

  return (SWAMI_CONTROL (control));
}

/* string combo box handler SwamiControlFunc get value function */
static void
combo_box_string_control_get_func (SwamiControl *control, GValue *value)
{
  GtkComboBox *combo = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GParamSpec *pspec;
  gpointer data;
  char *text = NULL;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data));	/* ++ ref for outside of lock */
  pspec = swami_control_get_spec (control);	/* ++ ref param spec */
  SWAMI_UNLOCK_READ (control);

  if (combo)
  {
    if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      model = gtk_combo_box_get_model (combo);

      gtk_tree_model_get (model, &iter,
			  0, &text,		/* ++ alloc */
			  -1);
    }

    g_value_set_string (value, text);

    g_free (text);				/* -- free */
    g_object_unref (combo);			/* -- unref combo box */
  }

  g_param_spec_unref (pspec);			/* -- unref param spec */
}

/* string combo box handler SwamiControlFunc set value function */
static void
combo_box_string_control_set_func (SwamiControl *control, SwamiControlEvent *event,
				   const GValue *value)
{
  GtkComboBox *combo = NULL;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gpointer data;
  int active = -1;	/* defaults to de-selecting active combo item */
  const char *activestr;
  char *str;
  int i;

  /* minimize lock time - ref the combo box */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data)); /* ++ ref combo box */
  SWAMI_UNLOCK_READ (control);

  if (combo)
  {
    activestr = g_value_get_string (value);
    model = gtk_combo_box_get_model (combo);

    if (activestr && gtk_tree_model_get_iter_first (model, &iter))
    {
      i = 0;

      /* look for matching combo text row */
      do
      {
	gtk_tree_model_get (model, &iter,
			    0, &str,		/* ++ alloc */
			    -1);

	if (strcmp (activestr, str) == 0)
	{
	  g_free (str);				/* -- free */
	  active = i;
	  break;
	}

	g_free (str);				/* -- free */
	i++;
      }
      while (gtk_tree_model_iter_next (model, &iter));


      g_signal_handlers_block_by_func (combo, combo_box_string_control_changed, control);
      gtk_combo_box_set_active (combo, active);
      g_signal_handlers_unblock_by_func (combo, combo_box_string_control_changed, control);
    }

    g_object_unref (combo);		/* -- unref combo box */
  }
}

/* string combo box changed callback to transmit control change to connected controls */
static void
combo_box_string_control_changed (GtkComboBox *combo, gpointer user_data)
{
  SwamiControl *control = SWAMI_CONTROL (user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;
  GValue value = { 0 };
  char *str;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter))
    gtk_tree_model_get (model, &iter,
			0, &str,		/* ++ alloc */
			-1);

  /* transmit the combo box change */
  g_value_init (&value, G_TYPE_STRING);
  g_value_take_string (&value, str);		/* !! takes string ownership */
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}


/*
 *  Enum combo box control handler
 */


static SwamiControl *
combo_box_enum_control_handler (GObject *widget, GType value_type, GParamSpec *pspec,
				SwamiguiControlFlags flags)
{
  SwamiControlFunc *control;
  GEnumClass *enumklass;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter iter;
  char *s;
  int i;

  g_return_val_if_fail (G_TYPE_FUNDAMENTAL (value_type) == G_TYPE_ENUM, NULL);

  store = gtk_list_store_new (1, G_TYPE_STRING);
  gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer,
				  "text", 0, NULL);

  enumklass = g_type_class_ref (value_type);	/* ++ ref enum class */

  /* add enum items to combo box */
  for (i = 0; i < enumklass->n_values; i++)
  {
    gtk_list_store_append (store, &iter);

    s = g_strdup (enumklass->values[i].value_nick);	/* ++ alloc string dup */
    s[0] = toupper (s[0]);
    gtk_list_store_set (store, &iter, 0, _(s), -1);
    g_free (s);						/* -- free string */
  }

  g_type_class_unref (enumklass);		/* -- unref enum class */

  if (flags & SWAMIGUI_CONTROL_NO_CREATE)
    return (NULL);

  g_object_ref (widget);	/* ++ ref button widget for control */

  if (!pspec)
    pspec = g_param_spec_enum ("value", "value", "value",
			       value_type, enumklass->minimum, G_PARAM_READWRITE);
  else g_param_spec_ref (pspec);	/* ++ ref for swami_control_set_spec */

  control = swami_control_func_new ();
  swami_control_set_spec ((SwamiControl *)control, pspec);
  swami_control_func_assign_funcs (control, combo_box_enum_control_get_func,
				   combo_box_enum_control_set_func,
				   NULL, widget);

  g_signal_connect (widget, "destroy",
		    G_CALLBACK (func_control_cb_widget_destroy), control);

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
  {
    gtk_widget_set_sensitive ((GtkWidget *)widget, TRUE);
    g_signal_connect (widget, "changed",
		      G_CALLBACK (combo_box_enum_control_changed), control);
  }
  else gtk_widget_set_sensitive ((GtkWidget *)widget, FALSE);

  return (SWAMI_CONTROL (control));
}

/* enum combo box handler SwamiControlFunc get value function */
static void
combo_box_enum_control_get_func (SwamiControl *control, GValue *value)
{
  GtkComboBox *combo = NULL;
  GEnumClass *enumklass;
  GParamSpec *pspec;
  gpointer data;
  int active;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data));	/* ++ ref for outside of lock */
  pspec = swami_control_get_spec (control);	/* ++ ref param spec */
  SWAMI_UNLOCK_READ (control);

  if (combo)
  {
    enumklass = g_type_class_ref (G_PARAM_SPEC_TYPE (pspec));	/* ++ ref enum class */
    active = gtk_combo_box_get_active (combo);

    if (active != -1 && active < enumklass->n_values)
      g_value_set_enum (value, enumklass->values[active].value);

    g_type_class_unref (enumklass);		/* -- unref enum class */
    g_object_unref (combo);			/* -- unref combo box */
  }

  g_param_spec_unref (pspec);			/* -- unref param spec */
}

/* enum combo box handler SwamiControlFunc set value function */
static void
combo_box_enum_control_set_func (SwamiControl *control, SwamiControlEvent *event,
				 const GValue *value)
{
  GtkComboBox *combo = NULL;
  GEnumClass *enumklass;
  GParamSpec *pspec;
  gpointer data;
  int pos, val;
  GType type;

  /* minimize lock time - ref the combo box */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data)); /* ++ ref combo box */
  pspec = swami_control_get_spec (control);	/* ++ ref param spec */
  SWAMI_UNLOCK_READ (control);

  type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  g_param_spec_unref (pspec);			/* -- unref param spec */

  g_return_if_fail (G_TYPE_FUNDAMENTAL (type) == G_TYPE_ENUM);

  if (combo)
  {
    enumklass = g_type_class_ref (type);	/* ++ ref enum class */
    val = g_value_get_enum (value);

    for (pos = 0; pos < enumklass->n_values; pos++)
      if (enumklass->values[pos].value == val) break;

    if (pos < enumklass->n_values)
    {
      g_signal_handlers_block_by_func (combo, combo_box_enum_control_changed, control);
      gtk_combo_box_set_active (combo, pos);
      g_signal_handlers_unblock_by_func (combo, combo_box_enum_control_changed, control);
    }

    g_type_class_unref (enumklass);	/* -- unref enum class */
    g_object_unref (combo);		/* -- unref combo box */
  }
}

/* enum combo box changed callback to transmit control change to connected controls */
static void
combo_box_enum_control_changed (GtkComboBox *combo, gpointer user_data)
{
  SwamiControl *control = SWAMI_CONTROL (user_data);
  GEnumClass *enumklass;
  GParamSpec *pspec;
  GValue value = { 0 };
  GType type;
  int active;

  pspec = swami_control_get_spec (control);	/* ++ ref param spec */

  type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  g_return_if_fail (G_TYPE_FUNDAMENTAL (type) == G_TYPE_ENUM);

  enumklass = g_type_class_ref (type);	/* ++ ref enum class */
  active = gtk_combo_box_get_active (combo);

  /* transmit the combo box change */
  if (active < enumklass->n_values)
  {
    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    g_value_set_enum (&value, enumklass->values[active].value);
    swami_control_transmit_value ((SwamiControl *)control, &value);
    g_value_unset (&value);
  }

  g_type_class_unref (enumklass);		/* -- unref enum class */
  g_param_spec_unref (pspec);			/* -- unref param spec */
}


/*
 *  GType combo box control handler
 */


static SwamiControl *
combo_box_gtype_control_handler (GObject *widget, GType value_type, GParamSpec *pspec,
				 SwamiguiControlFlags flags)
{
  SwamiControlFunc *control;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter iter;
  GType basetype;
  GType *types;
  char *s;
  int i;

  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_GTYPE, NULL);

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_GTYPE);
  gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer,
				  "text", 0, NULL);

  basetype = ((GParamSpecGType *)pspec)->is_a_type;

  /* ++ alloc list of child types of base GType of parameter spec */
  types = swami_util_get_child_types (basetype, NULL);

  /* add types to combo box */
  for (i = 0; types[i]; i++)
  {
    gtk_list_store_append (store, &iter);

    s = g_strdup (g_type_name (types[i]));		/* ++ alloc string */
    gtk_list_store_set (store, &iter,
			0, s,
			1, types[i],
			-1);
    g_free (s);						/* -- free string */
  }

  g_free (types);					/* -- free type array */

  if (flags & SWAMIGUI_CONTROL_NO_CREATE)
    return (NULL);

  g_object_ref (widget);	/* ++ ref button widget for control */

  if (!pspec)
    pspec = g_param_spec_gtype ("value", "value", "value",
			        basetype, G_PARAM_READWRITE);
  else g_param_spec_ref (pspec);	/* ++ ref for swami_control_set_spec */

  control = swami_control_func_new ();
  swami_control_set_spec ((SwamiControl *)control, pspec);
  swami_control_func_assign_funcs (control, combo_box_gtype_control_get_func,
				   combo_box_gtype_control_set_func,
				   NULL, widget);

  g_signal_connect (widget, "destroy",
		    G_CALLBACK (func_control_cb_widget_destroy), control);

  if (flags & SWAMIGUI_CONTROL_CTRL) /* controllable? */
  {
    gtk_widget_set_sensitive ((GtkWidget *)widget, TRUE);
    g_signal_connect (widget, "changed",
		      G_CALLBACK (combo_box_gtype_control_changed), control);
  }
  else gtk_widget_set_sensitive ((GtkWidget *)widget, FALSE);

  return (SWAMI_CONTROL (control));
}

/* GType combo box handler SwamiControlFunc get value function */
static void
combo_box_gtype_control_get_func (SwamiControl *control, GValue *value)
{
  GtkComboBox *combo = NULL;
  GtkTreeIter iter;
  GParamSpec *pspec;
  gpointer data;
  GType type;

  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data));	/* ++ ref for outside of lock */
  pspec = swami_control_get_spec (control);	/* ++ ref param spec */
  SWAMI_UNLOCK_READ (control);

  if (combo)
  {
    if (gtk_combo_box_get_active_iter (combo, &iter))
    { /* get the GType value stored for the active item */
      gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter, 1, &type, -1);
      g_value_set_gtype (value, type);
    }

    g_object_unref (combo);			/* -- unref combo box */
  }

  g_param_spec_unref (pspec);			/* -- unref param spec */
}

/* GType combo box handler SwamiControlFunc set value function */
static void
combo_box_gtype_control_set_func (SwamiControl *control, SwamiControlEvent *event,
				  const GValue *value)
{
  GtkComboBox *combo = NULL;
  GtkTreeModel *model;
  gpointer data;
  GtkTreeIter iter;
  GType valtype, type;
  gboolean type_in_list = FALSE;

  /* minimize lock time - ref the combo box */
  SWAMI_LOCK_READ (control);
  data = SWAMI_CONTROL_FUNC_DATA (control);
  if (data) combo = GTK_COMBO_BOX (g_object_ref (data)); /* ++ ref combo box */
  SWAMI_UNLOCK_READ (control);

  if (!combo) return;

  model = gtk_combo_box_get_model (combo);

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    valtype = g_value_get_gtype (value);

    /* look for matching combo box item by GType */
    do
    {
      gtk_tree_model_get (model, &iter, 1, &type, -1);

      if (valtype == type)
      {
	g_signal_handlers_block_by_func (combo, combo_box_gtype_control_changed,
					 control);
	gtk_combo_box_set_active_iter (combo, &iter);
	g_signal_handlers_unblock_by_func (combo, combo_box_gtype_control_changed,
					   control);
	type_in_list = TRUE;
	break;
      }
    }
    while (gtk_tree_model_iter_next (model, &iter));
  }

  g_object_unref (combo);		/* -- unref combo box */

  g_return_if_fail (type_in_list);
}

/* GType combo box changed callback to transmit control change to connected controls */
static void
combo_box_gtype_control_changed (GtkComboBox *combo, gpointer user_data)
{
  SwamiControl *control = SWAMI_CONTROL (user_data);
  GtkTreeModel *model;
  GValue value = { 0 };
  GtkTreeIter iter;
  GType type;

  model = gtk_combo_box_get_model (combo);
  if (!gtk_combo_box_get_active_iter (combo, &iter)) return;

  /* get GType of active combo box item */
  gtk_tree_model_get (model, &iter, 1, &type, -1);

  /* transmit the combo box change */
  g_value_init (&value, G_TYPE_GTYPE);
  g_value_set_gtype (&value, type);
  swami_control_transmit_value ((SwamiControl *)control, &value);
  g_value_unset (&value);
}
