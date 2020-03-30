/*
 * util.c - Miscellaneous utility functions
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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "util.h"
#include "swami_priv.h"

static void recurse_types(GType type, GArray *array);

#define capture_object 1 // for debugging
#if capture_object
/*----------------------------------------------------------------------------
 GObject type instance capture system.
-----------------------------------------------------------------------------*/
/*
 Description:
 This ultra simple interactive system is particularly useful during
 debugging helping to localize any missing  g_object_unref() call inside the
 whole application using the GObject library.

 The system allows to focus on a given type of object to track inside a given
 code section of the application. For example if one want to know if all object
 of type "SwamiControl" are instancied/finalized, we just need to start  a
 capture of this type by calling start_count_obj("SwamiControl") at the
 beginning of the application (after type registration). Then we must call
 stop_count_obj() at the end of the application. The system displays messages
 to the stdout console.

 After a capture is started:
 - The system capture all instances creation of "SwamiControl" object and
   displays a message on the console:
       "Start capture of object "SamiControl":
	   ....
       "create object SwamiControl=32075784, count=1"
       "create object SwamiControl=32075999, count=2"
	   ...
 - When any instance is finalised the sytem display a message:
       "finalize object SwamiControl=32075784, count=1"

 Note:
 - When an instance is created before calling start_capture() and it is
   finalized during the capture, the message is:
       "finalize object SwamiControl=30511552 not in capture list"

 When the capture is stopped, the system display all remaining instance not
 yet finalized during the capture:

   Capture stopped
     remaining object SwamiControl=51495624
     remaining object SwamiControl=51495448
     remaining object SwamiControl=30908200
     Remaining object count: 3
     Press "entry" to continue

 In this example the counter "says" that 3 instances are not finalized during
 the capture.
-----------------------------------------------------------------------------*/

static GObject* hook_constructor(GType                 type,
                                 guint                 n_construct_properties,
                                 GObjectConstructParam *construct_properties);
static void hook_finalize(GObject *object);

#define CAPTURE_STARTED  0x01 /* capture has been started */
#define OBJECT_CAPTURED  0x02 /* At leat one object has been captured */

static struct _capture_obj
{
    /* constructor */
    GObject* (*ori_constructor)  (GType                 type,
                                  guint                 n_construct_properties,
                                  GObjectConstructParam *construct_properties);
    /* finalize */
    void (*ori_finalize) (GObject *object);
    GObjectClass  *obj_class;        /* class of object */
    guint8 flags;
    GType type;       /* type of object under capture */
    guint count;      /* actual number of object captured */
    GList *list_obj;  /* the list of object captured */
}capture_obj;

/* hook callback, called on instance construction */
static GObject* hook_constructor  (GType                 type,
                                   guint                 n_construct_properties,
                                   GObjectConstructParam *construct_properties)
{
    /* calling original instance constructor */
    GObject *object = capture_obj.ori_constructor(type, n_construct_properties,
                                                  construct_properties);

    if(g_list_find (capture_obj.list_obj, object))
    {
        printf("Error, this shouldn't happens\n");
    }
	/* memorise the instance of interest in list */
    if(type == capture_obj.type)
    {
        capture_obj.list_obj = g_list_prepend (capture_obj.list_obj, object);
        capture_obj.count++; /* update count of instance in list */
        /* just to know that at least one object has been captured */
        capture_obj.flags |= OBJECT_CAPTURED;
        printf("Create object %s=%d, count=%d\n", G_OBJECT_TYPE_NAME(object),
                                                  object, capture_obj.count);
    }
    return object; /* return new instance */
}

/* hook callback, called on instance finalization */
static void hook_finalize(GObject *object)
{
    GType type = G_TYPE_FROM_INSTANCE(object);
    if(type == capture_obj.type)
    {
        /* find and remove object from list*/
        GList *element = g_list_find (capture_obj.list_obj, object);
        if(element)
        {
            capture_obj.list_obj = g_list_delete_link(capture_obj.list_obj, element);
            capture_obj.count--;
            printf("Finalize object %s=%d, count=%d\n", g_type_name(type), object, capture_obj.count);
        }
        else
        {
            printf("Finalize object %s=%d not in capture list\n", g_type_name(type), object);
        }
    }

    /* calling original finalization method */
    if( capture_obj.ori_finalize)
    {
        capture_obj.ori_finalize(object);
    }
}

/*
  start_count_obj(char * name_type)
  The function start a capture of all creation/destruction instance of the named
  type of object.

  The type must already registered and derived from G_TYPE_OBJECT, and it must be
  instantiable, (abstact types are invalid).
  If a capture is already started, the function fail.

  @type_name: The name of the type object to be captured
  @return TRUE if a capture is started, FALSE otherwise
*/
gboolean start_count_obj(char *type_name)
{
    GType type;
    if(capture_obj.flags != 0)
    {
        printf("Cannot start capture because a capture is already started, stop capture first !\n");
        return FALSE;
    }
    /* check if type exist and is valid */
    type = g_type_from_name (type_name);
	if(! g_type_is_a(type, G_TYPE_OBJECT))
    {
        printf("Cannot start capture of invalid object \"%s\"!\n", type_name);
        return FALSE;
    }
    {
        GObjectClass  *obj_class;
        obj_class = (GObjectClass *)g_type_class_peek(type);

        if(obj_class == NULL)
        {
            obj_class = (GObjectClass *)g_type_class_ref(type);
        }
        capture_obj.obj_class = obj_class;
        capture_obj.type = type; /* type object of interest */
        capture_obj.count = 0;   /* reset count */
        capture_obj.flags = CAPTURE_STARTED;   /*  flags */
        capture_obj.list_obj = NULL; /* init list */

        /* install hooks in relevent parent class */
        capture_obj.ori_constructor = obj_class->constructor;
        obj_class->constructor = hook_constructor;

        capture_obj.ori_finalize = obj_class->finalize;
        obj_class->finalize = hook_finalize;

        printf("Start capture of object \"%s\":\n", g_type_name(capture_obj.type));
        return TRUE;
    }
}

/*
 Stop a capture started with start_count_obj(), otherwise the function fail.
 @return TRUE if a capture is stopped, FALSE otherwise
*/
gboolean stop_count_obj()
{
    GList *obj;

    if(!(capture_obj.flags & CAPTURE_STARTED))
    {
        printf("Cannot stop a capture, start a capture first!\n");
        return FALSE;
    }

    printf("Capture of object \"%s\" stopped\n", g_type_name(capture_obj.type));
    obj = g_list_reverse(capture_obj.list_obj);
    if(obj)
    {
        while(obj)
        {
            printf("Remaining object %s=%d\n", G_OBJECT_TYPE_NAME(obj->data), obj->data);
            obj = obj->next;
        }
        g_list_free(capture_obj.list_obj);
        printf("Remaining object count=%d\n", capture_obj.count);
    }
    else
    {
        if(capture_obj.flags & OBJECT_CAPTURED)
        {
           printf("All captured object %s were instanciated/finalized\n", g_type_name(capture_obj.type));
        }
        else
        {
           printf("No object %s were captured\n", g_type_name(capture_obj.type));
        }
    }

    capture_obj.flags = 0;   /*  clear flags */

    /* restore original method in relevent class */
    capture_obj.obj_class->constructor = capture_obj.ori_constructor;
    capture_obj.obj_class->finalize = capture_obj.ori_finalize;
    printf("Press \"entry\" to continue");
	getchar();
    return TRUE;
}
#endif

/*----------------------------------------------------------------------------
 Ancestry gObject type related functions
-----------------------------------------------------------------------------*/
/**
 * swami_util_get_child_types:
 * @type: Type to get all children from
 * @n_types: Location to store count of types or %NULL to ignore
 *
 * Recursively get all child types of a @type.
 *
 * Returns: Newly allocated and 0 terminated array of types
 */
GType *
swami_util_get_child_types(GType type, guint *n_types)
{
    GArray *array;
    GType *types;

    array = g_array_new(TRUE, FALSE, sizeof(GType));
    recurse_types(type, array);

    if(n_types)
    {
        *n_types = array->len;
    }

    types = (GType *)(array->data);
    g_array_free(array, FALSE);

    return (types);
}

static void
recurse_types(GType type, GArray *array)
{
    GType *child_types, *ptype;

    child_types = g_type_children(type, NULL);

    for(ptype = child_types; *ptype; ptype++)
    {
        g_array_append_val(array, *ptype);
        recurse_types(*ptype, array);
    }

    g_free(child_types);
}

/*----------------------------------------------------------------------------
 GValue allocation using slice memory pool
-----------------------------------------------------------------------------*/
/**
 * swami_util_new_value:
 *
 * Allocate a new GValue using a GMemChunk.
 *
 * Returns: New uninitialized (zero) GValue
 */
GValue *
swami_util_new_value(void)
{
    return (g_slice_new0(GValue));
}

/**
 * swami_util_free_value:
 * @value: GValue created from swami_util_new_value().
 *
 * Free a GValue that was allocated with swami_util_new_value().
 */
void
swami_util_free_value(GValue *value)
{
    g_value_unset(value);
    g_slice_free(GValue, value);
}

/*----------------------------------------------------------------------------
 MIDI octave to music octave convertion.
 MIDI note to music note convertion.
-----------------------------------------------------------------------------*/
/*
   MIDI_TO_MUSIC_OFFSET is the offset between music octave number and MIDI
   Actually this offset leads to diapason note A4 (music octave 4) beeing MIDI
   note 69 (MIDI octave 5).
*/
#define MIDI_TO_MUSIC_OFFSET (-1)

/* MIDI_TO_MUSIC_OCT returns a "music octave number" from a "MIDI octave numer" */
#define MIDI_TO_MUSIC_OCT(midi_oct) ((midi_oct) + MIDI_TO_MUSIC_OFFSET )

/* MUSIC_TO_MIDI_OCT returns a "MIDI octave number" from a "music octave numer" */
#define MUSIC_TO_MIDI_OCT(music_oct) ((music_oct) -  MIDI_TO_MUSIC_OFFSET)

/**
 * swami_util_midi_note_to_str:
 * @note: MIDI note number (0-127)
 * @str: Buffer to store string to (at least 5 bytes in length)
 */
void
swami_util_midi_note_to_str(int note, char *str)
{
    static const char *notes[]
        = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    char octavestr[3];

    g_return_if_fail(note >= 0 && note <= 127);

    strcpy(str, notes[note % 12]);
    /* C0 is actually note 12 */
    sprintf (octavestr, "%d", MIDI_TO_MUSIC_OCT(note / 12));
    strcat(str, octavestr);
}

/**
 * swami_util_midi_str_to_note:
 * @str: String to parse as a MIDI note
 *
 * Parse a string in the form "0"-"127" as a MIDI note or
 * a note name in the form  "[A-G|a-g][b#]n".
 * Where 'n' is the octave number between -1 and 9. '#' is used to indicate
 * "sharp".  'b' means "flat".  Examples:
 *    "C4" is middle C (note 60),
 *    "F#-1" is note 5, "Db-1" is the same as "C#-1" (note 1).
 *
 * Any chars following a valid MIDI note string are ignored.
 *
 * Returns: The MIDI note # or -1 on error (str is malformed).
*/
int
swami_util_midi_str_to_note(const char *str)
{
    guint8 octofs[7] = { 9, 11, 0, 2, 4, 5, 7 };	/* octave offset for A-G */
    char *endptr;
    long int l;
    int note, oct_sign, music_oct;
    char c;

    g_return_val_if_fail(str != NULL, -1);

    if(!*str)
    {
        return (-1);
    }

    /* try converting as a decimal string */
    l = strtol(str, &endptr, 10);

    if(!*endptr && l >= 0 && l <= 127)
    {
        return (l);
    }

    /* get first char (should be a note character */
    c = *str++;

    if(c >= 'A' && c <= 'G')
    {
        note = octofs[c - 'A'];
    }
    else if(c >= 'a' && c <= 'g')
    {
        note = octofs[c - 'a'];
    }
    else
    {
        return (-1);
    }

    c = *str++;

    if(c == '#')
    {
        note++;
        c = *str++;
    }
    else if(c == 'b')
    {
        note--;
        c = *str++;
    }
    else if(c == 0)
    {
        return (-1);
    }

    /* Get octave number */
    if (c == '-')
    {
        c = *str++;
        oct_sign = -1;
    }
    else
    {
        oct_sign = +1;
    }
    if( c >= '0' && c <= '9') /* Only 1 digit? */
    {
        music_oct = (c - '0') * oct_sign;
        note += MUSIC_TO_MIDI_OCT(music_oct) * 12;
    }
    else
    {
        return -1;
    }

    if(note >= 0 && note <= 127)
    {
        return (note);
    }
    else
    {
        return (-1);
    }
}
