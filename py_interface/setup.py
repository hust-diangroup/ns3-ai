# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
# Copyright (c) 2019 Huazhong University of Science and Technology, Dian Group
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Pengyu Liu <eic_lpy@hust.edu.cn>

import setuptools

name = "ns3_ai"
with open("README.md", "r") as fh:
    long_description = fh.read()

extension = setuptools.Extension("shm_pool",
                                 ["memory-pool.c", "memory-pool-module.c"],
                                 # extra_compile_args=['-E'],
                                 depends=[
                                     "memory-pool.c",
                                     "memory-pool.h",
                                     "memory-pool-module.c",
                                     "setup.py",
                                 ],
                                 )

setuptools.setup(name=name,
                 version='0.0.2',
                 description="NS-3 AI interface for Python",
                 long_description=long_description,
                 long_description_content_type="text/markdown",
                 author="Pengyu Liu",
                 author_email="eic_lpy@hust.edu.cn",
                 url="placeholder",
                 packages=setuptools.find_packages(),
                 install_requires=["psutil==5.7.2"],
                 classifiers=[
                    "Programming Language :: Python :: 3",
                    "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
                    "Operating System :: POSIX :: Linux",
                 ],
                 py_modules=['py_interface', 'ns3_util'],
                 ext_modules=[extension]
                 )
