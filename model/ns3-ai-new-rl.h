#ifndef NS3_AI_RL_H
#define NS3_AI_RL_H

#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include "sh_mem.h"

namespace ns3
{

template <typename EnvType, typename ActType>
class NS3AIRL
{
  public:
    NS3AIRL() = delete;
    explicit NS3AIRL(uint32_t size,
                     bool use_vector = true,
                     bool is_memory_creator = false,
                     const char* segment_name = "My Seg",
                     const char* env_name = "My Env",
                     const char* act_name = "My Act",
                     const char* lockable_name = "My Lockable");
    ~NS3AIRL();

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
    RlShMemLockable *m_lockable;
    bool m_isCreator;
    bool m_isFinished;
    std::string m_segName;
};

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::set_env_begin()
{
    RlShMemLockable::sem_wait(&m_lockable->m_empty_env_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::set_env_end()
{
    RlShMemLockable::sem_post(&m_lockable->m_full_env_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::get_act_begin()
{
    RlShMemLockable::sem_wait(&m_lockable->m_full_act_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::get_act_end()
{
    RlShMemLockable::sem_post(&m_lockable->m_empty_act_count);
}

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::set_finished() {
    RlShMemLockable::sem_wait(&m_lockable->m_empty_env_count);
    m_lockable->m_isFinished = true;
    RlShMemLockable::sem_post(&m_lockable->m_full_env_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::get_env_begin()
{
    RlShMemLockable::sem_wait(&m_lockable->m_full_env_count);
    m_isFinished = m_lockable->m_isFinished;
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::get_env_end()
{
    RlShMemLockable::sem_post(&m_lockable->m_empty_env_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::set_act_begin()
{
    RlShMemLockable::sem_wait(&m_lockable->m_empty_act_count);
}

template <typename EnvType, typename ActType>
void
NS3AIRL<EnvType, ActType>::set_act_end()
{
    RlShMemLockable::sem_post(&m_lockable->m_full_act_count);
}

template<typename EnvType, typename ActType>
bool NS3AIRL<EnvType, ActType>::is_finished() {
    return m_isFinished;
}

template<typename EnvType, typename ActType>
NS3AIRL<EnvType, ActType>::~NS3AIRL() {
    if (m_isCreator)
    {
        boost::interprocess::shared_memory_object::remove(m_segName.c_str());
    }
    else {
        set_finished();
    }
}

template <typename EnvType, typename ActType>
NS3AIRL<EnvType, ActType>::NS3AIRL(uint32_t size,
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
        m_lockable = segment.construct<RlShMemLockable>(lockable_name)();
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
        m_lockable = segment.find<RlShMemLockable>(lockable_name).first;
    }
}

}

#endif // NS3_AI_RL_H
