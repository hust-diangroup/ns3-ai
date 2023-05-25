from libcpp.vector cimport vector

cdef extern from "ns3-ai-new-rl.h" namespace "ns3":
    cdef cppclass NS3AIRL[EnvType, ActType]:
        NS3AIRL(unsigned int, bint, char*, char*, char*, char*) except +
        void set_env()
        void get_env()
        void set_act()
        void get_act()
        bint is_finished()
        vector[EnvType] m_local_env
        vector[ActType] m_local_act
