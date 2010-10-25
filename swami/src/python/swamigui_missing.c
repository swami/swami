#include <Python.h>

#define NO_IMPORT_PYGOBJECT
#include "pygobject.h"

#include <libgnomecanvas/libgnomecanvas.h>

static PyTypeObject *_PyGtkObject_Type;
#define PyGtkObject_Type (*_PyGtkObject_Type)

PyTypeObject PyGnomeCanvasItem_Type;
PyTypeObject PyGnomeCanvasGroup_Type;


static int
pygobject_no_constructor(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    g_snprintf(buf, sizeof(buf), "%s is an abstract widget", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return -1;
}

PyTypeObject PyGnomeCanvasItem_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "SwamiGui.GnomeCanvasItem",		/* tp_name */
    sizeof(PyGObject),	        /* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)0,			/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,	/* tp_getattr */
    (setattrfunc)0,	/* tp_setattr */
    (cmpfunc)0,		/* tp_compare */
    (reprfunc)0,		/* tp_repr */
    0,			/* tp_as_number */
    0,		/* tp_as_sequence */
    0,			/* tp_as_mapping */
    (hashfunc)0,		/* tp_hash */
    (ternaryfunc)0,		/* tp_call */
    (reprfunc)0,		/* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    NULL, 				/* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,			/* tp_clear */
    (richcmpfunc)0,	/* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,		/* tp_iter */
    (iternextfunc)0,	/* tp_iternext */
    NULL,			/* tp_methods */
    0,					/* tp_members */
    0,		       	/* tp_getset */
    NULL,				/* tp_base */
    NULL,				/* tp_dict */
    (descrgetfunc)0,	/* tp_descr_get */
    (descrsetfunc)0,	/* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)pygobject_no_constructor,		/* tp_init */
};

PyTypeObject PyGnomeCanvasGroup_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "SwamiGui.GnomeCanvasGroup",		/* tp_name */
    sizeof(PyGObject),	        /* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)0,			/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,	/* tp_getattr */
    (setattrfunc)0,	/* tp_setattr */
    (cmpfunc)0,		/* tp_compare */
    (reprfunc)0,		/* tp_repr */
    0,			/* tp_as_number */
    0,		/* tp_as_sequence */
    0,			/* tp_as_mapping */
    (hashfunc)0,		/* tp_hash */
    (ternaryfunc)0,		/* tp_call */
    (reprfunc)0,		/* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    NULL, 				/* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,			/* tp_clear */
    (richcmpfunc)0,	/* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,		/* tp_iter */
    (iternextfunc)0,	/* tp_iternext */
    NULL,			/* tp_methods */
    0,					/* tp_members */
    0,		       	/* tp_getset */
    NULL,				/* tp_base */
    NULL,				/* tp_dict */
    (descrgetfunc)0,	/* tp_descr_get */
    (descrsetfunc)0,	/* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)pygobject_no_constructor,		/* tp_init */
};

/* intialise stuff extension classes */
void
pyswamigui_register_missing_classes (PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gtk")) != NULL) {
        PyObject *moddict = PyModule_GetDict(module);

        _PyGtkObject_Type = (PyTypeObject *)PyDict_GetItemString(moddict,
								 "GtkObject");
        if (_PyGtkObject_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name GtkObject from gtk");
            return;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gtk");
        return;
    }

    pygobject_register_class(d, "GnomeCanvasItem", GNOME_TYPE_CANVAS_ITEM, &PyGnomeCanvasItem_Type, Py_BuildValue("(O)", &PyGtkObject_Type));
    pygobject_register_class(d, "GnomeCanvasGroup", GNOME_TYPE_CANVAS_GROUP, &PyGnomeCanvasGroup_Type, Py_BuildValue("(O)", &PyGnomeCanvasItem_Type));
}
