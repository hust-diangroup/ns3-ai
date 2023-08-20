#ifndef NS3_AI_MSG_INTERFACE_H
#define NS3_AI_MSG_INTERFACE_H

#include "ns3-ai-semaphore.h"
#include "ns3/singleton.h"
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ns3
{

struct Ns3AiMsgSync
{
    volatile uint8_t m_cpp2py_empty_count{1};
    volatile uint8_t m_cpp2py_full_count{0};
    volatile uint8_t m_py2cpp_empty_count{1};
    volatile uint8_t m_py2cpp_full_count{0};
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
                               const char* lockable_name = "My Lockable");
    ~Ns3AiMsgInterfaceImpl();

    // for C++ side:
    void cpp_send_begin();
    void cpp_send_end();
    void cpp_recv_begin();
    void cpp_recv_end();
    void cpp_set_finished();

    // for Python side:
    void py_recv_begin();
    void py_recv_end();
    void py_send_begin();
    void py_send_end();
    bool py_get_finished();

    // use structure for the simple case
    Cpp2PyMsgType* m_single_cpp2py_msg;
    Py2CppMsgType* m_single_py2cpp_msg;

    // use vector for passing multiple structures at once
    typedef boost::interprocess::
        allocator<Cpp2PyMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Cpp2PyMsgAllocator;
    typedef boost::interprocess::vector<Cpp2PyMsgType, Cpp2PyMsgAllocator> Cpp2PyMsgVector;
    typedef boost::interprocess::
        allocator<Py2CppMsgType, boost::interprocess::managed_shared_memory::segment_manager>
            Py2CppMsgAllocator;
    typedef boost::interprocess::vector<Py2CppMsgType, Py2CppMsgAllocator> Py2CppMsgVector;
    Cpp2PyMsgVector* m_cpp2py_msg;
    Py2CppMsgVector* m_py2cpp_msg;

  private:
    Ns3AiMsgSync* m_sync;
    const bool m_isCreator;
    const bool m_handleFinish;
    const std::string m_segName;
    bool m_isFinished;
};

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::cpp_send_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2py_empty_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::cpp_send_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_cpp2py_full_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::cpp_recv_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_py2cpp_full_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::cpp_recv_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_py2cpp_empty_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::cpp_set_finished()
{
    assert(m_handleFinish);
    m_isFinished = true;
    cpp_send_begin();
    m_sync->m_isFinished = true;
    cpp_send_end();
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::py_recv_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_cpp2py_full_count);
    if (m_handleFinish)
    {
        m_isFinished = m_sync->m_isFinished;
    }
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::py_recv_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_cpp2py_empty_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::py_send_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_py2cpp_empty_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
void
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::py_send_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_py2cpp_full_count);
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
bool
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::py_get_finished()
{
    assert(m_handleFinish);
    return m_isFinished;
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::~Ns3AiMsgInterfaceImpl()
{
    if (m_isCreator)
    {
        boost::interprocess::shared_memory_object::remove(m_segName.c_str());
    }
    else
    {
        if (m_handleFinish)
        {
            cpp_set_finished();
        }
    }
}

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType>::Ns3AiMsgInterfaceImpl(bool is_memory_creator,
                                                                   bool use_vector,
                                                                   bool handle_finish,
                                                                   uint32_t size,
                                                                   const char* segment_name,
                                                                   const char* cpp2py_msg_name,
                                                                   const char* py2cpp_msg_name,
                                                                   const char* lockable_name)
    : m_isCreator(is_memory_creator),
      m_handleFinish(handle_finish),
      m_segName(segment_name),
      m_isFinished(false)
{
    using namespace boost::interprocess;
    if (m_isCreator)
    {
        shared_memory_object::remove(m_segName.c_str());
        static managed_shared_memory segment(create_only, m_segName.c_str(), size);
        if (use_vector)
        {
            static const Cpp2PyMsgAllocator alloc_env(segment.get_segment_manager());
            static const Cpp2PyMsgAllocator alloc_act(segment.get_segment_manager());
            m_cpp2py_msg = segment.construct<Cpp2PyMsgVector>(cpp2py_msg_name)(alloc_env);
            m_py2cpp_msg = segment.construct<Py2CppMsgVector>(py2cpp_msg_name)(alloc_act);
            m_single_cpp2py_msg = nullptr;
            m_single_py2cpp_msg = nullptr;
        }
        else
        {
            m_cpp2py_msg = nullptr;
            m_py2cpp_msg = nullptr;
            m_single_cpp2py_msg = segment.construct<Cpp2PyMsgType>(cpp2py_msg_name)();
            m_single_py2cpp_msg = segment.construct<Py2CppMsgType>(py2cpp_msg_name)();
        }
        m_sync = segment.construct<Ns3AiMsgSync>(lockable_name)();
    }
    else
    {
        static managed_shared_memory segment(open_only, segment_name);
        if (use_vector)
        {
            m_cpp2py_msg = segment.find<Cpp2PyMsgVector>(cpp2py_msg_name).first;
            m_py2cpp_msg = segment.find<Py2CppMsgVector>(py2cpp_msg_name).first;
            m_single_cpp2py_msg = nullptr;
            m_single_py2cpp_msg = nullptr;
        }
        else
        {
            m_cpp2py_msg = nullptr;
            m_py2cpp_msg = nullptr;
            m_single_cpp2py_msg = segment.find<Cpp2PyMsgType>(cpp2py_msg_name).first;
            m_single_py2cpp_msg = segment.find<Py2CppMsgType>(py2cpp_msg_name).first;
        }
        m_sync = segment.find<Ns3AiMsgSync>(lockable_name).first;
    }
}

class Ns3AiMsgInterface : public Singleton<Ns3AiMsgInterface>
{
  public:
    void SetIsMemoryCreator(bool isMemoryCreator);
    void SetUseVector(bool useVector);
    void SetHandleFinish(bool handleFinish);
    void SetMemorySize(uint32_t size);
    void SetNames(std::string segmentName,
                  std::string cpp2pyMsgName,
                  std::string py2cppMsgName,
                  std::string lockableName);
    template<typename Cpp2PyMsgType, typename Py2CppMsgType>
    Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType> *GetInterface();

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

template <typename Cpp2PyMsgType, typename Py2CppMsgType>
Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType> *
Ns3AiMsgInterface::GetInterface()
{
    static Ns3AiMsgInterfaceImpl<Cpp2PyMsgType, Py2CppMsgType> interface(
        this->m_isMemoryCreator,
        this->m_useVector,
        this->m_handleFinish,
        this->m_size,
        this->m_segmentName.c_str(),
        this->m_cpp2pyMsgName.c_str(),
        this->m_py2cppMsgName.c_str(),
        this->m_lockableName.c_str()
    );
    return &interface;
}

} // namespace ns3

#endif // NS3_AI_MSG_INTERFACE_H
