# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ns3-ai', ['core'])
    module.source = [
        'model/memory-pool.cc',
        ]

    module_test = bld.create_ns3_module_test_library('ns3-ai')
    module_test.source = [
        ]

    headers = bld(features='ns3header')
    headers.module = 'ns3-ai'
    headers.source = [
        'model/memory-pool.h',
        'model/train-var.h',
        'model/ns3-ai-rl.h',
        'model/ns3-ai-dl.h',
        ]

    # if bld.env.ENABLE_EXAMPLES:
    #     bld.recurse('examples')

    # bld.ns3_python_bindings()

