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
