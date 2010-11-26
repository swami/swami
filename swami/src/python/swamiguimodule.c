/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>
#include <swamigui/swamigui.h>
#include "swamigui_missing.h"

void pyswamigui_register_missing_classes (PyObject *d);
void pyswamigui_register_classes (PyObject *d);
void pyswamigui_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pyswamigui_functions[];

DL_EXPORT(void)
initswamigui(void)
{
    PyObject *m, *d;

    init_pygobject ();
    swamigui_init (NULL, NULL);	/* initialize swamigui */

    m = Py_InitModule ("swamigui", pyswamigui_functions);
    d = PyModule_GetDict (m);
	
    pyswamigui_register_missing_classes (d);
    pyswamigui_register_classes (d);
    pyswamigui_add_constants(m, "SWAMIGUI_");

    if (PyErr_Occurred ())
      {
	PyErr_Print ();
	Py_FatalError ("can't initialise module swamigui");
      }
}
