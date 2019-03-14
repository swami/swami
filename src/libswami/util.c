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

static void recurse_types (GType type, GArray *array);


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
swami_util_get_child_types (GType type, guint *n_types)
{
  GArray *array;
  GType *types;

  array = g_array_new (TRUE, FALSE, sizeof (GType));
  recurse_types (type, array);

  if (n_types) *n_types = array->len;

  types = (GType *)(array->data);
  g_array_free (array, FALSE);

  return (types);
}

static void
recurse_types (GType type, GArray *array)
{
  GType *child_types, *ptype;

  child_types = g_type_children (type, NULL);

  for (ptype = child_types; *ptype; ptype++)
    {
      g_array_append_val (array, *ptype);
      recurse_types (*ptype, array);
    }

  g_free (child_types);
}

/**
 * swami_util_new_value:
 *
 * Allocate a new GValue using a GMemChunk.
 *
 * Returns: New uninitialized (zero) GValue
 */
GValue *
swami_util_new_value (void)
{
  return (g_slice_new0 (GValue));
}

/**
 * swami_util_free_value:
 * @value: GValue created from swami_util_new_value().
 *
 * Free a GValue that was allocated with swami_util_new_value().
 */
void
swami_util_free_value (GValue *value)
{
  g_value_unset (value);
  g_slice_free (GValue, value);
}

/**
 * swami_util_midi_note_to_str:
 * @note: MIDI note number (0-127)
 * @str: Buffer to store string to (at least 5 bytes in length)
 */
void
swami_util_midi_note_to_str (int note, char *str)
{
  static const char *notes[]
    = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
  char octavestr[3];

  g_return_if_fail (note >= 0 && note <= 127);

  strcpy (str, notes[note % 12]);
  sprintf (octavestr, "%d", note / 12 - 2);	/* C0 is actually note 24 */
  strcat (str, octavestr);
}

/**
 * swami_util_midi_str_to_note:
 * @str: String to parse as a MIDI note
 *
 * Parse a string in the form "[A-Ga-g][b#]n" or "0"-"127" as a MIDI note.
 * Where 'n' is the octave number between -2 and 8. '#' is used to indicate
 * "sharp".  'b' means "flat".  Examples: "C3" is middle C (note 60),
 * "F#-2" is note 5, "Db-2" is the same as "C#-2" (note 0).  Any chars
 * following a valid MIDI note string are ignored.
 *
 * Returns: The MIDI note # or -1 on error (str is malformed).
 */
int
swami_util_midi_str_to_note (const char *str)
{
  guint8 octofs[7] = { 9, 11, 0, 2, 4, 5, 7 };	/* octave offset for A-G */
  char *endptr;
  long int l;
  int note;
  char c;

  g_return_val_if_fail (str != NULL, -1);
  if (!*str) return (-1);

  /* try converting as a decimal string */
  l = strtol (str, &endptr, 10);
  if (!*endptr && l >= 0 && l <= 127) return (l);

  /* get first char (should be a note character */
  c = *str++;
  if (c >= 'A' && c <= 'G') note = octofs[c - 'A'];
  else if (c >= 'a' && c <= 'g') note = octofs[c - 'a'];
  else return (-1);

  c = *str++;
  if (c == '#')
    {
      note++;
      c = *str++;
    }
  else if (c == 'b')
    {
      note--;
      c = *str++;
    }
  else if (c == 0) return (-1);

  if (c == '-')
    {
      c = *str++;
      if (c == '1') note += 12;
      else if (c != '2') return (-1);
    }
  else if ((c >= '0' && c <= '8'))	/* Only 1 digit? */
    note += (c - '0') * 12 + 24;
  else return (-1);

  if (note >= 0 && note <= 127) return (note);
  else return (-1);
}
