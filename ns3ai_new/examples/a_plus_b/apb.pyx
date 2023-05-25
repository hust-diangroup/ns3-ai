# distutils: language = c++

from ns3_ai_new_rl cimport NS3AIRL

cdef extern from "apb.h":
    ctypedef struct EnvStruct:
        unsigned int env_a
        unsigned int env_b
    
    ctypedef struct ActStruct:
        unsigned int act_c

cdef class ApbNS3AIRL:
    cdef NS3AIRL[EnvStruct, ActStruct] *c_rl
    cdef ActStruct temp_act

    def __cinit__(self, unsigned int size, bint is_memory_creator, char *segment_name = "My Seg", char *env_name = "My Env", char *act_name = "My Act", char *lockable_name = "My Lockable"):
        self.c_rl = new NS3AIRL[EnvStruct, ActStruct](size, is_memory_creator, segment_name, env_name, act_name, lockable_name)

    def get_env(self):
        self.c_rl.get_env()
        return self.c_rl.m_local_env

    def local_act_clear(self):
        self.c_rl.m_local_act.clear()

    def local_act_push_back(self, unsigned int c):
        self.temp_act.act_c = c
        self.c_rl.m_local_act.push_back(self.temp_act)

    def set_act(self):
        self.c_rl.set_act()

    def is_finished(self):
        return self.c_rl.is_finished()

