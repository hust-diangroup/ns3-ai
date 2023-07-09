#ifndef APB_H
#define APB_H

struct EnvStruct
{
    uint32_t env_a;
    uint32_t env_b;

    EnvStruct()
        : env_a(0),
          env_b(0)
    {
    }
};

struct ActStruct
{
    uint32_t act_c;

    ActStruct()
        : act_c(0)
    {
    }
};

#endif // APB_H
