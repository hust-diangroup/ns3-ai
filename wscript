# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('shared-memory', ['core'])
    module.source = [
        'model/memory-pool.cc',
        ]

    module_test = bld.create_ns3_module_test_library('shared-memory')
    module_test.source = [
        'test/shared-memory-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'shared-memory'
    headers.source = [
        'model/memory-pool.h',
        'model/shm-var.h',
        'model/shm-rl.h',
        ]

    # if bld.env.ENABLE_EXAMPLES:
    #     bld.recurse('examples')

    # bld.ns3_python_bindings()

