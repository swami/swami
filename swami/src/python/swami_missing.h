/*
 * swami_missing.h: Provides missing type definitions for swamimodule
 * that should probably be handled by other library bindings.
 */

#include <Python.h>

#define NO_IMPORT_PYGOBJECT
#include "pygobject.h"

extern PyTypeObject PyGTypeModule_Type;
