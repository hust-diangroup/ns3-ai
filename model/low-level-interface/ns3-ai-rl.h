#ifndef NS3_AI_RL_H
#define NS3_AI_RL_H

#include "../ns3-ai-semaphore.h"

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

struct Ns3AiRlSync {
    volatile uint8_t m_empty_env_count{1};
    volatile uint8_t m_full_env_count{0};
    volatile uint8_t m_empty_act_count{1};
    volatile uint8_t m_full_act_count{0};
    bool m_isFinished{false};
};

template <typename EnvType, typename ActType>
class Ns3AiRl
{
  public:
    Ns3AiRl() = delete;
    explicit Ns3AiRl(uint32_t size,
                     bool use_vector = true,
                     bool is_memory_creator = false,
                     const char* segment_name = "My Seg",
                     const char* env_name = "My Env",
                     const char* act_name = "My Act",
                     const char* lockable_name = "My Lockable");
    ~Ns3AiRl();

    // for C++ side:
    void set_env_begin();
    void set_env_end();
    void get_act_begin();
    void get_act_end();

    // for Python side:
    void get_env_begin();
    void get_env_end();
    void set_act_begin();
    void set_act_end();
    bool is_finished();

    // use structure for the simple case
    EnvType *m_single_env;
    ActType *m_single_act;

    // use vector for passing multiple structures at once
    typedef boost::interprocess::allocator<EnvType, boost::interprocess::managed_shared_memory::segment_manager>  ShmemEnvAllocator;
    typedef boost::interprocess::vector<EnvType, ShmemEnvAllocator> ShmemEnvVector;
    typedef boost::interprocess::allocator<ActType, boost::interprocess::managed_shared_memory::segment_manager>  ShmemActAllocator;
    typedef boost::interprocess::vector<ActType, ShmemActAllocator> ShmemActVector;
    ShmemEnvVector *m_env;
    ShmemActVector *m_act;

  private:
    void set_finished();    // for C++ side when exiting
    Ns3AiRlSync *m_sync;
    bool m_isCreator;
    bool m_isFinished;
    std::string m_segName;
};

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::set_env_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_empty_env_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::set_env_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_full_env_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::get_act_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_full_act_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::get_act_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_empty_act_count);
}

template<typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::set_finished() {
    Ns3AiSemaphore::sem_wait(&m_sync->m_empty_env_count);
    m_sync->m_isFinished = true;
    Ns3AiSemaphore::sem_post(&m_sync->m_full_env_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::get_env_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_full_env_count);
    m_isFinished = m_sync->m_isFinished;
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::get_env_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_empty_env_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::set_act_begin()
{
    Ns3AiSemaphore::sem_wait(&m_sync->m_empty_act_count);
}

template <typename EnvType, typename ActType>
void
Ns3AiRl<EnvType, ActType>::set_act_end()
{
    Ns3AiSemaphore::sem_post(&m_sync->m_full_act_count);
}

template<typename EnvType, typename ActType>
bool
Ns3AiRl<EnvType, ActType>::is_finished() {
    return m_isFinished;
}

template<typename EnvType, typename ActType>
Ns3AiRl<EnvType, ActType>::~Ns3AiRl() {
    if (m_isCreator)
    {
        boost::interprocess::shared_memory_object::remove(m_segName.c_str());
    }
    else {
        set_finished();
    }
}

template <typename EnvType, typename ActType>
Ns3AiRl<EnvType, ActType>::Ns3AiRl(uint32_t size,
                                   bool use_vector,
                                   bool is_memory_creator,
                                   const char* segment_name,
                                   const char* env_name,
                                   const char* act_name,
                                   const char* lockable_name)
    : m_isCreator(is_memory_creator), m_isFinished(false), m_segName(segment_name)
{
    using namespace boost::interprocess;
    if (m_isCreator)
    {
        shared_memory_object::remove(m_segName.c_str());
        static managed_shared_memory segment(create_only, m_segName.c_str(), size);
        if (use_vector) {
            static const ShmemEnvAllocator alloc_env(segment.get_segment_manager());
            static const ShmemEnvAllocator alloc_act(segment.get_segment_manager());
            m_env = segment.construct<ShmemEnvVector>(env_name)(alloc_env);
            m_act = segment.construct<ShmemActVector>(act_name)(alloc_act);
            m_single_env = nullptr;
            m_single_act = nullptr;
        }
        else {
            m_env = nullptr;
            m_act = nullptr;
            m_single_env = segment.construct<EnvType>(env_name)();
            m_single_act = segment.construct<ActType>(act_name)();
        }
        m_sync = segment.construct<Ns3AiRlSync>(lockable_name)();
    }
    else
    {
        static managed_shared_memory segment(open_only, segment_name);
        if (use_vector) {
            m_env = segment.find<ShmemEnvVector>(env_name).first;
            m_act = segment.find<ShmemActVector>(act_name).first;
            m_single_env = nullptr;
            m_single_act = nullptr;
        }
        else {
            m_env = nullptr;
            m_act = nullptr;
            m_single_env = segment.find<EnvType>(env_name).first;
            m_single_act = segment.find<ActType>(act_name).first;
        }
        m_sync = segment.find<Ns3AiRlSync>(lockable_name).first;
    }
}

}

#endif // NS3_AI_RL_H
