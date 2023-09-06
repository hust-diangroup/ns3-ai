/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
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
 * Authors:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#ifndef NS3_AI_MSG_INTERFACE_H
#define NS3_AI_MSG_INTERFACE_H

#include "ns3-ai-semaphore.h"

#include <ns3/singleton.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace ns3
{

/**
 * \brief Structure containing semaphores used in msg interface
 */
struct Ns3AiMsgSync
{
    volatile uint8_t m_cpp2pyEmptyCount{1};
    volatile uint8_t m_cpp2pyFullCount{0};
    volatile uint8_t m_py2cppEmptyCount{1};
    volatile uint8_t m_py2cppFullCount{0};
    bool m_isFinished{false};
};

/**
 * \brief A template class implementation of the message interface
 */
template <typename Cpp2PyMsgType, typename Py2CppMsgType>
class Ns3AiMsgInterfaceImpl
{
  public:
    Ns3AiMsgInterfaceImpl() = delete;

    explicit Ns3AiMsgInterfaceImpl(bool is_memory_creator,
                                   bool use_vector,
                                   bool handle_finish,
                                   uint32_t size = 4096,
                                   const char* segment_name = "My Seg",
                                   const char* cpp2py_msg_name = "My Cpp to Python Msg",
                                   const char* py2cpp_msg_name = "My Python to Cpp Msg",
                                   const char* lockable_name = "My Lockable")
        : m_isCreator(is_memory_creator),
          m_useVector(use_vector),
          m_handleFinish(handle_finish),
          m_segName(segment_name),
          m_isFinished(false)
    {
        using namespace boost::interprocess;
        if (m_isCreator)
        {
            shared_memory_object::remove(m_segName.c_str());
            static managed_shared_memory segment(create_only, m_segName.c_str(), size);
            if (m_useVector)
            {
                static const Cpp2PyMsgAllocator alloc_env(segment.get_segment_manager());
                static const Cpp2PyMsgAllocator alloc_act(segment.get_segment_manager());
                m_cpp2pyVector = segment.construct<Cpp2PyMsgVector>(cpp2py_msg_name)(alloc_env);
                m_py2cppVector = segment.construct<Py2CppMsgVector>(py2cpp_msg_name)(alloc_act);
                m_cpp2pyStruct = nullptr;
                m_py2CppStruct = nullptr;
            }
            else
            {
                m_cpp2pyVector = nullptr;
                m_py2cppVector = nullptr;
                m_cpp2pyStruct = segment.construct<Cpp2PyMsgType>(cpp2py_msg_name)();
                m_py2CppStruct = segment.construct<Py2CppMsgType>(py2cpp_msg_name)();
            }
            m_sync = segment.construct<Ns3AiMsgSync>(lockable_name)();
        }
        else
        {
            static managed_shared_memory segment(open_only, segment_name);
            if (m_useVector)
            {
                m_cpp2pyVector = segment.find<Cpp2PyMsgVector>(cpp2py_msg_name).first;
                m_py2cppVector = segment.find<Py2CppMsgVector>(py2cpp_msg_name).first;
                m_cpp2pyStruct = nullptr;
                m_py2CppStruct = nullptr;
            }
            else
            {
                m_cpp2pyVector = nullptr;
                m_py2cppVector = nullptr;
                m_cpp2pyStruct = segment.find<Cpp2PyMsgType>(cpp2py_msg_name).first;
                m_py2CppStruct = segment.find<Py2CppMsgType>(py2cpp_msg_name).first;
            }
            m_sync = segment.find<Ns3AiMsgSync>(lockable_name).first;
        }
    };

    ~Ns3AiMsgInterfaceImpl()
    {
        if (m_isCreator)
        {
            boost::interprocess::shared_memory_object::remove(m_segName.c_str());
        }
        else
        {
            if (m_handleFinish)
            {
                CppSetFinished();
            }
        }
    };

    typedef boost::interprocess::
        allocator<Cpp2PyMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Cpp2PyMsgAllocator;
    typedef boost::interprocess::vector<Cpp2PyMsgType, Cpp2PyMsgAllocator> Cpp2PyMsgVector;
    typedef boost::interprocess::
        allocator<Py2CppMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Py2CppMsgAllocator;
    typedef boost::interprocess::vector<Py2CppMsgType, Py2CppMsgAllocator> Py2CppMsgVector;

    // use structure for the simple case:

    /**
     * Get the struct used in C++ to Python transmission in
     * struct-based message interface
     */
    Cpp2PyMsgType* GetCpp2PyStruct()
    {
        assert(!m_useVector);
        return m_cpp2pyStruct;
    };

    /**
     * Get the struct used in Python to C++ transmission in
     * struct-based message interface
     */
    Py2CppMsgType* GetPy2CppStruct()
    {
        assert(!m_useVector);
        return m_py2CppStruct;
    };

    // use vector for passing multiple structures at once:

    /**
     * Get the vector used in C++ to Python transmission in
     * vector-based message interface
     */
    Cpp2PyMsgVector* GetCpp2PyVector()
    {
        assert(m_useVector);
        return m_cpp2pyVector;
    };

    /**
     * Get the vector used in Python to C++ transmission in
     * vector-based message interface
     */
    Py2CppMsgVector* GetPy2CppVector()
    {
        assert(m_useVector);
        return m_py2cppVector;
    };

    // for C++ side:

    /**
     * C++ side starts writing into shared memory, struct-based
     * or vector-based
     */
    void CppSendBegin()
    {
        Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2pyEmptyCount);
    };

    /**
     * C++ side stops writing into shared memory, struct-based
     * or vector-based
     */
    void CppSendEnd()
    {
        Ns3AiSemaphore::sem_post(&m_sync->m_cpp2pyFullCount);
    };

    /**
     * C++ side starts reading from shared memory, struct-based
     * or vector-based
     */
    void CppRecvBegin()
    {
        Ns3AiSemaphore::sem_wait(&m_sync->m_py2cppFullCount);
    };

    /**
     * C++ side stops reading from shared memory, struct-based
     * or vector-based
     */
    void CppRecvEnd()
    {
        Ns3AiSemaphore::sem_post(&m_sync->m_py2cppEmptyCount);
    };

    /**
     * C++ side sets the overall status to finished when
     * the simulation is over
     */
    void CppSetFinished()
    {
        assert(m_handleFinish);
        m_isFinished = true;
        CppSendBegin();
        m_sync->m_isFinished = true;
        CppSendEnd();
    };

    // for Python side:

    /**
     * Python side starts reading from shared memory, struct-based
     * or vector-based
     */
    void PyRecvBegin()
    {
        Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2pyFullCount);
        if (m_handleFinish)
        {
            m_isFinished = m_sync->m_isFinished;
        }
    };

    /**
     * Python side stops reading from shared memory, struct-based
     * or vector-based
     */
    void PyRecvEnd()
    {
        Ns3AiSemaphore::sem_post(&m_sync->m_cpp2pyEmptyCount);
    };

    /**
     * Python side starts writing into shared memory, struct-based
     * or vector-based
     */
    void PySendBegin()
    {
        Ns3AiSemaphore::sem_wait(&m_sync->m_py2cppEmptyCount);
    };

    /**
     * Python side stops writing into shared memory, struct-based
     * or vector-based
     */
    void PySendEnd()
    {
        Ns3AiSemaphore::sem_post(&m_sync->m_py2cppFullCount);
    };

    /**
     * Python side gets whether the simulation is over
     */
    bool PyGetFinished()
    {
        assert(m_handleFinish);
        return m_isFinished;
    };

  private:
    Cpp2PyMsgType* m_cpp2pyStruct;
    Py2CppMsgType* m_py2CppStruct;
    Cpp2PyMsgVector* m_cpp2pyVector;
    Py2CppMsgVector* m_py2cppVector;

    Ns3AiMsgSync* m_sync;
    const bool m_isCreator;
    const bool m_useVector;
    const bool m_handleFinish;
    const std::string m_segName;
    bool m_isFinished;
};

/**
 * \brief The message interface, a singleton class
 */

class Ns3AiMsgInterface : public Singleton<Ns3AiMsgInterface>
{
  public:
    /**
     * Sets if this side (C++ or Python) is the memory creator.
     * Configuration on two sides must be different
     */
    void SetIsMemoryCreator(bool isMemoryCreator)
    {
        this->m_isMemoryCreator = isMemoryCreator;
    };

    /**
     * Sets if both C++ and Python sides use vector. Configuration on
     * two sides must be same
     */
    void SetUseVector(bool useVector)
    {
        this->m_useVector = useVector;
    };

    /**
     * Sets if both C++ and Python sides handle finish. Configuration on
     * two sides must be same
     */
    void SetHandleFinish(bool handleFinish)
    {
        this->m_handleFinish = handleFinish;
    };

    /**
     * Sets shared memory segment size, only valid for
     * the shared memory creator. Normally the default
     * size is OK.
     */
    void SetMemorySize(uint32_t size)
    {
        this->m_size = size;
    };

    /**
     * Sets the names of the named objects. See Boost's
     * documentation for details. Normally the default
     * names are OK.
     */
    void SetNames(std::string segmentName,
                  std::string cpp2pyMsgName,
                  std::string py2cppMsgName,
                  std::string lockableName)
    {
        this->m_segmentName = segmentName;
        this->m_cpp2pyMsgName = cpp2pyMsgName;
        this->m_py2cppMsgName = py2cppMsgName;
        this->m_lockableName = lockableName;
    };

    /**
     * Gets the impl which has semaphore (synchronization)
     * methods
     */
    template <typename Cpp2PyMsgType, typename Py2CppMsgType>
    Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>* GetInterface()
    {
        static Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType> interface(
            this->m_isMemoryCreator,
            this->m_useVector,
            this->m_handleFinish,
            this->m_size,
            this->m_segmentName.c_str(),
            this->m_cpp2pyMsgName.c_str(),
            this->m_py2cppMsgName.c_str(),
            this->m_lockableName.c_str());
        return &interface;
    };

  private:
    bool m_isMemoryCreator;
    bool m_useVector;
    bool m_handleFinish;
    uint32_t m_size = 4096;
    std::string m_segmentName = "My Seg";
    std::string m_cpp2pyMsgName = "My Cpp to Python Msg";
    std::string m_py2cppMsgName = "My Python to Cpp Msg";
    std::string m_lockableName = "My Lockable";
};

} // namespace ns3

#endif // NS3_AI_MSG_INTERFACE_H
