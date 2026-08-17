/*
 * Copyright (C) 2014-2015 Michael Petch <mpetch@capp-sysware.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * $Id: pylocdefs.h,v 1.9 2019/12/16 21:46:23 plm Exp $
 */

#ifndef PYLOCDEFS_H
#define PYLOCDEFS_H

#include "config.h"

#if defined(USE_PYTHON)

#if PY_MAJOR_VERSION >= 3
#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#define MOD_DEF(ob, name, doc, methods) \
            static struct PyModuleDef moduledef = { \
              PyModuleDef_HEAD_INIT, name, doc, -1, methods, \
              NULL, NULL, NULL, NULL, }; \
            ob = PyModule_Create(&moduledef)
#else
#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#define MOD_INIT(name) void init##name(void)
#define MOD_DEF(ob, name, doc, methods) \
            ob = Py_InitModule3(name, methods, doc)
#endif

#if PY_MAJOR_VERSION >= 3
#define PyInt_Check PyLong_Check
#define PyInt_AsLong PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong
#define PyExc_StandardError PyExc_Exception
#endif

#endif                          /* USE_PYTHON */

#endif
