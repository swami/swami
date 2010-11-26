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
#include <libswami/libswami.h>
#include <libinstpatch/libinstpatch.h>
#include "swami_missing.h"

void pyswami_register_missing_classes (PyObject *d);
void pyswami_register_classes (PyObject *d);
void pyswami_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pyswami_functions[];

DL_EXPORT(void)
initswami(void)
{
    PyObject *m, *d;
	
    init_pygobject ();
    swami_init ();		/* initialize libswami */

    m = Py_InitModule ("swami", pyswami_functions);
    d = PyModule_GetDict (m);
	
    pyswami_register_missing_classes (d);
    pyswami_register_classes (d);
    pyswami_add_constants(m, "SWAMI_");

    if (PyErr_Occurred ())
      {
	PyErr_Print ();
	Py_FatalError ("can't initialise module swami");
      }
}
