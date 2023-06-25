#ifndef APB_H
#define APB_H

typedef struct _EnvStruct
{
    uint32_t env_a;
    uint32_t env_b;

    _EnvStruct()
        : env_a(0),
          env_b(0)
    {
    }
} EnvStruct;

typedef struct _ActStruct
{
    uint32_t act_c;

    _ActStruct()
        : act_c(0)
    {
    }
} ActStruct;

#endif // APB_H
