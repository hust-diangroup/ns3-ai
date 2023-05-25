#ifndef SH_MEM_H
#define SH_MEM_H

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

struct RlShMemLockable
{

    explicit RlShMemLockable()
        : env_in(false), act_in(false), m_isFinished(false)
    {}

    //Mutex to protect access to the vector
    boost::interprocess::interprocess_mutex      env_mutex;
    boost::interprocess::interprocess_mutex      act_mutex;

    //Conditions to wait
    boost::interprocess::interprocess_condition  cond_env_avail_for_python;
    boost::interprocess::interprocess_condition  cond_env_empty_for_cpp;
    boost::interprocess::interprocess_condition  cond_act_avail_for_cpp;
    boost::interprocess::interprocess_condition  cond_act_empty_for_python;

    //Is there any message
    bool env_in;
    bool act_in;

    //Is communication finished
    bool m_isFinished;
};

#endif // SH_MEM_H
