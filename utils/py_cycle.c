#define PY_SSIZE_T_CLEAN
#include <Python.h>

uint64_t rdtsc()
{
    unsigned long lo, hi;
    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) + lo;
}

static PyObject *py_getCycle(PyObject *self, PyObject *args)
{
    return Py_BuildValue("K", rdtsc());
}

uint64_t gTimestamp[256];
uint64_t gTimestampTotal[256];
uint64_t gTimestampCount[256];
uint64_t gTimestampMax[256];
uint64_t gTimestampMin[256];

static PyObject *py_timerDiff(PyObject *self, PyObject *args)
{
    unsigned char id;
    if (__builtin_expect(!PyArg_ParseTuple(args, "B", &id), 0))
        return NULL;
    register uint64_t ret = gTimestamp[id];
    gTimestamp[id] = rdtsc();
    if (__builtin_expect((ret == 0), 0))
        Py_RETURN_NONE;
    ret = gTimestamp[id] - ret;
    gTimestampTotal[id] += ret;
    if (__builtin_expect((ret > gTimestampMax[id]), 0))
        gTimestampMax[id] = ret;
    if (__builtin_expect((ret < gTimestampMin[id]), 0))
        gTimestampMin[id] = ret;
    ++gTimestampCount[id];
    return Py_BuildValue("K", ret);
}

static PyObject *py_timeDiffStatistic(PyObject *self, PyObject *args)
{
    unsigned char id;
    if (__builtin_expect(!PyArg_ParseTuple(args, "B", &id), 0))
        return NULL;
    if (__builtin_expect((!gTimestampCount[id]), 0))
    {
        PyErr_Format(PyExc_RuntimeError, "ID: %u don't has statistic info", id);
        return NULL;
    }
    return Py_BuildValue("(KKdKK)", gTimestampCount[id], gTimestampTotal[id], 1.0 * gTimestampTotal[id] / gTimestampCount[id], gTimestampMin[id], gTimestampMax[id]);
}

static PyObject *py_timerDiffNoStatistic(PyObject *self, PyObject *args)
{
    unsigned char id;
    if (__builtin_expect(!PyArg_ParseTuple(args, "B", &id), 0))
        return NULL;
    register uint64_t temp = gTimestamp[id];
    gTimestamp[id] = rdtsc();
    if (__builtin_expect((temp == 0), 0))
        Py_RETURN_NONE;
    return Py_BuildValue("K", gTimestamp[id] - temp);
}

static PyMethodDef py_cycle_methods[] = {
    {"getCycle", py_getCycle, METH_NOARGS,
     "Get cpu cycle"},
    {"getStatistic", py_timeDiffStatistic, METH_VARARGS,
     "Get statistic information"},
    {"cycleDiffNoSta", py_timerDiffNoStatistic, METH_VARARGS,
     "Get cpu cycle diff of timer id(No statistics)"},
    {"cycleDiff", py_timerDiff, METH_VARARGS,
     "Get cpu cycle diff of timer id"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef py_cycle_module = {
    PyModuleDef_HEAD_INIT,
    "py_cycle",
    "Get CPU cycle",
    -1,
    py_cycle_methods};

PyMODINIT_FUNC
PyInit_py_cycle(void)
{
    PyObject *m;

    if ((m = PyModule_Create(&py_cycle_module)) == NULL)
        return NULL;
    for (int i = 0; i < 256; ++i)
        gTimestampMin[i] = 18446744073709551615UL;
    return m;
}