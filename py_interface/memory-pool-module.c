/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Huazhong University of Science and Technology, Dian Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Pengyu Liu <eic_lpy@hust.edu.cn>
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
// #include "structmember.h"
#include "memory-pool.h"

/* Module method table */
static PyMethodDef MemoryPool_methods[] = {
    {"Init",
     py_init,
     METH_VARARGS,
     "Init shared memory pool"},
    {"ResetAll",
     py_resetAll,
     METH_NOARGS,
     "Reset shared memory pool(including ctrl block)"},
    {"Reset",
     py_reset,
     METH_NOARGS,
     "Reset shared memory pool"},
    {"FreeMemory",
     py_freeMemory,
     METH_NOARGS,
     "Free shared memory pool"},
    {"GetMemory",
     py_getMemory,
     METH_VARARGS,
     "Get memory of id"},
    {"RegisterMemory",
     py_regMemory,
     METH_VARARGS,
     "Register memory of id"},
    {"AcquireMemory",
     py_acquireMemory,
     METH_VARARGS,
     "Acquire memory of id"},
    {"AcquireMemoryCond",
     py_acquireMemoryCond,
     METH_VARARGS,
     "Acquire memory of id"},
    {"AcquireMemoryTarget",
     py_acquireMemoryTarget,
     METH_VARARGS,
     "Acquire memory of id"},
    {"AcquireMemoryCondFunc",
     py_acquireMemoryCondFunc,
     METH_VARARGS,
     "Acquire memory of id"},
    {"ReleaseMemory",
     py_releaseMemory,
     METH_VARARGS,
     "Release memory of id"},
    {"ReleaseMemoryRB",
     py_releaseMemoryAndRollback,
     METH_VARARGS,
     "Release memory of id and roll back memory version"},
    {"GetMemoryVersion",
     py_getMemoryVersion,
     METH_VARARGS,
     "Get memory version of id"},
    {"IncMemoryVersion",
     py_incMemoryVersion,
     METH_VARARGS,
     "Inc memory version of id"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/* Module structure */
static struct PyModuleDef shmModule = {
    PyModuleDef_HEAD_INIT,
    "shm_pool",           /* name of module */
    "Shared memory pool", /* Doc string (may be NULL) */
    -1,                   /* Size of per-interpreter state or -1 */
    MemoryPool_methods    /* Method table */
};

/* Module initialization function */
PyMODINIT_FUNC
PyInit_shm_pool(void)
{
    return PyModule_Create(&shmModule);
}