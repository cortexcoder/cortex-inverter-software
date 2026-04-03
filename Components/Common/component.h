#pragma once

#include <stdint.h>
#include <stdbool.h>

#define COMPONENT_INIT_OK   0
#define COMPONENT_INIT_ERR -1

#define COMPONENT_RUN_OK   0
#define COMPONENT_RUN_ERR  -1

typedef int32_t Component_Status;

typedef struct {
    void *instance;
    Component_Status (*init)(void *instance);
    Component_Status (*run)(void *instance);
    void (*reset)(void *instance);
} Component_Interface;

#define COMPONENT_INTERFACE_DEFINE(type) \
    static Component_Status type##_Init(void *instance); \
    static Component_Status type##_Run(void *instance); \
    static void type##_Reset(void *instance); \
    static const Component_Interface type##_Interface = { \
        .instance = instance, \
        .init = type##_Init, \
        .run = type##_Run, \
        .reset = type##_Reset \
    }

#define COMPONENT_HANDLE(type) type##_Handle_t

#define COMPONENT_CONFIG(type) type##_Config_t

#define COMPONENT_DECLARE(type) \
    typedef struct type##_Config_s type##_Config_t; \
    typedef struct type##_Handle_s type##_Handle_t; \
    struct type##_Config_s; \
    struct type##_Handle_s

#define COMPONENT_DEFINE(type) \
    struct type##_Config_s

#define COMPONENT_HANDLE_DEF(type) \
    struct type##_Handle_s

#define PI_CTRL_CONFIG(type) type##_Config_t
#define PI_CTRL_HANDLE(type) type##_Handle_t

#define SVPWM_CONFIG(type) type##_Config_t
#define SVPWM_HANDLE(type) type##_Handle_t

#define LPF_CONFIG(type) type##_Config_t
#define LPF_HANDLE(type) type##_Handle_t
