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
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "Python.h"

#define MAX_ID 16384
#define Packed __attribute__((__packed__))

enum CtrlInfo
{
    LastCtrlBlock = 0,
    FollowCtrlBlock = 0x3
};

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

typedef struct
{
    volatile uint16_t ctrlInfo : 2;
    volatile uint16_t ctrlInfoVersion : 14;
    volatile uint16_t ctrlInfoLock;
    volatile uint32_t freeMemOffset;
} Packed CtrlInfoBlock;

typedef struct
{
    volatile uint16_t ctrlInfo : 2;
    volatile uint16_t id : 14;
    volatile uint32_t size;
    volatile uint32_t offset;
} Packed SharedMemoryCtrl;

typedef struct
{
    volatile uint8_t version;
    volatile uint8_t nextVersion;
    uint8_t mem[0];
} Packed SharedMemoryLockable;

PyObject *py_init(PyObject *self, PyObject *args);
PyObject *py_resetAll(PyObject *self, PyObject *args);
PyObject *py_reset(PyObject *self, PyObject *args);
PyObject *py_freeMemory(PyObject *self, PyObject *args);
PyObject *py_getMemory(PyObject *self, PyObject *args);
PyObject *py_regMemory(PyObject *self, PyObject *args);
PyObject *py_acquireMemory(PyObject *self, PyObject *args);
PyObject *py_acquireMemoryCond(PyObject *self, PyObject *args);
PyObject *py_acquireMemoryTarget(PyObject *self, PyObject *args);
PyObject *py_acquireMemoryCondFunc(PyObject *self, PyObject *args); // Not suggest
PyObject *py_releaseMemory(PyObject *self, PyObject *args);
PyObject *py_releaseMemoryAndRollback(PyObject *self, PyObject *args);
PyObject *py_getMemoryVersion(PyObject *self, PyObject *args);
PyObject *py_incMemoryVersion(PyObject *self, PyObject *args);
