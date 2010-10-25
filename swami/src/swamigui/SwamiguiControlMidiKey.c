/*
 * SwamiguiControlMidiKey.c - MIDI keyboard control
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

/*
 * NOTE: Setting key arrays is NOT multi-thread safe!
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include <gdk/gdkkeysyms.h>

#include "SwamiguiControlMidiKey.h"
#include "SwamiguiRoot.h"

/* Some default values */
#define DEFAULT_LOWER_OCTAVE    2
#define DEFAULT_UPPER_OCTAVE    DEFAULT_LOWER_OCTAVE + 1

enum
{
  PROP_0,
  PROP_LOWER_OCTAVE,
  PROP_UPPER_OCTAVE,
  PROP_JOIN_OCTAVES,
  PROP_LOWER_VELOCITY,
  PROP_UPPER_VELOCITY,
  PROP_SAME_VELOCITY,
  PROP_LOWER_CHANNEL,
  PROP_UPPER_CHANNEL
};

typedef struct
{
  guint key;		    /* GDK key (see gdk/gdkkeysym.h header) */
  gint8 active_note;	    /* active MIDI note or -1 if not active */
  gint8 active_chan;	 /* MIDI channel of active note (if not -1) */
} MidiKey;

static void swamigui_control_midi_key_get_property (GObject *object,
						   guint property_id,
						   GValue *value,
						   GParamSpec *pspec);
static void swamigui_control_midi_key_set_property (GObject *object,
						   guint property_id,
						   const GValue *value,
						   GParamSpec *pspec);
static gint swamigui_control_midi_key_snooper (GtkWidget *grab_widget,
					       GdkEventKey *event,
					       gpointer func_data);
static void swamigui_control_midi_key_finalize (GObject *object);


/* FIXME - Dynamically determine key map */

/* default lower keys */
static guint default_lower_keys[] =
{
  GDK_z, GDK_s, GDK_x, GDK_d, GDK_c, GDK_v, GDK_g, GDK_b, GDK_h, GDK_n, GDK_j,
  GDK_m, GDK_comma, GDK_l, GDK_period, GDK_semicolon, GDK_slash
};
 
/* default upper keys */
static guint default_upper_keys[] =
{
  GDK_q, GDK_2, GDK_w, GDK_3, GDK_e, GDK_r, GDK_5, GDK_t, GDK_6, GDK_y, GDK_7,
  GDK_u, GDK_i, GDK_9, GDK_o, GDK_0, GDK_p, GDK_bracketleft, GDK_equal,
  GDK_bracketright
};

/* Widget types to ignore key presses from (text entries, etc) */
static char *ignore_widget_type_names[] = {
  "GtkEntry",
  "GtkTextView"
};

/* Allocated and resolved type array, G_TYPE_INVALID terminated */
static GType *ignore_widget_types;


G_DEFINE_TYPE (SwamiguiControlMidiKey, swamigui_control_midi_key,
               SWAMI_TYPE_CONTROL_MIDI);


static void
swamigui_control_midi_key_class_init (SwamiguiControlMidiKeyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  int i;

  obj_class->set_property = swamigui_control_midi_key_set_property;
  obj_class->get_property = swamigui_control_midi_key_get_property;
  obj_class->finalize = swamigui_control_midi_key_finalize;

  g_object_class_install_property (obj_class, PROP_LOWER_OCTAVE,
      g_param_spec_int ("lower-octave", "Lower octave", "Lower keyboard MIDI octave",
                        -2, 8, DEFAULT_LOWER_OCTAVE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_OCTAVE,
      g_param_spec_int ("upper-octave", "Upper octave", "Upper keyboard MIDI octave",
                        -2, 8, DEFAULT_UPPER_OCTAVE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_JOIN_OCTAVES,
      g_param_spec_boolean ("join-octaves", "Join octaves", "Join upper and lower octaves",
                            TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_VELOCITY,
      g_param_spec_int ("lower-velocity", "Lower velocity", "Lower keyboard MIDI velocity",
                        1, 127, 127, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_VELOCITY,
      g_param_spec_int ("upper-velocity", "Upper velocity", "Upper keyboard MIDI velocity",
                        1, 127, 127, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_SAME_VELOCITY,
      g_param_spec_boolean ("same-velocity", "Same velocity", "Same velocity for upper and lower",
                            TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_CHANNEL,
      g_param_spec_int ("lower-channel", "Lower channel", "Lower keyboard MIDI channel",
                        0, 15, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_CHANNEL,
      g_param_spec_int ("upper-channel", "Upper channel", "Upper keyboard MIDI channel",
                        0, 15, 0, G_PARAM_READWRITE));

  ignore_widget_types = g_new (GType, G_N_ELEMENTS (ignore_widget_type_names) + 1);

  for (i = 0; i < G_N_ELEMENTS (ignore_widget_type_names); i++)
    ignore_widget_types[i] = g_type_from_name (ignore_widget_type_names[i]);

  ignore_widget_types[i] = G_TYPE_INVALID;
}

static void
swamigui_control_midi_key_get_property (GObject *object, guint property_id,
				        GValue *value, GParamSpec *pspec)
{
  SwamiguiControlMidiKey *keyctrl = SWAMIGUI_CONTROL_MIDI_KEY (object);

  switch (property_id)
    {
    case PROP_LOWER_OCTAVE:
      g_value_set_int (value, keyctrl->lower_octave);
      break;
    case PROP_UPPER_OCTAVE:
      g_value_set_int (value, keyctrl->upper_octave);
      break;
    case PROP_JOIN_OCTAVES:
      g_value_set_boolean (value, keyctrl->join_octaves);
      break;
    case PROP_LOWER_VELOCITY:
      g_value_set_int (value, keyctrl->lower_velocity);
      break;
    case PROP_UPPER_VELOCITY:
      g_value_set_int (value, keyctrl->upper_velocity);
      break;
    case PROP_SAME_VELOCITY:
      g_value_set_boolean (value, keyctrl->same_velocity);
      break;
    case PROP_LOWER_CHANNEL:
      g_value_set_int (value, keyctrl->lower_channel);
      break;
    case PROP_UPPER_CHANNEL:
      g_value_set_int (value, keyctrl->upper_channel);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_control_midi_key_set_property (GObject *object, guint property_id,
				        const GValue *value, GParamSpec *pspec)
{
  SwamiguiControlMidiKey *keyctrl = SWAMIGUI_CONTROL_MIDI_KEY (object);

  switch (property_id)
    {
    case PROP_LOWER_OCTAVE:
      keyctrl->lower_octave = g_value_get_int (value);

      if (keyctrl->join_octaves)
      {
        keyctrl->upper_octave = keyctrl->lower_octave < 8 ? keyctrl->lower_octave + 1 : 8;
        g_object_notify (object, "upper-octave");
      }
      break;
    case PROP_UPPER_OCTAVE:
      keyctrl->upper_octave = g_value_get_int (value);

      if (keyctrl->join_octaves)
      {
        keyctrl->lower_octave = keyctrl->upper_octave > -2 ? keyctrl->upper_octave - 1 : -2;
        g_object_notify (object, "lower-octave");
      }
      break;
    case PROP_JOIN_OCTAVES:
      keyctrl->join_octaves = g_value_get_boolean (value);

      if (keyctrl->join_octaves
          && keyctrl->lower_octave + 1 != keyctrl->upper_octave)
      {
        keyctrl->upper_octave = keyctrl->lower_octave < 8 ? keyctrl->lower_octave + 1 : 8;
        g_object_notify (object, "upper-octave");
      }
      break;
    case PROP_LOWER_VELOCITY:
      keyctrl->lower_velocity = g_value_get_int (value);

      if (keyctrl->same_velocity)
      {
        keyctrl->upper_velocity = keyctrl->lower_velocity;
        g_object_notify (object, "upper-velocity");
      }
      break;
    case PROP_UPPER_VELOCITY:
      keyctrl->upper_velocity = g_value_get_int (value);

      if (keyctrl->same_velocity)
      {
        keyctrl->lower_velocity = keyctrl->upper_velocity;
        g_object_notify (object, "lower-velocity");
      }
      break;
    case PROP_SAME_VELOCITY:
      keyctrl->same_velocity = g_value_get_boolean (value);

      if (keyctrl->same_velocity
          && keyctrl->lower_velocity != keyctrl->upper_velocity)
      {
        keyctrl->upper_velocity = keyctrl->lower_velocity;
        g_object_notify (object, "upper-velocity");
      }
      break;
    case PROP_LOWER_CHANNEL:
      keyctrl->lower_channel = g_value_get_int (value);
      break;
    case PROP_UPPER_CHANNEL:
      keyctrl->upper_channel = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_control_midi_key_init (SwamiguiControlMidiKey *keyctrl)
{
  MidiKey midikey;
  int i;

  swami_control_set_flags (SWAMI_CONTROL (keyctrl), SWAMI_CONTROL_SENDS);

  keyctrl->lower_keys = g_array_new (FALSE, FALSE, sizeof (MidiKey));
  keyctrl->upper_keys = g_array_new (FALSE, FALSE, sizeof (MidiKey));
  keyctrl->lower_octave = DEFAULT_LOWER_OCTAVE;
  keyctrl->upper_octave = DEFAULT_UPPER_OCTAVE;
  keyctrl->join_octaves = TRUE;
  keyctrl->lower_velocity = 127;
  keyctrl->upper_velocity = 127;
  keyctrl->same_velocity = TRUE;
  keyctrl->lower_channel = 0;
  keyctrl->upper_channel = 0;

  /* initialize lower and upper key array */

  for (i = 0; i < G_N_ELEMENTS (default_lower_keys); i++)
    {
      midikey.key = default_lower_keys[i];
      midikey.active_note = -1;
      g_array_append_val (keyctrl->lower_keys, midikey);
    }

  for (i = 0; i < G_N_ELEMENTS (default_upper_keys); i++)
    {
      midikey.key = default_upper_keys[i];
      midikey.active_note = -1;
      g_array_append_val (keyctrl->upper_keys, midikey);
    }

  /* install our key snooper */
  keyctrl->snooper_id =
    gtk_key_snooper_install (swamigui_control_midi_key_snooper, keyctrl);
}

static gint
swamigui_control_midi_key_snooper (GtkWidget *grab_widget, GdkEventKey *event,
				   gpointer func_data)
{
  SwamiguiControlMidiKey *keyctrl = SWAMIGUI_CONTROL_MIDI_KEY (func_data);
  GtkWidget *toplevel, *focus;
  GType *ignore;
  guint key;

  if (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) return (FALSE);

  key = gdk_keyval_to_lower (event->keyval);

  if (event->type == GDK_KEY_PRESS)
  {
    toplevel = gtk_widget_get_toplevel (grab_widget);
    
    /* Only listen to key presses on main window, not dialogs (allow releases though) */
    if (toplevel != swamigui_root->main_window)
      return (FALSE);

    /* Check if focus widget type should be ignored (text entries, etc) */
    focus = gtk_window_get_focus (GTK_WINDOW (toplevel));

    if (focus)
    {
      for (ignore = ignore_widget_types; *ignore != G_TYPE_INVALID; ignore++)
      {
        if (g_type_is_a (G_OBJECT_TYPE (focus), *ignore))
	  return (FALSE);
      }
    }

    swamigui_control_midi_key_press (keyctrl, key);
  }
  else if (event->type == GDK_KEY_RELEASE)
    swamigui_control_midi_key_release (keyctrl, key);

  return (FALSE);	/* don't stop event propagation */
}

static void
swamigui_control_midi_key_finalize (GObject *object)
{
  SwamiguiControlMidiKey *keyctrl = SWAMIGUI_CONTROL_MIDI_KEY (object);

  gtk_key_snooper_remove (keyctrl->snooper_id);

  g_array_free (keyctrl->lower_keys, TRUE);
  g_array_free (keyctrl->upper_keys, TRUE);

  if (G_OBJECT_CLASS (swamigui_control_midi_key_parent_class)->finalize)
    G_OBJECT_CLASS (swamigui_control_midi_key_parent_class)->finalize (object);
}

/**
 * swamigui_control_midi_key_new:
 *
 * Create a new MIDI keyboard control.
 *
 * Returns: New MIDI keyboard control with a refcount of 1 which the
 * caller owns.
 */
SwamiguiControlMidiKey *
swamigui_control_midi_key_new (void)
{
  return (SWAMIGUI_CONTROL_MIDI_KEY
	  (g_object_new (SWAMIGUI_TYPE_CONTROL_MIDI_KEY, NULL)));
}

/**
 * swamigui_control_midi_key_press:
 * @keyctrl: MIDI keyboard control
 * @key: GDK keyboard key (see gdk/gdkkeysyms.h header)
 *
 * Send a key press to a MIDI keyboard control.
 */
void
swamigui_control_midi_key_press (SwamiguiControlMidiKey *keyctrl, guint key)
{
  MidiKey *midikey, *found = NULL;
  int newnote, newvel, newchan;
  int i, count;

  g_return_if_fail (SWAMIGUI_IS_CONTROL_MIDI_KEY (keyctrl));

  /* look for key in lower octave */
  count = keyctrl->lower_keys->len;
  for (i = 0; i < count; i++)
    {
      midikey = &g_array_index (keyctrl->lower_keys, MidiKey, i);
      if (midikey->key == key)	/* found key? */
	{
	  newnote = (keyctrl->lower_octave + 2) * 12 + i; /* calculate MIDI note */
	  newvel = keyctrl->lower_velocity;
	  newchan = keyctrl->lower_channel;
	  found = midikey;
	  break;
	}
    }

  if (!found)			/* not found in lower octave? */
    { /* look for key in upper octave */
      count = keyctrl->upper_keys->len;
      for (i = 0; i < count; i++)
	{
	  midikey = &g_array_index (keyctrl->upper_keys, MidiKey, i);
	  if (midikey->key == key)	/* found key? */
	    {
	      newnote = (keyctrl->upper_octave + 2) * 12 + i; /* calc MIDI note */
	      newvel = keyctrl->upper_velocity;
	      newchan = keyctrl->upper_channel;
	      found = midikey;
	      break;
	    }
	}
    }

  /* return if key not found or already active and the same as new note */
  if (!found || found->active_note == newnote) return;

  if (found->active_note != -1)	/* previous active note? */
    { /* send note off event */
      swami_control_midi_transmit (SWAMI_CONTROL_MIDI (keyctrl),
				   SWAMI_MIDI_NOTE_OFF, found->active_chan,
				   found->active_note, 0x7F);
      found->active_note = -1;
    }

  if (newnote <= 127)
    { /* send note on event */
      swami_control_midi_transmit (SWAMI_CONTROL_MIDI (keyctrl),
				   SWAMI_MIDI_NOTE_ON, newchan,
				   newnote, newvel);
      found->active_note = newnote;
      found->active_chan = newchan;
    }
}

/**
 * swamigui_control_midi_key_release:
 * @keyctrl: MIDI keyboard control
 * @key: GDK keyboard key (see gdk/gdkkeysyms.h header)
 *
 * Send a key release to a MIDI keyboard control.
 */
void
swamigui_control_midi_key_release (SwamiguiControlMidiKey *keyctrl, guint key)
{
  MidiKey *midikey, *found = NULL;
  int i, count;

  g_return_if_fail (SWAMIGUI_IS_CONTROL_MIDI_KEY (keyctrl));

  /* look for key in lower octave */
  count = keyctrl->lower_keys->len;
  for (i = 0; i < count; i++)
    {
      midikey = &g_array_index (keyctrl->lower_keys, MidiKey, i);
      if (midikey->key == key)	/* found key? */
	{
	  found = midikey;
	  break;
	}
    }

  if (!found)			/* not found in lower octave? */
    { /* look for key in upper octave */
      count = keyctrl->upper_keys->len;
      for (i = 0; i < count; i++)
	{
	  midikey = &g_array_index (keyctrl->upper_keys, MidiKey, i);
	  if (midikey->key == key)	/* found key? */
	    {
	      found = midikey;
	      break;
	    }
	}
    }

  if (!found || found->active_note == -1) return;

  /* send note off event */
  swami_control_midi_transmit (SWAMI_CONTROL_MIDI (keyctrl),
			       SWAMI_MIDI_NOTE_OFF, found->active_chan,
			       found->active_note, 0x7F);
  found->active_note = -1;
}

/**
 * swamigui_control_midi_key_set_lower:
 * @keyctrl: MIDI keyboard control
 * @keys: Array of GDK key values
 * @count: Number of values in @keys
 * 
 * Set the lower keyboard key array.
 *
 * Note: Not multi-thread safe
 */
void
swamigui_control_midi_key_set_lower (SwamiguiControlMidiKey *keyctrl,
				    const guint *keys, guint count)
{
  MidiKey midikey;
  int i;

  g_return_if_fail (SWAMIGUI_IS_CONTROL_MIDI_KEY (keyctrl));
  g_return_if_fail (keys != NULL || count == 0);

  g_array_set_size (keyctrl->lower_keys, 0);

  for (i = 0; i < count; i++)
    {
      midikey.key = keys[i];
      midikey.active_note = -1;
      g_array_append_val (keyctrl->lower_keys, midikey);
    }
}

/**
 * swamigui_control_midi_key_set_upper:
 * @keyctrl: MIDI keyboard control
 * @keys: Array of GDK key values
 * @count: Number of values in @keys
 * 
 * Set the upper keyboard key array.
 *
 * Note: Not multi-thread safe
 */
void
swamigui_control_midi_key_set_upper (SwamiguiControlMidiKey *keyctrl,
				    const guint *keys, guint count)
{
  MidiKey midikey;
  int i;

  g_return_if_fail (SWAMIGUI_IS_CONTROL_MIDI_KEY (keyctrl));
  g_return_if_fail (keys != NULL || count == 0);

  g_array_set_size (keyctrl->upper_keys, 0);

  for (i = 0; i < count; i++)
    {
      midikey.key = keys[i];
      midikey.active_note = -1;
      g_array_append_val (keyctrl->upper_keys, midikey);
    }
}
