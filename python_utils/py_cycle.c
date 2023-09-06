#define PY_SSIZE_T_CLEAN
#include <Python.h>

uint64_t
rdtsc()
{
#ifdef __x86_64__
    unsigned long lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) + lo;
#else
    return 0;
#endif
}

static PyObject*
py_getCycle(PyObject* self, PyObject* args)
{
    return Py_BuildValue("K", rdtsc());
}

static PyMethodDef py_cycle_methods[] = {{"getCycle", py_getCycle, METH_NOARGS, "Get cpu cycle"},
                                         {NULL, NULL, 0, NULL}};

static struct PyModuleDef py_cycle_module = {PyModuleDef_HEAD_INIT,
                                             "py_cycle",
                                             "Get CPU cycle",
                                             -1,
                                             py_cycle_methods};

PyMODINIT_FUNC
PyInit_py_cycle(void)
{
    PyObject* m;

    if ((m = PyModule_Create(&py_cycle_module)) == NULL)
        return NULL;
    return m;
}
