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
#         Hao Yin <haoyin@uw.edu>

import os
import subprocess

try:
    devnull = subprocess.DEVNULL
except:
    devnull = open(os.devnull, 'w')


def get_setting(setting_map):
    ret = ''
    for key, value in setting_map.items():
        ret += ' --{}={}'.format(key, value)
    return ret


def build_ns3(path):
    print('build')
    proc = subprocess.Popen('./ns3 build', shell=True, stdout=subprocess.PIPE,
                            stderr=devnull, universal_newlines=True, cwd=path)
    proc.wait()
    for line in proc.stdout:
        if ("error" in line) or ("Error" in line):
            return False
    return True


def run_single_ns3(path, pname, setting=None, env=None, show_output=False):
    if env:
        env.update(os.environ)
    env['LD_LIBRARY_PATH'] = os.path.abspath(os.path.join(path, 'build', 'lib'))
    # import pdb; pdb.set_trace()
    exec_path = os.path.join(path, 'ns3')
    if not setting:
        cmd = '{} run {}'.format(exec_path, pname)
    else:
        cmd = '{} run {} --{}'.format(exec_path, pname, get_setting(setting))
    print("Running ns3 with: ", cmd)
    if show_output:
        proc = subprocess.Popen(
            cmd, shell=True, universal_newlines=True, env=env)
    else:
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=devnull, universal_newlines=True, env=env)

    return proc


# # This class set up the ns-3 environment and built the shared memory pool.
# class Experiment:
#     _created = False
#
#     # init ns-3 environment
#     # \param[in] shmKey : share memory key
#     # \param[in] memSize : share memory size
#     # \param[in] programName : program name of ns3
#     # \param[in] path : current working directory
#     def __init__(self, programName, ns3Path, shmSize, segName, envName, actName, lockName):
#         if self._created:
#             raise Exception('Experiment is singleton')
#         self.programName = programName
#         self.ns3Path = ns3Path
#         self.shmSize = shmSize
#         self.segName = segName
#         self.envName = envName
#         self.actName = actName
#         self.lockName = lockName
#         self.proc = None
#         self.dirty = True
#         Init(shmKey, memSize)
#
#     def __del__(self):
#         print('cleaning')
#         self.kill()
#         FreeMemory()
#
#     # run ns3 script in cmd with the setting being input
#     # \param[in] setting : ns3 script input parameters(default : None)
#     # \param[in] show_output : whether to show output or not(default : False)
#     def run(self, setting=None, show_output=False):
#         self.kill()
#         env = {'NS_GLOBAL_VALUE': 'SharedMemoryKey={};SharedMemoryPoolSize={};'.format(
#             self.shmKey, self.memSize)}
#         #
#         #print(self.using_waf)
#         self.proc = run_single_ns3(
#             self.path, self.programName, setting, env=env, show_output=show_output, build=self.dirty, using_waf = self.using_waf)
#         self.dirty = False
#         return self.proc
#
#     def kill(self):
#         if self.proc and self.isalive():
#             kill_proc_tree(self.proc)
#             self.proc = None
#
#     def reset(self):
#         self.kill()
#         Reset()
#
#     def isalive(self):
#         return self.proc.poll() == None


__all__ = ['get_setting', 'build_ns3', 'run_single_ns3']
