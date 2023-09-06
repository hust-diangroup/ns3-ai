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
#         Hao Yin <haoyin@uw.edu>
#         Muyuan Shen <muyuan_shen@hust.edu.cn>

import os
import subprocess
import psutil
import time
import signal


SIMULATION_EARLY_ENDING = 0.5   # wait and see if the subprocess is running after creation


def get_setting(setting_map):
    ret = ''
    for key, value in setting_map.items():
        ret += ' --{}={}'.format(key, value)
    return ret


def run_single_ns3(path, pname, setting=None, env=None, show_output=False):
    if env is None:
        env = {}
    env.update(os.environ)
    env['LD_LIBRARY_PATH'] = os.path.abspath(os.path.join(path, 'build', 'lib'))
    # import pdb; pdb.set_trace()
    exec_path = os.path.join(path, 'ns3')
    if not setting:
        cmd = '{} run {}'.format(exec_path, pname)
    else:
        cmd = '{} run {} --{}'.format(exec_path, pname, get_setting(setting))
    if show_output:
        proc = subprocess.Popen(cmd, shell=True, text=True, env=env,
                                stdin=subprocess.PIPE,
                                preexec_fn=os.setpgrp)
    else:
        proc = subprocess.Popen(cmd, shell=True, text=True, env=env,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                preexec_fn=os.setpgrp)

    return cmd, proc


# used to kill the ns-3 script process and its child processes
def kill_proc_tree(p, timeout=None, on_terminate=None):
    print('ns3ai_utils: Killing subprocesses...')
    if isinstance(p, int):
        p = psutil.Process(p)
    elif not isinstance(p, psutil.Process):
        p = psutil.Process(p.pid)
    ch = [p]+p.children(recursive=True)
    for c in ch:
        try:
            # print("\t-- {}, pid={}, ppid={}".format(psutil.Process(c.pid).name(), c.pid, c.ppid()))
            # print("\t   \"{}\"".format(" ".join(c.cmdline())))
            c.kill()
        except Exception as e:
            continue
    succ, err = psutil.wait_procs(ch, timeout=timeout,
                                  callback=on_terminate)
    return succ, err


# According to Python signal docs, after a signal is received, the
# low-level signal handler sets a flag which tells the virtual machine
# to execute the corresponding Python signal handler at a later point.
#
# As a result, a long ns-3 simulation, during which no C++-Python
# interaction occurs (such as the Multi-BSS example), may run uninterrupted
# for a long time regardless of any signals received.
def sigint_handler(sig, frame):
    print("\nns3ai_utils: SIGINT detected")
    exit(1)  # this will execute the `finally` block


# This class sets up the shared memory and runs the simulation process.
class Experiment:
    _created = False

    # init ns-3 environment
    # \param[in] memSize : share memory size
    # \param[in] targetName : program name of ns3
    # \param[in] path : current working directory
    def __init__(self, targetName, ns3Path, msgModule,
                 handleFinish=False,
                 useVector=False, vectorSize=None,
                 shmSize=4096,
                 segName="My Seg",
                 cpp2pyMsgName="My Cpp to Python Msg",
                 py2cppMsgName="My Python to Cpp Msg",
                 lockableName="My Lockable"):
        if self._created:
            raise Exception('ns3ai_utils: Error: Experiment is singleton')
        self._created = True
        self.targetName = targetName  # ns-3 target name, not file name
        os.chdir(ns3Path)
        self.msgModule = msgModule
        self.handleFinish = handleFinish
        self.useVector = useVector
        self.vectorSize = vectorSize
        self.shmSize = shmSize
        self.segName = segName
        self.cpp2pyMsgName = cpp2pyMsgName
        self.py2cppMsgName = py2cppMsgName
        self.lockableName = lockableName

        self.msgInterface = msgModule.Ns3AiMsgInterfaceImpl(
            True, self.useVector, self.handleFinish,
            self.shmSize, self.segName, self.cpp2pyMsgName, self.py2cppMsgName, self.lockableName
        )
        if self.useVector:
            if self.vectorSize is None:
                raise Exception('ns3ai_utils: Error: Using vector but size is unknown')
            self.msgInterface.GetCpp2PyVector().resize(self.vectorSize)
            self.msgInterface.GetPy2CppVector().resize(self.vectorSize)

        self.proc = None
        self.simCmd = None
        print('ns3ai_utils: Experiment initialized')

    def __del__(self):
        self.kill()
        del self.msgInterface
        print('ns3ai_utils: Experiment destroyed')

    # run ns3 script in cmd with the setting being input
    # \param[in] setting : ns3 script input parameters(default : None)
    # \param[in] show_output : whether to show output or not(default : False)
    def run(self, setting=None, show_output=False):
        self.kill()
        self.simCmd, self.proc = run_single_ns3(
            './', self.targetName, setting=setting, show_output=show_output)
        print("ns3ai_utils: Running ns-3 with: ", self.simCmd)
        # exit if an early error occurred, such as wrong target name
        time.sleep(SIMULATION_EARLY_ENDING)
        if not self.isalive():
            print('ns3ai_utils: Subprocess died very early')
            exit(1)
        signal.signal(signal.SIGINT, sigint_handler)
        return self.msgInterface

    def kill(self):
        if self.proc and self.isalive():
            kill_proc_tree(self.proc)
            self.proc = None
            self.simCmd = None

    def isalive(self):
        return self.proc.poll() is None


__all__ = ['Experiment']
