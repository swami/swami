/*
 * util.h - Swami utility stuff
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
#ifndef __SWAMI_UTIL_H__
#define __SWAMI_UTIL_H__

GType *swami_util_get_child_types (GType type, guint *n_types);
GValue *swami_util_new_value (void);
void swami_util_free_value (GValue *value);
void swami_util_midi_note_to_str (int note, char *str);
int swami_util_midi_str_to_note (const char *str);

#endif /* __SWAMI_UTIL_H__ */
