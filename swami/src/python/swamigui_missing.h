/*
 * swamigui_missing.h: Provides missing type definitions for
 * swamiguimodule that should probably be handled by other library bindings.
 */

#include <Python.h>

#define NO_IMPORT_PYGOBJECT
#include "pygobject.h"

extern PyTypeObject PyGnomeCanvasItem_Type;
extern PyTypeObject PyGnomeCanvasGroup_Type;
