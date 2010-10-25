/*
 * swami_python.h - Header file for Python interpreter functions
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
#ifndef __SWAMIGUI_SWAMI_PYTHON_H__
#define __SWAMIGUI_SWAMI_PYTHON_H__

#include <glib.h>

typedef void (*SwamiguiPythonOutputFunc)(const char *output,
					gboolean is_stderr);

void swamigui_python_set_output_func (SwamiguiPythonOutputFunc func);
void swamigui_python_set_root (void);
gboolean swamigui_python_is_initialized (void);

#endif
