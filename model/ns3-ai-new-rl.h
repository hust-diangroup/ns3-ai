#ifndef NS3_AI_RL_H
#define NS3_AI_RL_H

#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
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
                     bool is_memory_creator = false, // true for Python side
                     const char *segment_name = "My Seg",
                     const char *env_name = "My Env",
                     const char *act_name = "My Act",
                     const char *lockable_name = "My Lockable");
    ~NS3AIRL();

    // for C++ side:
    void set_env(std::vector<EnvType> &env);
    void get_act(std::vector<ActType> &act);

    // for Python side:
    void get_env(std::vector<EnvType> &env);
    void set_act(std::vector<ActType> &act);
    bool is_finished();

  private:
    typedef boost::interprocess::allocator<EnvType, boost::interprocess::managed_shared_memory::segment_manager>  ShmemEnvAllocator;
    typedef boost::interprocess::vector<EnvType, ShmemEnvAllocator> MyEnvVector;
    typedef boost::interprocess::allocator<ActType, boost::interprocess::managed_shared_memory::segment_manager>  ShmemActAllocator;
    typedef boost::interprocess::vector<ActType, ShmemActAllocator> MyActVector;
    void set_finished();    // for C++ side
    bool m_isCreator;
    bool m_isFinished;
    std::string m_segName;
    MyEnvVector *m_env;
    MyActVector *m_act;
    RlShMemLockable *m_lockable;
};

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::set_env(std::vector<EnvType> &env) {
    using namespace boost::interprocess;
    scoped_lock<interprocess_mutex> env_lock(m_lockable->env_mutex);
    if (m_lockable->env_in) {
        m_lockable->cond_env_empty_for_cpp.wait(env_lock);
    }

    for (EnvType i : env) {
        m_env->push_back(i);
    }

    //Notify to Python that a env is produced
    m_lockable->cond_env_avail_for_python.notify_one();
    m_lockable->env_in = true;
}

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::get_act(std::vector<ActType> &act) {
    using namespace boost::interprocess;
    scoped_lock<interprocess_mutex> act_lock(m_lockable->act_mutex);
    if (!m_lockable->act_in) {
        m_lockable->cond_act_avail_for_cpp.wait(act_lock);
    }

    act.clear();
    for (ActType j : *m_act) {
        act.push_back(j);
    }
    m_act->clear();

    //Notify to Python that an act is consumed
    m_lockable->cond_act_empty_for_python.notify_one();
    m_lockable->act_in = false;
}

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::set_finished() {
    using namespace boost::interprocess;
    scoped_lock<interprocess_mutex> env_lock(m_lockable->env_mutex);
    if (m_lockable->env_in) {
        m_lockable->cond_env_empty_for_cpp.wait(env_lock);
    }

    m_lockable->m_isFinished = true;

    //Notify to Python that a env is produced
    m_lockable->cond_env_avail_for_python.notify_one();
    m_lockable->env_in = true;
}

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::get_env(std::vector<EnvType> &env) {
    using namespace boost::interprocess;
    scoped_lock<interprocess_mutex> env_lock(m_lockable->env_mutex);
    if (!m_lockable->env_in){
        m_lockable->cond_env_avail_for_python.wait(env_lock);
    }

    env.clear();
    for (EnvType i : *m_env) {
        env.push_back(i);
    }
    m_env->clear();

    // get whether C++ side finished
    m_isFinished =  m_lockable->m_isFinished;

    //Notify to C++ that a env is consumed
    m_lockable->cond_env_empty_for_cpp.notify_one();
    m_lockable->env_in = false;
}

template<typename EnvType, typename ActType>
void NS3AIRL<EnvType, ActType>::set_act(std::vector<ActType> &act) {
    using namespace boost::interprocess;
    scoped_lock<interprocess_mutex> act_lock(m_lockable->act_mutex);
    if (m_lockable->act_in){
        m_lockable->cond_act_empty_for_python.wait(act_lock);
    }

    for (ActType i : act) {
        m_act->push_back(i);
    }

    //Notify to C++ that an action is produced
    m_lockable->cond_act_avail_for_cpp.notify_one();
    m_lockable->act_in = true;
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
                                   bool is_memory_creator,
                                   const char *segment_name,
                                   const char *env_name,
                                   const char *act_name,
                                   const char *lockable_name)
    : m_isCreator(is_memory_creator), m_isFinished(false), m_segName(segment_name)
{
    using namespace boost::interprocess;
    if (m_isCreator)
    {
        shared_memory_object::remove(m_segName.c_str());
        static managed_shared_memory segment(create_only, m_segName.c_str(), size);
        static const ShmemEnvAllocator alloc_env(segment.get_segment_manager());
        static const ShmemEnvAllocator alloc_act(segment.get_segment_manager());
        m_env = segment.construct<MyEnvVector>(env_name)(alloc_env);
        m_act = segment.construct<MyActVector>(act_name)(alloc_act);
        m_lockable = segment.construct<RlShMemLockable>(lockable_name)();
    }
    else
    {
        static managed_shared_memory segment(open_only, segment_name);
        m_env = segment.find<MyEnvVector>(env_name).first;
        m_act = segment.find<MyActVector>(act_name).first;
        m_lockable = segment.find<RlShMemLockable>(lockable_name).first;
    }
}

}

#endif // NS3_AI_RL_H
