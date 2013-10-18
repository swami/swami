/*
 * SwamiguiPiano.c - Piano canvas item
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
#include <gtk/gtk.h>

#include <libswami/libswami.h>

#include "SwamiguiPiano.h"
#include "SwamiguiRoot.h"
#include "SwamiguiStatusbar.h"
#include "i18n.h"
#include "util.h"

/* Piano keys: C C# D D# E F F# G G# A A# B */

enum
{
  PROP_0,
  PROP_WIDTH_PIXELS,		/* width in pixels */
  PROP_HEIGHT_PIXELS,		/* height in pixels */
  PROP_KEY_COUNT,		/* number of keys in piano */
  PROP_START_OCTAVE,		/* piano start octave (first key) */
  PROP_LOWER_OCTAVE,		/* lower keyboard octave */
  PROP_UPPER_OCTAVE,		/* upper keyboard octave */
  PROP_LOWER_VELOCITY,		/* lower keyboard velocity */
  PROP_UPPER_VELOCITY,		/* upper keyboard velocity */
  PROP_MIDI_CONTROL,		/* Piano MIDI control */
  PROP_EXPRESS_CONTROL,		/* Piano expression control */
  PROP_MIDI_CHANNEL,		/* MIDI channel to send events on */
  PROP_BG_COLOR,		/* color of border and in between white keys */
  PROP_WHITE_KEY_COLOR,		/* color of white keys */
  PROP_BLACK_KEY_COLOR,		/* color of black keys */
  PROP_SHADOW_EDGE_COLOR,	/* color of white key shadow edge */
  PROP_WHITE_KEY_PLAY_COLOR,	/* color of white key play highlight color */
  PROP_BLACK_KEY_PLAY_COLOR	/* color of black key play highlight color */
};

enum
{
  NOTE_ON,
  NOTE_OFF,
  LAST_SIGNAL
};

#define DEFAULT_BG_COLOR		GNOME_CANVAS_COLOR (0, 0, 0)
#define DEFAULT_WHITE_KEY_COLOR		GNOME_CANVAS_COLOR (255, 255, 255)
#define DEFAULT_BLACK_KEY_COLOR		GNOME_CANVAS_COLOR (0, 0, 0)
#define DEFAULT_SHADOW_EDGE_COLOR	GNOME_CANVAS_COLOR (128, 128, 128)
#define DEFAULT_WHITE_KEY_PLAY_COLOR	GNOME_CANVAS_COLOR (169, 127, 255)
#define DEFAULT_BLACK_KEY_PLAY_COLOR	GNOME_CANVAS_COLOR (169, 127, 255)

/* piano vertical line width to white key width scale */
#define PIANO_VLINE_TO_KEY_SCALE  (1.0/5.0)

/* piano horizontal line width to piano height scale */
#define PIANO_HLINE_TO_HEIGHT_SCALE (1.0/48.0)

/* piano black key height to piano height */
#define PIANO_BLACK_TO_HEIGHT_SCALE (26.0/48.0)

/* piano white key grey edge to piano height scale */
#define PIANO_GREY_TO_HEIGHT_SCALE (2.0/48.0)

/* piano white key indicator width to key scale */
#define PIANO_WHITE_INDICATOR_WIDTH_SCALE (4.0/8.0)

/* piano white key indicator height to piano height scale */
#define PIANO_WHITE_INDICATOR_HEIGHT_SCALE (2.0/48.0)

/* piano white key velocity indicator range to height scale */
#define PIANO_WHITE_INDICATOR_RANGE_SCALE (18.0/48.0)

/* piano white key velocity offset to height scale */
#define PIANO_WHITE_INDICATOR_OFS_SCALE (28.0/48.0)


/* piano black key indicator width to key scale */
#define PIANO_BLACK_INDICATOR_WIDTH_SCALE (3.0/5.0)

/* piano black key indicator height to piano height scale */
#define PIANO_BLACK_INDICATOR_HEIGHT_SCALE (3.0/48.0)

/* piano black key velocity indicator range scale */
#define PIANO_BLACK_INDICATOR_RANGE_SCALE (22.0/28.0)

/* piano black key velocity offset scale */
#define PIANO_BLACK_INDICATOR_OFS_SCALE (2.0/28.0)

/* piano active black key shorten scale (to look pressed down) */
#define PIANO_BLACK_SHORTEN_SCALE (1.0/26.0)


/* a structure used for each key of a piano */
typedef struct
{
  GnomeCanvasItem *item; /* canvas item for keys (black and white) */
  GnomeCanvasItem *indicator; /* indicator item (if set note is active) */
  GnomeCanvasItem *shadow;	/* bottom shadow edge for white keys */
} KeyInfo;

static void swamigui_piano_class_init (SwamiguiPianoClass *klass);
static void swamigui_piano_set_property (GObject *object, guint property_id,
					const GValue *value,
					GParamSpec *pspec);
static void swamigui_piano_get_property (GObject *object, guint property_id,
					GValue *value, GParamSpec *pspec);
static void swamigui_piano_init (SwamiguiPiano *piano);
static void swamigui_piano_finalize (GObject *object);
static void swamigui_piano_midi_ctrl_callback (SwamiControl *control,
					      SwamiControlEvent *event,
					      const GValue *value);

static void swamigui_piano_item_realize (GnomeCanvasItem *item);
static void swamigui_piano_draw (SwamiguiPiano *piano);
static void swamigui_piano_destroy_keys (SwamiguiPiano *piano);
static void swamigui_piano_update_key_colors (SwamiguiPiano *piano);
static void swamigui_piano_draw_noteon (SwamiguiPiano *piano, int note,
				       int velocity);
static void swamigui_piano_draw_noteoff (SwamiguiPiano *piano, int note);
static void swamigui_piano_update_mouse_note (SwamiguiPiano *piano,
					     double x, double y);
static gboolean swamigui_piano_cb_event (GnomeCanvasItem *item, GdkEvent *event,
					gpointer data);
static gboolean swamigui_piano_note_on_internal (SwamiguiPiano *piano, int note,
						int velocity);
static gboolean swamigui_piano_note_off_internal (SwamiguiPiano *piano, int note,
						 int velocity);


static GObjectClass *parent_class = NULL;
static guint piano_signals[LAST_SIGNAL] = {0};

GType
swamigui_piano_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiPianoClass), NULL, NULL,
	  (GClassInitFunc) swamigui_piano_class_init, NULL, NULL,
	  sizeof (SwamiguiPiano), 0,
	  (GInstanceInitFunc) swamigui_piano_init,
	};

      obj_type = g_type_register_static (GNOME_TYPE_CANVAS_GROUP,
					 "SwamiguiPiano", &obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_piano_class_init (SwamiguiPianoClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GnomeCanvasItemClass *canvas_item_class = GNOME_CANVAS_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_piano_set_property;
  obj_class->get_property = swamigui_piano_get_property;
  obj_class->finalize = swamigui_piano_finalize;

  canvas_item_class->realize = swamigui_piano_item_realize;

  piano_signals[NOTE_ON] =
    g_signal_new ("note-on", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (SwamiguiPianoClass, note_on), NULL, NULL,
		  g_cclosure_marshal_VOID__UINT,
		  G_TYPE_NONE, 1, G_TYPE_UINT);

  piano_signals[NOTE_OFF] =
    g_signal_new ("note-off", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (SwamiguiPianoClass, note_off), NULL, NULL,
		  g_cclosure_marshal_VOID__UINT,
		  G_TYPE_NONE, 1, G_TYPE_UINT);

  g_object_class_install_property (obj_class, PROP_WIDTH_PIXELS,
			g_param_spec_int ("width-pixels", _("Pixel Width"),
					  _("Width in pixels"),
					  1, G_MAXINT,
					  SWAMIGUI_PIANO_DEFAULT_WIDTH,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_HEIGHT_PIXELS,
			g_param_spec_int ("height-pixels", _("Pixel Height"),
					  _("Height in pixels"),
					  1, G_MAXINT,
					  SWAMIGUI_PIANO_DEFAULT_HEIGHT,
					  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_KEY_COUNT,
			g_param_spec_int ("key-count", _("Key Count"),
					  _("Size of piano in keys"),
					  1, 128, 128,
					  G_PARAM_READWRITE
					  | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (obj_class, PROP_START_OCTAVE,
		g_param_spec_int ("start-octave", _("Start Octave"),
				  _("Piano start octave (0 = MIDI note 0"),
				  0, 10, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_OCTAVE,
		g_param_spec_int ("lower-octave", _("Lower Octave"),
				  _("Lower keyboard start octave"),
				  0, 10, SWAMIGUI_PIANO_DEFAULT_LOWER_OCTAVE,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_OCTAVE,
		g_param_spec_int ("upper-octave", _("Upper Octave"),
				  _("Upper keyboard start octave"),
				  0, 10, SWAMIGUI_PIANO_DEFAULT_UPPER_OCTAVE,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LOWER_VELOCITY,
		g_param_spec_int ("lower-velocity", _("Lower Velocity"),
				  _("Lower keyboard velocity"),
				  0, 127, 127,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_UPPER_VELOCITY,
		g_param_spec_int ("upper-velocity", _("Upper Velocity"),
				  _("Upper keyboard velocity"),
				  0, 127, 127,
				  G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_MIDI_CONTROL,
		g_param_spec_object ("midi-control", _("MIDI Control"),
				     _("Piano MIDI control"),
				     SWAMI_TYPE_CONTROL,
				     G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_EXPRESS_CONTROL,
	g_param_spec_object ("expression-control",
			     _("Expression Control"),
			_("Piano expression control (vertical mouse axis)"),
			     SWAMI_TYPE_CONTROL,
			     G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_MIDI_CHANNEL,
		g_param_spec_int ("midi-channel", _("MIDI Channel"),
				  _("MIDI channel to send events on"),
				  0, 15, 0, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_BG_COLOR,
    ipatch_param_set (g_param_spec_uint ("bg-color", _("Background color"),
					 _("Color of border and between white keys"),
					 0, G_MAXUINT, DEFAULT_BG_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_WHITE_KEY_COLOR,
    ipatch_param_set (g_param_spec_uint ("white-key-color", _("White key color"),
					 _("White key color"),
					 0, G_MAXUINT, DEFAULT_WHITE_KEY_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_BLACK_KEY_COLOR,
    ipatch_param_set (g_param_spec_uint ("black-key-color", _("Black key color"),
					 _("Black key color"),
					 0, G_MAXUINT, DEFAULT_BLACK_KEY_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_SHADOW_EDGE_COLOR,
    ipatch_param_set (g_param_spec_uint ("shadow-edge-color", _("Shadow edge color"),
					 _("Bottom shadow edge color"),
					 0, G_MAXUINT, DEFAULT_SHADOW_EDGE_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_WHITE_KEY_PLAY_COLOR,
    ipatch_param_set (g_param_spec_uint ("white-key-play-color",
					 _("White key player color"),
					 _("Color of white key play highlight"),
					 0, G_MAXUINT, DEFAULT_WHITE_KEY_PLAY_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
  g_object_class_install_property (obj_class, PROP_BLACK_KEY_PLAY_COLOR,
    ipatch_param_set (g_param_spec_uint ("black-key-play-color",
					 _("Black key play color"),
					 _("Color of black key play highlight"),
					 0, G_MAXUINT, DEFAULT_BLACK_KEY_PLAY_COLOR,
					 G_PARAM_READWRITE),
		      "unit-type", SWAMIGUI_UNIT_RGBA_COLOR, NULL));
}

static void
swamigui_piano_set_property (GObject *object, guint property_id,
			    const GValue *value, GParamSpec *pspec)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  switch (property_id)
    {
    case PROP_WIDTH_PIXELS:
      piano->width = g_value_get_int (value);
      piano->up2date = FALSE;
      swamigui_piano_draw (piano);
      break;
    case PROP_HEIGHT_PIXELS:
      piano->height = g_value_get_int (value);
      piano->up2date = FALSE;
      swamigui_piano_draw (piano);
      break;
    case PROP_KEY_COUNT:
      piano->key_count = g_value_get_int (value);
      swamigui_piano_destroy_keys (piano);
      swamigui_piano_draw (piano);
      break;
    case PROP_START_OCTAVE:
      piano->start_note = g_value_get_int (value) * 12;
      break;
    case PROP_LOWER_OCTAVE:
      piano->lower_octave = g_value_get_int (value);
      break;
    case PROP_UPPER_OCTAVE:
      piano->upper_octave = g_value_get_int (value);
      break;
    case PROP_LOWER_VELOCITY:
      piano->lower_velocity = g_value_get_int (value);
      break;
    case PROP_UPPER_VELOCITY:
      piano->upper_velocity = g_value_get_int (value);
      break;
    case PROP_MIDI_CHANNEL:
      piano->midi_chan = g_value_get_int (value);
      break;
    case PROP_BG_COLOR:
      piano->bg_color = g_value_get_uint (value);
      if (piano->bg)
	g_object_set (piano->bg, "fill-color", piano->bg_color, NULL);
      break;
    case PROP_WHITE_KEY_COLOR:
      piano->white_key_color = g_value_get_uint (value);
      swamigui_piano_update_key_colors (piano);
      break;
    case PROP_BLACK_KEY_COLOR:
      piano->black_key_color = g_value_get_uint (value);
      swamigui_piano_update_key_colors (piano);
      break;
    case PROP_SHADOW_EDGE_COLOR:
      piano->shadow_edge_color = g_value_get_uint (value);
      swamigui_piano_update_key_colors (piano);
      break;
    case PROP_WHITE_KEY_PLAY_COLOR:
      piano->white_key_play_color = g_value_get_uint (value);
      break;
    case PROP_BLACK_KEY_PLAY_COLOR:
      piano->black_key_play_color = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_piano_get_property (GObject *object, guint property_id,
			    GValue *value, GParamSpec *pspec)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  switch (property_id)
    {
    case PROP_WIDTH_PIXELS:
      g_value_set_int (value, piano->width);
      break;
    case PROP_HEIGHT_PIXELS:
      g_value_set_int (value, piano->height);
      break;
    case PROP_KEY_COUNT:
      g_value_set_int (value, piano->key_count);
      break;
    case PROP_START_OCTAVE:
      g_value_set_int (value, piano->start_note / 12);
      break;
    case PROP_LOWER_OCTAVE:
      g_value_set_int (value, piano->lower_octave);
      break;
    case PROP_UPPER_OCTAVE:
      g_value_set_int (value, piano->upper_octave);
      break;
    case PROP_LOWER_VELOCITY:
      g_value_set_int (value, piano->lower_velocity);
      break;
    case PROP_UPPER_VELOCITY:
      g_value_set_int (value, piano->upper_velocity);
      break;
    case PROP_MIDI_CONTROL:
      g_value_set_object (value, piano->midi_ctrl);
      break;
    case PROP_EXPRESS_CONTROL:
      g_value_set_object (value, piano->express_ctrl);
      break;
    case PROP_MIDI_CHANNEL:
      g_value_set_int (value, piano->midi_chan);
      break;
    case PROP_BG_COLOR:
      g_value_set_uint (value, piano->bg_color);
      break;
    case PROP_WHITE_KEY_COLOR:
      g_value_set_uint (value, piano->white_key_color);
      break;
    case PROP_BLACK_KEY_COLOR:
      g_value_set_uint (value, piano->black_key_color);
      break;
    case PROP_SHADOW_EDGE_COLOR:
      g_value_set_uint (value, piano->shadow_edge_color);
      break;
    case PROP_WHITE_KEY_PLAY_COLOR:
      g_value_set_uint (value, piano->white_key_play_color);
      break;
    case PROP_BLACK_KEY_PLAY_COLOR:
      g_value_set_uint (value, piano->black_key_play_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_piano_init (SwamiguiPiano *piano)
{
  int i;

  /* create MIDI control */
  piano->midi_ctrl = SWAMI_CONTROL (swami_control_midi_new ());
  swami_control_set_queue (piano->midi_ctrl, swamigui_root->ctrl_queue);
  swami_control_midi_set_callback (SWAMI_CONTROL_MIDI (piano->midi_ctrl),
				   swamigui_piano_midi_ctrl_callback, piano);

  /* create expression control */
  piano->express_ctrl = SWAMI_CONTROL (swami_control_value_new ());
  swami_control_set_queue (piano->express_ctrl, swamigui_root->ctrl_queue);
  swami_control_set_spec (piano->express_ctrl,
			  g_param_spec_int ("expression", "Expression",
					    "Expression",
					    0, 127, 0,
					    G_PARAM_READWRITE));
  swami_control_value_alloc_value (SWAMI_CONTROL_VALUE (piano->express_ctrl));

  piano->velocity = 127;	/* default velocity */
  piano->key_count = 128; /* FIXME: Shouldn't it get set on construction? */

  piano->width = SWAMIGUI_PIANO_DEFAULT_WIDTH;
  piano->height = SWAMIGUI_PIANO_DEFAULT_HEIGHT;
  piano->key_info = NULL;
  piano->start_note = 0;
  piano->lower_octave = SWAMIGUI_PIANO_DEFAULT_LOWER_OCTAVE;
  piano->upper_octave = SWAMIGUI_PIANO_DEFAULT_UPPER_OCTAVE;
  piano->lower_velocity = 127;
  piano->upper_velocity = 127;
  piano->mouse_note = 128;	/* disabled mouse key (> 127) */

  piano->bg_color = DEFAULT_BG_COLOR;
  piano->white_key_color = DEFAULT_WHITE_KEY_COLOR;
  piano->black_key_color = DEFAULT_BLACK_KEY_COLOR;
  piano->shadow_edge_color = DEFAULT_SHADOW_EDGE_COLOR;
  piano->white_key_play_color = DEFAULT_WHITE_KEY_PLAY_COLOR;
  piano->black_key_play_color = DEFAULT_BLACK_KEY_PLAY_COLOR;

  /* get the white key count for the number of keys in the piano */
  piano->white_count = piano->key_count / 12 * 7; /* 7 whites per oct */
  i = piano->key_count % 12; /* calculate for octave remainder (if any) */
  if (i < 6) piano->white_count += (i + 1) / 2;
  else piano->white_count += (i | 0x1) / 2 + 1;

  g_signal_connect (G_OBJECT (piano),
		    "event", G_CALLBACK (swamigui_piano_cb_event), piano);
}

static void
swamigui_piano_finalize (GObject *object)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (object);

  g_object_unref (piano->midi_ctrl); /* -- unref MIDI control */
  g_object_unref (piano->express_ctrl);	/* -- unref expression control */

  g_free (piano->key_info);
  piano->key_info = NULL;

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize)(object);
}

/* SwamiControlFunc set value func for MIDI control */
static void
swamigui_piano_midi_ctrl_callback (SwamiControl *control,
				  SwamiControlEvent *event,
				  const GValue *value)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (SWAMI_CONTROL_FUNC(control)->user_data);
  GArray *valarray = NULL;
  SwamiMidiEvent *midi;
  int i, count = 1;		/* default for single values */

  /* if its multiple values, fetch the value array */
  if (G_VALUE_TYPE (value) == G_TYPE_ARRAY)
    {
      valarray = g_value_get_boxed (value);
      count = valarray->len;
    }

  i = 0;
  while (i < count)
    {
      if (valarray) value = &g_array_index (valarray, GValue, i);

      if (G_VALUE_TYPE (value) == SWAMI_TYPE_MIDI_EVENT
	  && (midi = g_value_get_boxed (value)))
	{
	  if (midi->channel == piano->midi_chan)
	    {
	      switch (midi->type)
		{
		case SWAMI_MIDI_NOTE_ON:
		  swamigui_piano_note_on_internal (piano, midi->data.note.note,
						   midi->data.note.velocity);
		  break;
		case SWAMI_MIDI_NOTE_OFF:
		  swamigui_piano_note_off_internal (piano, midi->data.note.note,
						    midi->data.note.velocity);
		  break;
		default:
		  break;
		}
	    }
	}
      i++;
    }
}

static void
swamigui_piano_item_realize (GnomeCanvasItem *item)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (item);

  if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
    (*GNOME_CANVAS_ITEM_CLASS (parent_class)->realize) (item);

  swamigui_piano_draw (piano);
}

static void
swamigui_piano_draw (SwamiguiPiano *piano)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (piano);
  KeyInfo *keyp;
  gboolean isblack;
  double x, x1, x2;
  int i, mod;
  int vlineh1, vlineh2;

  /* return if not added to a canvas yet */
  if (!GNOME_CANVAS_ITEM (piano)->canvas) return;

  if (!piano->up2date)
    {
      /* convert pixel width and height to world values */
      gnome_canvas_c2w (GNOME_CANVAS_ITEM (piano)->canvas,
			piano->width, piano->height,
			&piano->world_width, &piano->world_height);

      piano->key_width = piano->world_width / piano->key_count;
      piano->key_width_half = piano->key_width / 2;

      piano->vline_width = (int)(piano->key_width * PIANO_VLINE_TO_KEY_SCALE + 0.5);
      piano->hline_width = (int)(piano->height * PIANO_HLINE_TO_HEIGHT_SCALE + 0.5);

      /* black key dimensions */
      piano->black_height = piano->world_height * PIANO_BLACK_TO_HEIGHT_SCALE;

      /* top of grey edge */
      piano->shadow_top = piano->world_height - piano->hline_width
	- piano->world_height * PIANO_GREY_TO_HEIGHT_SCALE;

      /* black key velocity range and offset cache values */
      piano->black_vel_ofs = piano->black_height
	* PIANO_BLACK_INDICATOR_OFS_SCALE;
      piano->black_vel_range = piano->black_height
	* PIANO_BLACK_INDICATOR_RANGE_SCALE;

      /* white key velocity range and offset cache values */
      piano->white_vel_ofs = piano->world_height
	* PIANO_WHITE_INDICATOR_OFS_SCALE;
      piano->white_vel_range = piano->world_height
	* PIANO_WHITE_INDICATOR_RANGE_SCALE;

      piano->up2date = TRUE;
    }

  if (!piano->bg)		/* one time canvas item creation */
    piano->bg =	/* black piano background (border & separators) */
      gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_RECT,
			     "x1", (double)0.0,
			     "y1", (double)0.0,
			     "fill-color-rgba", piano->bg_color,
			     "outline-color", NULL,
			     NULL);

  /* create black and white keys if not already done */
  if (!piano->key_info)
    {
      piano->key_info = g_new0 (KeyInfo, piano->key_count);

      for (i = 0, mod = 0, isblack = FALSE; i < piano->key_count; i++, mod++)
	{
	  if (mod == 12) mod = 0;	/* octave modulo */

	  keyp = &((KeyInfo *)(piano->key_info))[i];

	  /* create grey edge for white keys */
	  if (!isblack)
	    { /* bottom grey edge of white keys */
      	      keyp->shadow =
		gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_RECT,
				       "fill-color-rgba", piano->shadow_edge_color,
				       "outline-color", NULL,
				       NULL);
	    }

	  keyp->item = gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_RECT,
					      "fill-color-rgba",
					      isblack ? piano->black_key_color
					      : piano->white_key_color,
					      NULL);

	  /* lower white keys one step (so previous black key is above it) */
	  if (i > 0 && !isblack)
	    {
	      gnome_canvas_item_lower (keyp->item, 2);
	      gnome_canvas_item_lower (keyp->shadow, 2);
	    }

	  if (mod != 4 && mod != 11) isblack ^= 1;
	}
    }

  gnome_canvas_item_set (piano->bg, /* set background size */
			 "x2", piano->world_width,
			 "y2", piano->world_height,
			 NULL);

  /* calculate halves of vline_width */
  vlineh1 = (int)(piano->vline_width + 0.5);
  vlineh2 = vlineh1 / 2;
  vlineh1 -= vlineh2;

  for (i = 0, mod = 0, isblack = FALSE; i < piano->key_count; i++, mod++)
    {
      if (mod == 12) mod = 0;	/* octave modulo */

      keyp = &((KeyInfo *)(piano->key_info))[i];
      x = i * piano->world_width / piano->key_count;

      if (isblack)	/* key is black? */
	{
	  gnome_canvas_item_set (keyp->item,
				 "x1", x,
				 "x2", x + piano->key_width,
				 "y1", piano->hline_width,
				 "y2", piano->black_height,
				 NULL);
	}
      else	/* white key */
	{
	  if (mod == 0 || mod == 5) x1 = x;
	  else x1 = x - piano->key_width_half;

	  if (mod == 4 || mod == 11) x2 = x + piano->key_width;
	  else if (i != piano->key_count - 1)
	    x2 = x + piano->key_width + piano->key_width_half;
	  else x2 = x + piano->key_width - 1.0;

	  x1 += vlineh1;
	  x2 -= vlineh2;

	  gnome_canvas_item_set (keyp->item,
				 "x1", x1,
				 "x2", x2,
				 "y1", piano->hline_width,
				 "y2", piano->shadow_top,
				 NULL);
	  gnome_canvas_item_set (keyp->shadow,
				 "x1", x1,
				 "x2", x2,
				 "y1", piano->shadow_top,
				 "y2", piano->world_height - piano->hline_width,
				 NULL);
	}

      if (mod != 4 && mod != 11) isblack ^= 1;
    }
}

/* called to destroy piano keys (used when key count changes for example) */
static void
swamigui_piano_destroy_keys (SwamiguiPiano *piano)
{
  KeyInfo *keyp;
  int i;

  if (!piano->key_info) return;

  for (i = 0; i < piano->key_count; i++)
  {
    keyp = &((KeyInfo *)(piano->key_info))[i];

    if (keyp->item) gtk_object_destroy (GTK_OBJECT (keyp->item));
    if (keyp->indicator) gtk_object_destroy (GTK_OBJECT (keyp->indicator));
    if (keyp->shadow) gtk_object_destroy (GTK_OBJECT (keyp->shadow));
  }

  g_free (piano->key_info);
  piano->key_info = NULL;
}

/* Update the color of keys.
 * keytype: 0 = white, 1 = black, 2 = grey edge */
static void
swamigui_piano_update_key_colors (SwamiguiPiano *piano)
{
  KeyInfo *keyp;
  gboolean isblack;
  int i, mod;

  for (i = 0, mod = 0, isblack = FALSE; i < piano->key_count; i++, mod++)
  {
    if (mod == 12) mod = 0;	/* octave modulo */

    keyp = &((KeyInfo *)(piano->key_info))[i];

    if (!isblack)
    {
      if (keyp->item)
	g_object_set (keyp->item, "fill-color-rgba", piano->white_key_color,
		      NULL);

      if (keyp->shadow)
	g_object_set (keyp->shadow, "fill-color-rgba", piano->shadow_edge_color,
		      NULL);
    }
    else if (keyp->item)
      g_object_set (keyp->item, "fill-color-rgba", piano->black_key_color, NULL);

    if (mod != 4 && mod != 11) isblack ^= 1;
  }
}

static void
swamigui_piano_draw_noteon (SwamiguiPiano *piano, int note, int velocity)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (piano);
  KeyInfo *key_info;
  double pos, w, vel;
  int note_ofs;
  gboolean black;

  /* get key info structure for note */
  note_ofs = note - piano->start_note;
  key_info = &((KeyInfo *)(piano->key_info))[note_ofs];

  pos = swamigui_piano_note_to_pos (piano, note, 0, TRUE, &black);

  if (!black)		/* white key? */
    { /* lengthen white key to make it look pressed down */
      gnome_canvas_item_set (GNOME_CANVAS_ITEM (key_info->item),
			     "y2", piano->world_height - piano->hline_width,
			     NULL);

      /* calculate size and velocity position of white key indicator */
      w = piano->key_width_half * PIANO_WHITE_INDICATOR_WIDTH_SCALE;
      vel = velocity * piano->white_vel_range / 127.0 + piano->white_vel_ofs;
    }
  else				/* black key */
    {		  /* shorten black key to make it look pressed down */
      gnome_canvas_item_set (GNOME_CANVAS_ITEM (key_info->item),
			     "y2", piano->black_height
			     - piano->black_height
			     * PIANO_BLACK_SHORTEN_SCALE,
			     NULL);

      /* calculate size and velocity position of black key indicator */
      w = piano->key_width_half * PIANO_BLACK_INDICATOR_WIDTH_SCALE;
      vel = velocity * piano->black_vel_range / 127.0 + piano->black_vel_ofs;
    }

  /* create the indicator */
  key_info->indicator =
    gnome_canvas_item_new (group, GNOME_TYPE_CANVAS_RECT,
			   "x1", pos - w,
			   "y1", piano->hline_width,
			   "x2", pos + w,
			   "y2", vel,
			   "fill-color-rgba", black ? piano->black_key_play_color
			   : piano->white_key_play_color,
			   "outline-color", NULL,
			   NULL);
}

static void
swamigui_piano_draw_noteoff (SwamiguiPiano *piano, int note)
{
  KeyInfo *key_info;
  int note_ofs, mod;
  gboolean black;

  /* get key info structure for note */
  note_ofs = note - piano->start_note;
  key_info = &((KeyInfo *)(piano->key_info))[note_ofs];

  if (!key_info->indicator) return;

  /* destroy indicator */
  gtk_object_destroy (GTK_OBJECT (key_info->indicator));
  key_info->indicator = NULL;

  /* determine if key is black or white */
  mod = note % 12;
  if (mod <= 4) black = mod & 1;
  else black = !(mod & 1);

  if (black)
    {			   /* reset black key back to normal length */
      gnome_canvas_item_set (GNOME_CANVAS_ITEM (key_info->item),
			     "y2", piano->black_height,
			     NULL);
    }
  else			/* reset white key back to normal length */
    {
      gnome_canvas_item_set (GNOME_CANVAS_ITEM (key_info->item),
			     "y2", piano->shadow_top,
			     NULL);
    }
}

static void
swamigui_piano_update_mouse_note (SwamiguiPiano *piano, double x, double y)
{
  static guint8 last_note = -1;		/* cache note number for statusbar */
  KeyInfo *key_info;
  double indicator_y;
  int note, velocity = 127;
  char midiNote[5];
  char *statusmsg;
  int *velp = NULL;
  gboolean isblack;

  if (x < 0.0) x = 0.0;
  else if (x > piano->world_width) x = piano->world_width;
  else velp = &velocity;

  if (y < 0.0 || y > piano->world_height)
  {
    y = 0.0;
    velp = NULL;
  }

  note = swamigui_piano_pos_to_note (piano, x, y, velp, &isblack);
  note = CLAMP (note, 0, 127);	/* force clamp note */

  key_info = &((KeyInfo *)(piano->key_info))[note];

  if (isblack)
    indicator_y = piano->black_vel_ofs + (velocity * piano->black_vel_range
  					  / 127.0);
  else indicator_y = piano->white_vel_ofs + (velocity * piano->white_vel_range
  					     / 127.0);

  if (last_note != note)	/* update statusbar note value */
    {
      swami_util_midi_note_to_str (note, midiNote);
      statusmsg = g_strdup_printf (_("Note: %-3s (%d) Velocity: %d"), midiNote,
      				   note, velocity);
      swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global",
					statusmsg);
      g_free (statusmsg);
    }

  if (piano->mouse_note > 127) return;	/* if mouse note play not active, we done */

  if (note != piano->mouse_note) /* note changed? */
    {
      swamigui_piano_note_off (piano, piano->mouse_note, 127); /* note off */
      piano->mouse_note = note;
      swamigui_piano_note_on (piano, note, velocity); /* new note on */
    }
  else				/* same note, update indicator for velocity */
    {
      gnome_canvas_item_set (key_info->indicator,
      			     "y2", indicator_y, NULL);
    }
}

static gboolean
swamigui_piano_cb_event (GnomeCanvasItem *item, GdkEvent *event,
			gpointer data)
{
  SwamiguiPiano *piano = SWAMIGUI_PIANO (data);
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int note, velocity;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *)event;
      if (bevent->button != 1) break;
      note = swamigui_piano_pos_to_note (piano, bevent->x, bevent->y,
					 &velocity, NULL);
      if (note < 0) break;	/* click not on a note? */
      piano->mouse_note = note;
      swamigui_piano_note_on (piano, note, velocity);

      gnome_canvas_item_grab (item, GDK_POINTER_MOTION_MASK
			      | GDK_BUTTON_RELEASE_MASK, NULL,
			      bevent->time);
      return (TRUE);
    case GDK_BUTTON_RELEASE:
      if (piano->mouse_note > 127) break; /* no mouse selected note? */
      note = piano->mouse_note;
      piano->mouse_note = 128;	/* no more selected mouse note */
      swamigui_piano_note_off (piano, note, 127);

      gnome_canvas_item_ungrab (item, event->button.time);
      break;
    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *)event;
      swamigui_piano_update_mouse_note (piano, mevent->x, mevent->y);
      break;
    case GDK_LEAVE_NOTIFY:
      swamigui_statusbar_msg_set_label (swamigui_root->statusbar, 0, "Global", NULL);
      break;
    default:
      break;
    }

  return (FALSE);
}

/**
 * swamigui_piano_note_on:
 * @piano: Piano object
 * @note: MIDI note number to turn on
 * @velocity: MIDI velocity number (-1 to use current keyboard velocity)
 *
 * Piano note on. Turns on note visually on piano as well as sends an event to
 * #SwamiControl objects connected to piano MIDI event control.
 */
void
swamigui_piano_note_on (SwamiguiPiano *piano, int note, int velocity)
{
  if (velocity == -1) velocity = piano->velocity;
  if (velocity == 0) swamigui_piano_note_off (piano, note, velocity);

  if (swamigui_piano_note_on_internal (piano, note, velocity))
    {	/* send a note on event */
      swami_control_midi_transmit (SWAMI_CONTROL_MIDI (piano->midi_ctrl),
				   SWAMI_MIDI_NOTE_ON, piano->midi_chan,
				   note, velocity);
    }
}

/* internal function for visually displaying a note on */
static gboolean
swamigui_piano_note_on_internal (SwamiguiPiano *piano, int note, int velocity)
{
  KeyInfo *key_info;
  int start_note;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), FALSE);
  g_return_val_if_fail (note >= 0 && note <= 127, FALSE);
  g_return_val_if_fail (velocity >= -1 && velocity <= 127, FALSE);

  start_note = piano->start_note;
  if (note < start_note || note >= start_note + piano->key_count)
    return (FALSE);

  key_info = &((KeyInfo *)(piano->key_info))[note - start_note];
  if (key_info->indicator) return (FALSE); /* note already on? */

  swamigui_piano_draw_noteon (piano, note, velocity);
  return (TRUE);
}

/**
 * swamigui_piano_note_off:
 * @piano: Piano object
 * @note: MIDI note number to turn off
 * @velocity: MIDI note off velocity
 *
 * Piano note off. Turns off note visually on piano as well as sends an event
 * to it's connected MIDI driver (if any).
 */
void
swamigui_piano_note_off (SwamiguiPiano *piano, int note, int velocity)
{
  if (swamigui_piano_note_off_internal (piano, note, velocity))
    {	/* send a note off event */
      swami_control_midi_transmit (SWAMI_CONTROL_MIDI (piano->midi_ctrl),
				   SWAMI_MIDI_NOTE_OFF, piano->midi_chan,
				   note, velocity);
    }
}

/* updates drawing of piano */
static gboolean
swamigui_piano_note_off_internal (SwamiguiPiano *piano, int note, int velocity)
{
  KeyInfo *key_info;
  int start_note;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), FALSE);
  g_return_val_if_fail (note >= 0 && note <= 127, FALSE);
  g_return_val_if_fail (velocity >= -1 && velocity <= 127, FALSE);

  start_note = piano->start_note;
  if (note < start_note || note >= start_note + piano->key_count)
    return (FALSE);

  key_info = &((KeyInfo *)(piano->key_info))[note - start_note];
  if (!key_info->indicator) return (FALSE); /* note already off? */

  swamigui_piano_draw_noteoff (piano, note);
  return (TRUE);
}

/**
 * swamigui_piano_pos_to_note:
 * @piano: Piano object
 * @x: X coordinate in canvas world units
 * @y: Y coordinate in canvas world units
 * @velocity: Location to store velocity or %NULL
 * @isblack: Location to store a boolean if key is black/white, or %NULL
 *
 * Get the MIDI note corresponding to a point on the piano. The velocity
 * relates to the vertical axis of the note. Positions towards the tip
 * of the key generate higher velocities.
 *
 * Returns: The MIDI note number or -1 if not a valid key.
 */
int
swamigui_piano_pos_to_note (SwamiguiPiano *piano, double x, double y,
			    int *velocity, gboolean *isblack)
{
  gboolean black;
  gdouble keyofs;
  int note, mod;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), -1);

  /* within piano bounds? */
  if (x < 0.0 || x > piano->world_width || y < 0.0 || y > piano->world_height)
    return (-1);

  /* calculate note */
  note = x / piano->key_width;

  if (note >= piano->key_count) note = piano->key_count - 1;
  mod = note % 12;
  black = (mod == 1 || mod == 3 || mod == 6 || mod == 8 || mod == 10);

  if (black && y > piano->black_height)	/* select white key, if below black keys */
    {
      keyofs = x - (note * piano->key_width);

      if (keyofs >= piano->key_width_half)
	{
	  note++;
	  if (note >= piano->key_count) note = piano->key_count - 1;
	}
      else note--;

      black = FALSE;
    }

  if (velocity)
    {
      if (black)
	{
	  if (y < piano->black_vel_ofs) *velocity = 1;
	  else if (y > piano->black_vel_ofs + piano->black_vel_range)
	    *velocity = 127;
	  else *velocity = 1.0 + (y - piano->black_vel_ofs)
		 / (piano->black_vel_range) * 126.0 + 0.5;
	}
      else			/* white key */
	{
	  if (y < piano->white_vel_ofs) *velocity = 1;
	  else if (y > piano->white_vel_ofs + piano->white_vel_range)
	    *velocity = 127;
	  else *velocity = 1.0 + (y - piano->white_vel_ofs)
		 / (piano->white_vel_range) * 126.0 + 0.5;
	}
    }

  if (isblack) *isblack = black;
  return (piano->start_note + note);
}

/**
 * swamigui_piano_note_to_pos (SwamiguiPiano *piano, int note, gboolean isblack)
 * @piano: Piano object
 * @note: MIDI note number to convert to canvas position
 * @edge: Which edge to get position of (-1 = left, 0 = center, 1 = right)
 * @realnote: If %TRUE then the coordinate is adjusted for the actual drawn key
 *   rather than the active area (equal for all keys), when %FALSE is specified.
 * @isblack: Pointer to store a gboolean which is set if key is black, or %NULL
 *
 * Find the canvas X coordinate for a given MIDI note.
 *
 * Returns: X canvas coordinate of @note for the specified @edge.
 */
double
swamigui_piano_note_to_pos (SwamiguiPiano *piano, int note, int edge,
			    gboolean realnote, gboolean *isblack)
{
  int noteofs;
  double pos;
  gboolean black;
  int mod;

  g_return_val_if_fail (SWAMIGUI_IS_PIANO (piano), 0.0);
  g_return_val_if_fail (note >= piano->start_note, 0.0);

  noteofs = note - piano->start_note;
  g_return_val_if_fail (noteofs < piano->key_count, 0.0);

  /* calculate position to note */
  pos = noteofs * piano->world_width / piano->key_count;

  mod = note % 12;
  black = (mod == 1 || mod == 3 || mod == 6 || mod == 8 || mod == 10);

  if (!realnote || black)	/* active area request or black key? */
    { /* active area is equal for all keys and black keys are equal to active area */
      if (edge == 0) pos += piano->key_width_half;
      else if (edge == 1)
	{
	  pos += piano->key_width;
	  if (noteofs == piano->key_count - 1) pos -= 1.0;
	}
    }
  else		/* white key */
    {
      if (edge == -1)
	{
	  if (mod != 0 && mod != 5)
	    pos -= piano->key_width_half;
	}
      else if (edge == 0)
	{
	  if (mod == 0 || mod == 5)
	    pos += (piano->key_width + piano->key_width_half) / 2.0;
	  else if (mod == 4 || mod == 11 || noteofs == piano->key_count - 1)
	    pos += piano->key_width_half / 2.0;
	  else pos += piano->key_width_half;
	}
      else
	{
	  if (mod == 0 || mod == 5)
	    pos += piano->key_width + piano->key_width_half;
	  else if (mod == 4 || mod == 11)
	    pos += piano->key_width;
	  else if (noteofs != piano->key_count - 1)
	    pos += piano->key_width + piano->key_width_half;
	  else pos += piano->key_width - 1.0;
	}
    }

  if (isblack) *isblack = black;
  return (pos);
}
