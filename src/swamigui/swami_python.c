/*
 * python.c - Python interpreter functions
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
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
#include <stdio.h>
#include <glib.h>
#include <Python.h>
#include "swami_python.h"

static void default_redir_func(const char *output, gboolean is_stderr);

static SwamiguiPythonOutputFunc output_func = default_redir_func;


static PyObject *
redir_stdout(PyObject *self, PyObject *args)
{
    char *s;

    if(!PyArg_ParseTuple(args, "s", &s))
    {
        return 0;
    }

    if(output_func)
    {
        output_func(s, FALSE);
    }

    return Py_BuildValue("");
}

static PyObject *
redir_stderr(PyObject *self, PyObject *args)
{
    char *s;

    if(!PyArg_ParseTuple(args, "s", &s))
    {
        return 0;
    }

    if(output_func)
    {
        output_func(s, TRUE);
    }

    return Py_BuildValue("");
}

// Define methods available to Python
static PyMethodDef ioMethods[] =
{
    {"redir_stdout", redir_stdout, METH_VARARGS, "redir_stdout"},
    {"redir_stderr", redir_stderr, METH_VARARGS, "redir_stderr"},
    {NULL, NULL, 0, NULL}
};

static gboolean python_is_initialized = FALSE;

/*
 * Initialize python for use with Swami. Sets up python output redirection.
 * Usually called once and only once by swamigui_init().
 */
void
_swamigui_python_init(int argc, char **argv)
{
    PyObject *swami_main_module;
    char *new_argv[argc];
    int i;
    char *init_code =
        "class Sout:\n"
        "    def write(self, s):\n"
        "        swami_redirect.redir_stdout(s)\n"
        "\n"
        "class Eout:\n"
        "    def write(self, s):\n"
        "        swami_redirect.redir_stderr(s)\n"
        "\n"
        "import sys\n"
        "import swami_redirect\n"
        "sys.stdout = Sout()\n"
        "sys.stderr = Eout()\n"
        "sys.stdin  = None\n";

    if(python_is_initialized)
    {
        return;
    }

    Py_Initialize();

    for(i = 1; i < argc; i++)
    {
        new_argv[i] = argv[i];
    }

    new_argv[0] = "";		/* set "script name" to empty string */

    PySys_SetArgv(argc, new_argv);

    swami_main_module = Py_InitModule("swami_redirect", ioMethods);

    if(PyRun_SimpleString(init_code) != 0)
    {
        g_critical("Failed to redirect Python output");
    }

    if(PyRun_SimpleString("import ipatch, swami, swamigui\n") != 0)
    {
        g_critical("Failed to 'import ipatch, swami, swamigui' modules");
    }

    python_is_initialized = TRUE;
}

/**
 * swamigui_python_set_output_func:
 * @func: Python output callback function or %NULL to use default (no
 *   redirection)
 *
 * Set the Python output callback function which gets called for any
 * output to stdout or stderr from the Python interpreter.
 */
void
swamigui_python_set_output_func(SwamiguiPythonOutputFunc func)
{
    output_func = func ? func : default_redir_func;
}

/* a default redirection function, which doesn't redirect at all :) */
static void
default_redir_func(const char *output, gboolean is_stderr)
{
    fputs(output, is_stderr ? stderr : stdout);
}

/**
 * swamigui_python_set_root:
 *
 * Runs a bit of Python code to set the swamigui.root variable to the global
 * swamigui_root object.
 */
void
swamigui_python_set_root(void)
{
    if(PyRun_SimpleString("swamigui.root = swamigui.swamigui_get_root()\n") != 0)
    {
        g_critical("Failed to assign swamigui.root object");
    }
}

/**
 * swamigui_python_is_initialized:
 *
 * Check if Python sub system is initialized and ready for action.
 *
 * Returns: TRUE if initialized, FALSE otherwise
 */
gboolean
swamigui_python_is_initialized(void)
{
    return python_is_initialized;
}
