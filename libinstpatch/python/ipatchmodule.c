#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>
#include <libinstpatch/libinstpatch.h>

void pyipatch_register_classes (PyObject *d);
void pyipatch_add_constants (PyObject *module, const gchar *strip_prefix);
static void pyipatch_register_boxed_types (PyObject *moddict);

extern PyMethodDef pyipatch_functions[];

DL_EXPORT(void)
initipatch(void)
{
    PyObject *m, *d;
	
    init_pygobject ();
    ipatch_init ();		/* initialize libInstPatch */

    m = Py_InitModule ("ipatch", pyipatch_functions);
    d = PyModule_GetDict (m);
	
    pyipatch_register_classes (d);
    pyipatch_add_constants(m, "IPATCH_");
    pyipatch_register_boxed_types (d);

    if (PyErr_Occurred ())
      Py_FatalError ("can't initialise module ipatch");
}

gboolean
pyipatch_range_from_pyobject (PyObject *object, IpatchRange *range)
{
    g_return_val_if_fail (range != NULL, FALSE);

    if (pyg_boxed_check (object, IPATCH_TYPE_RANGE)) {
        *range = *pyg_boxed_get (object, IpatchRange);
        return TRUE;
    }

    if (PyArg_ParseTuple (object, "ii", &range->low, &range->high))
        return TRUE;

    PyErr_Clear ();
    PyErr_SetString (PyExc_TypeError, "could not convert to IpatchRange");

    return FALSE;
}

static PyObject *
PyIpatchRange_from_value (const GValue *value)
{
    IpatchRange *range = (IpatchRange *)g_value_get_boxed (value);

    return pyg_boxed_new(IPATCH_TYPE_RANGE, range, TRUE, TRUE);
}

static int
PyIpatchRange_to_value (GValue *value, PyObject *object)
{
    IpatchRange range;

    if (!pyipatch_range_from_pyobject (object, &range))
        return -1;

    g_value_set_boxed (value, &range);

    return 0;
}

static void
pyipatch_register_boxed_types (PyObject *moddict)
{
    pyg_register_boxed_custom (IPATCH_TYPE_RANGE,
                               PyIpatchRange_from_value,
                               PyIpatchRange_to_value);
}
