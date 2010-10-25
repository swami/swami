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
