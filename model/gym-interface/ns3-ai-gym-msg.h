#ifndef NS3_NS3_AI_GYM_MSG_H
#define NS3_NS3_AI_GYM_MSG_H

#include <stdint.h>

#define MSG_BUFFER_SIZE 1024

struct Ns3AiGymMsg
{
    uint8_t buffer[MSG_BUFFER_SIZE];
    uint32_t size;
};

#endif // NS3_NS3_AI_GYM_MSG_H
