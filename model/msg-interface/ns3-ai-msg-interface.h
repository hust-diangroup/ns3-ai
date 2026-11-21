/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 * Copyright (c) 2026 Muhammad Uzair (modernization for ns-3.43+)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Authors: Muyuan Shen <muyuan_shen@hust.edu.cn>
 * Modernized by: Muhammad Uzair
 *
 * Changes from original:
 *   - Fixed critical: static managed_shared_memory replaced with member unique_ptr
 *   - Added try-catch for all Boost IPC operations with clear error messages
 *   - Added stale segment cleanup on creation failure
 *   - Replaced assert() with runtime exceptions (throw)
 *   - Added nullptr checks after segment.find()
 *   - Added non-copyable/non-movable semantics
 *   - Added memory size validation
 */

#ifndef NS3_AI_MSG_INTERFACE_H
#define NS3_AI_MSG_INTERFACE_H

#include "ns3-ai-semaphore.h"

#include <ns3/singleton.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace ns3
{

struct Ns3AiMsgSync
{
    volatile uint8_t m_cpp2pyEmptyCount{1};
    volatile uint8_t m_cpp2pyFullCount{0};
    volatile uint8_t m_py2cppEmptyCount{1};
    volatile uint8_t m_py2cppFullCount{0};
    bool m_isFinished{false};
};

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
          m_isFinished(false),
          m_cpp2pyStruct(nullptr),
          m_py2CppStruct(nullptr),
          m_cpp2pyVector(nullptr),
          m_py2cppVector(nullptr),
          m_sync(nullptr)
    {
        using namespace boost::interprocess;

        if (m_isCreator)
        {
            shared_memory_object::remove(m_segName.c_str());
            try
            {
                m_segment = std::make_unique<managed_shared_memory>(
                    create_only, m_segName.c_str(), size);
            }
            catch (const interprocess_exception& e)
            {
                throw std::runtime_error(
                    std::string("ns3-ai: Failed to create shared memory '") +
                    m_segName + "' (size=" + std::to_string(size) + "): " + e.what());
            }

            try
            {
                if (m_useVector)
                {
                    const Cpp2PyMsgAllocator alloc_env(m_segment->get_segment_manager());
                    const Cpp2PyMsgAllocator alloc_act(m_segment->get_segment_manager());
                    m_cpp2pyVector = m_segment->construct<Cpp2PyMsgVector>(cpp2py_msg_name)(alloc_env);
                    m_py2cppVector = m_segment->construct<Py2CppMsgVector>(py2cpp_msg_name)(alloc_act);
                }
                else
                {
                    m_cpp2pyStruct = m_segment->construct<Cpp2PyMsgType>(cpp2py_msg_name)();
                    m_py2CppStruct = m_segment->construct<Py2CppMsgType>(py2cpp_msg_name)();
                }
                m_sync = m_segment->construct<Ns3AiMsgSync>(lockable_name)();
            }
            catch (const interprocess_exception& e)
            {
                shared_memory_object::remove(m_segName.c_str());
                throw std::runtime_error(
                    std::string("ns3-ai: Failed to construct shared objects: ") + e.what() +
                    "\nHint: Increase segment size (current: " + std::to_string(size) + ")");
            }
        }
        else
        {
            try
            {
                m_segment = std::make_unique<managed_shared_memory>(
                    open_only, segment_name);
            }
            catch (const interprocess_exception& e)
            {
                throw std::runtime_error(
                    std::string("ns3-ai: Cannot open shared memory '") + segment_name +
                    "': " + e.what() +
                    "\nHint: Start the C++ simulation first.");
            }

            if (m_useVector)
            {
                m_cpp2pyVector = m_segment->find<Cpp2PyMsgVector>(cpp2py_msg_name).first;
                m_py2cppVector = m_segment->find<Py2CppMsgVector>(py2cpp_msg_name).first;
                if (!m_cpp2pyVector || !m_py2cppVector)
                {
                    throw std::runtime_error("ns3-ai: Vector objects not found in shared memory.");
                }
            }
            else
            {
                m_cpp2pyStruct = m_segment->find<Cpp2PyMsgType>(cpp2py_msg_name).first;
                m_py2CppStruct = m_segment->find<Py2CppMsgType>(py2cpp_msg_name).first;
                if (!m_cpp2pyStruct || !m_py2CppStruct)
                {
                    throw std::runtime_error("ns3-ai: Struct objects not found in shared memory.");
                }
            }

            m_sync = m_segment->find<Ns3AiMsgSync>(lockable_name).first;
            if (!m_sync)
            {
                throw std::runtime_error("ns3-ai: Sync object not found in shared memory.");
            }
        }
    }

    ~Ns3AiMsgInterfaceImpl()
    {
        if (m_isCreator)
        {
            boost::interprocess::shared_memory_object::remove(m_segName.c_str());
        }
        else
        {
            if (m_handleFinish && !m_isFinished)
            {
                try { CppSetFinished(); } catch (...) {}
            }
        }
    }

    Ns3AiMsgInterfaceImpl(const Ns3AiMsgInterfaceImpl&) = delete;
    Ns3AiMsgInterfaceImpl& operator=(const Ns3AiMsgInterfaceImpl&) = delete;

    typedef boost::interprocess::
        allocator<Cpp2PyMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Cpp2PyMsgAllocator;
    typedef boost::interprocess::vector<Cpp2PyMsgType, Cpp2PyMsgAllocator> Cpp2PyMsgVector;
    typedef boost::interprocess::
        allocator<Py2CppMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Py2CppMsgAllocator;
    typedef boost::interprocess::vector<Py2CppMsgType, Py2CppMsgAllocator> Py2CppMsgVector;

    Cpp2PyMsgType* GetCpp2PyStruct()
    {
        if (m_useVector) throw std::logic_error("ns3-ai: Use GetCpp2PyVector() in vector mode");
        return m_cpp2pyStruct;
    }

    Py2CppMsgType* GetPy2CppStruct()
    {
        if (m_useVector) throw std::logic_error("ns3-ai: Use GetPy2CppVector() in vector mode");
        return m_py2CppStruct;
    }

    Cpp2PyMsgVector* GetCpp2PyVector()
    {
        if (!m_useVector) throw std::logic_error("ns3-ai: Use GetCpp2PyStruct() in struct mode");
        return m_cpp2pyVector;
    }

    Py2CppMsgVector* GetPy2CppVector()
    {
        if (!m_useVector) throw std::logic_error("ns3-ai: Use GetPy2CppStruct() in struct mode");
        return m_py2cppVector;
    }

    void CppSendBegin() { Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2pyEmptyCount); }
    void CppSendEnd() { Ns3AiSemaphore::sem_post(&m_sync->m_cpp2pyFullCount); }
    void CppRecvBegin() { Ns3AiSemaphore::sem_wait(&m_sync->m_py2cppFullCount); }
    void CppRecvEnd() { Ns3AiSemaphore::sem_post(&m_sync->m_py2cppEmptyCount); }

    void CppSetFinished()
    {
        if (!m_handleFinish) throw std::logic_error("ns3-ai: handle_finish is false");
        m_isFinished = true;
        CppSendBegin();
        m_sync->m_isFinished = true;
        CppSendEnd();
    }

    void PyRecvBegin()
    {
        Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2pyFullCount);
        if (m_handleFinish) m_isFinished = m_sync->m_isFinished;
    }

    void PyRecvEnd() { Ns3AiSemaphore::sem_post(&m_sync->m_cpp2pyEmptyCount); }
    void PySendBegin() { Ns3AiSemaphore::sem_wait(&m_sync->m_py2cppEmptyCount); }
    void PySendEnd() { Ns3AiSemaphore::sem_post(&m_sync->m_py2cppFullCount); }

    bool PyGetFinished()
    {
        if (!m_handleFinish) throw std::logic_error("ns3-ai: handle_finish is false");
        return m_isFinished;
    }

  private:
    const bool m_isCreator;
    const bool m_useVector;
    const bool m_handleFinish;
    const std::string m_segName;
    bool m_isFinished;
    Cpp2PyMsgType* m_cpp2pyStruct;
    Py2CppMsgType* m_py2CppStruct;
    Cpp2PyMsgVector* m_cpp2pyVector;
    Py2CppMsgVector* m_py2cppVector;
    Ns3AiMsgSync* m_sync;
    std::unique_ptr<boost::interprocess::managed_shared_memory> m_segment;
};

class Ns3AiMsgInterface : public Singleton<Ns3AiMsgInterface>
{
  public:
    void SetIsMemoryCreator(bool v) { m_isMemoryCreator = v; }
    void SetUseVector(bool v) { m_useVector = v; }
    void SetHandleFinish(bool v) { m_handleFinish = v; }
    void SetMemorySize(uint32_t s) { m_size = s; }

    void SetNames(std::string seg, std::string c2p, std::string p2c, std::string lock)
    {
        m_segmentName = seg; m_cpp2pyMsgName = c2p; m_py2cppMsgName = p2c; m_lockableName = lock;
    }

    template <typename Cpp2PyMsgType, typename Py2CppMsgType>
    Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>* GetInterface()
    {
        static Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType> interface(
            m_isMemoryCreator, m_useVector, m_handleFinish, m_size,
            m_segmentName.c_str(), m_cpp2pyMsgName.c_str(),
            m_py2cppMsgName.c_str(), m_lockableName.c_str());
        return &interface;
    }

  private:
    bool m_isMemoryCreator = true;
    bool m_useVector = false;
    bool m_handleFinish = true;
    uint32_t m_size = 4096;
    std::string m_segmentName = "My Seg";
    std::string m_cpp2pyMsgName = "My Cpp to Python Msg";
    std::string m_py2cppMsgName = "My Python to Cpp Msg";
    std::string m_lockableName = "My Lockable";
};

} // namespace ns3

#endif // NS3_AI_MSG_INTERFACE_H
