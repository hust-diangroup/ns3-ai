# Copyright (c) 2019-2023 Huazhong University of Science and Technology
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
#         Muyuan Shen <muyuan_shen@hust.edu.cn>

import setuptools

name = "ns3ai_python_utils"

setuptools.setup(name=name,
                 version="1.0.0",
                 description="ns-3 AI Message Interface Utils for Python",
                 author="Pengyu Liu and Muyuan Shen",
                 author_email="muyuan_shen@hust.edu.cn",
                 packages=setuptools.find_packages(),
                 install_requires=["psutil"],
                 classifiers=[
                     "Programming Language :: Python :: 3",
                     "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
                     "Operating System :: POSIX :: Linux",
                 ],
                 py_modules=["ns3ai_utils"],
                 )
