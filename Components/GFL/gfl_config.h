#pragma once

#include "gfl_types.h"
#include "gfl_limits.h"
#include "gfl_dc_bus.h"
#include "gfl_ridethrough.h"
#include "gfl_weak_grid.h"
#include "gfl_loop.h"

/**
 * @file gfl_config.h
 * @brief GFL 配置层 - 模块初始化和配置
 */

/**
 * @brief GFL 完整实例
 * 
 * 包含所有子模块的配置和状态
 * 用户应将此结构体作为不透明句柄使用
 */
typedef struct {
    /* 主配置 */
    Gfl_Config config;
    
    /* 内部实现 - 隐藏细节以保持 ABI 兼容 */
    /* 预留 1024 字节用于内部状态，避免动态分配 */
    unsigned char loop_data[1024];
    
} Gfl_Instance;

/**
 * @brief 获取默认配置
 * 
 * @return Gfl_Config 默认配置参数
 */
Gfl_Config Gfl_GetDefaultConfig(void);

/**
 * @brief 初始化 GFL 实例
 * 
 * @param inst 实例指针
 */
void Gfl_InitInstance(Gfl_Instance *inst);

/**
 * @brief 配置层初始化
 * 
 * 在系统初始化时调用，配置所有模块参数
 * 
 * @param inst 实例指针
 */
void Gfl_Config_Init(Gfl_Instance *inst);

/**
 * @brief 启动 GFL
 * 
 * @param inst 实例指针
 */
void Gfl_Start(Gfl_Instance *inst);

/**
 * @brief 停止 GFL
 * 
 * @param inst 实例指针
 */
void Gfl_Stop(Gfl_Instance *inst);

/**
 * @brief GFL 主函数
 * 
 * 在控制任务中调用
 * 
 * @param inst 实例指针
 * @param v_alpha α轴电压
 * @param v_beta β轴电压
 * @param Vdc_bus 母线电压 (V)
 * @param P_ref 有功功率参考 (pu)
 * @param Q_ref 无功功率参考 (pu)
 * @param output 输出
 */
void Gfl_Step(Gfl_Instance *inst,
              float v_alpha, float v_beta, float Vdc_bus,
              float P_ref, float Q_ref,
              GflLoop_Output *output);

/**
 * @brief 获取当前工作模式
 * 
 * @param inst 实例指针
 * @return Gfl_Mode 当前模式
 */
Gfl_Mode Gfl_GetMode(Gfl_Instance *inst);

/**
 * @brief 获取故障状态
 * 
 * @param inst 实例指针
 * @return Gfl_FaultType 当前故障
 */
Gfl_FaultType Gfl_GetFault(Gfl_Instance *inst);

/**
 * @brief 清除故障
 * 
 * @param inst 实例指针
 */
void Gfl_ClearFault(Gfl_Instance *inst);

/*===========================================
 * 宏定义 - 方便其他模块调用
 *===========================================*/

/**
 * @brief 获取当前工作模式
 */
#define GFL_GET_MODE(inst)              Gfl_GetMode(inst)

/**
 * @brief 获取当前故障
 */
#define GFL_GET_FAULT(inst)             Gfl_GetFault(inst)

/**
 * @brief 获取高低穿状态
 */
#define GFL_GET_RT_STATE(inst)          GflLoop_GetRideThroughState((inst)->loop_data)

/**
 * @brief 检查电网是否就绪
 */
#define GFL_IS_GRID_READY(inst)         (Gfl_GetMode(inst) == GFL_MODE_RUNNING)

/**
 * @brief 检查是否有故障
 */
#define GFL_HAS_FAULT(inst)             (Gfl_GetFault(inst) != GFL_FAULT_NONE)

/**
 * @brief 获取母线状态
 */
#define GFL_GET_DC_BUS_STATE(inst, state) \
    GflLoop_GetDcBusState((inst)->loop_data, state)

/**
 * @brief 获取弱网状态
 */
#define GFL_GET_WEAK_GRID_STATE(inst, output) \
    GflLoop_GetWeakGridState((inst)->loop_data, output)

/**
 * @brief 获取 Id_ref (有功电流参考)
 */
#define GFL_GET_ID_REF(inst)            (((Gfl_Handle *)(inst)->loop_data)->current_ref_limited.Id_ref)

/**
 * @brief 获取 Iq_ref (无功电流参考)
 */
#define GFL_GET_IQ_REF(inst)            (((Gfl_Handle *)(inst)->loop_data)->current_ref_limited.Iq_ref)

/**
 * @brief 获取 P_ref (有功功率参考)
 */
#define GFL_GET_P_REF(inst)             (((Gfl_Handle *)(inst)->loop_data)->power_ref.P_ref)

/**
 * @brief 获取 Q_ref (无功功率参考)
 */
#define GFL_GET_Q_REF(inst)             (((Gfl_Handle *)(inst)->loop_data)->power_ref.Q_ref)

/**
 * @brief 获取锁相角度
 */
#define GFL_GET_THETA(inst)             (((Gfl_Handle *)(inst)->loop_data)->theta_pos)

/**
 * @brief 获取电网频率 (Hz)
 * @note 返回实际频率 = 额定频率(50Hz) + 频率偏差
 */
#define GFL_GET_FREQ(inst)              (50.0f + ((Gfl_Handle *)(inst)->loop_data)->weak_grid.freq_offset)

/**
 * @brief 获取电网电压幅值
 */
#define GFL_GET_GRID_AMPLITUDE(inst)    (((Gfl_Handle *)(inst)->loop_data)->voltage.grid_voltage)

/**
 * @brief 检查是否处于 LVRT 状态
 */
#define GFL_IS_LVRT(inst)                (GflLoop_GetRideThroughState((inst)->loop_data) == GFL_RT_LVRT)

/**
 * @brief 检查是否处于 HVRT 状态
 */
#define GFL_IS_HVRT(inst)                (GflLoop_GetRideThroughState((inst)->loop_data) == GFL_RT_HVRT)

/**
 * @brief 检查是否处于弱网状态
 */
#define GFL_IS_WEAK_GRID(inst)           (((Gfl_Handle *)(inst)->loop_data)->weak_grid.Z_grid > \
                                           (inst)->config.weak_grid_thresh)

/*===========================================
 * 电网类型和电流模式宏定义
 *===========================================*/

/**
 * @brief 检查电网类型
 */
#define GFL_GRID_IS_ABC3(inst)          ((inst)->config.grid_type == GRID_TYPE_ABC3)
#define GFL_GRID_IS_ABCN4(inst)         ((inst)->config.grid_type == GRID_TYPE_ABCN4)

/**
 * @brief 检查电流模式
 */
#define GFL_CURRENT_IS_SYMMETRIC(inst)   ((inst)->config.current_mode == CURRENT_MODE_SYMMETRIC)
#define GFL_CURRENT_IS_ASYMMETRIC(inst)  ((inst)->config.current_mode == CURRENT_MODE_ASYMMETRIC)

/*===========================================
 * 分相功率控制函数
 *===========================================*/

/**
 * @brief 分相功率转电流
 * 
 * @param power 相功率数组 [P, Q, V] per phase
 * @param num_phases 相数
 * @param mode 电流模式 (对称/不对称)
 * @param output 分相电流输出
 */
void Gfl_SplitPhase_CalcCurrent(const Gfl_PhasePower *power,
                                  uint8_t num_phases,
                                  Gfl_CurrentMode mode,
                                  Gfl_SplitCurrentRef *output);

/**
 * @brief 电流指令竞争仲裁
 * 
 * @param requests 请求数组
 * @param num_requests 请求数量
 * @param limits 电流限幅
 * @param output 仲裁结果
 */
void Gfl_CurrentArbitrate(const Gfl_CurrentRequest *requests,
                          uint8_t num_requests,
                          const Gfl_CurrentLimits *limits,
                          Gfl_SplitCurrentRef *output);

/**
 * @brief 电流限幅 (圆形+矩形)
 * 
 * @param input 输入电流
 * @param limits 限幅参数
 * @param mode 电流模式
 * @param output 限幅后电流
 */
void Gfl_CurrentLimit(const Gfl_SplitCurrentRef *input,
                      const Gfl_CurrentLimits *limits,
                      Gfl_CurrentMode mode,
                      Gfl_SplitCurrentRef *output);

/**
 * @brief 获取分相电流参考 (用于快速环)
 * 
 * @param inst 实例
 * @param phase_idx 相索引 (0=A, 1=B, 2=C, 3=N...)
 * @return float Id_ref
 */
#define GFL_GET_ID_REF_PHASE(inst, phase_idx) \
    (((Gfl_Handle *)(inst)->loop_data)->split_current.Id[(phase_idx)])

/**
 * @brief 获取分相无功电流参考
 * 
 * @param inst 实例
 * @param phase_idx 相索引
 * @return float Iq_ref
 */
#define GFL_GET_IQ_REF_PHASE(inst, phase_idx) \
    (((Gfl_Handle *)(inst)->loop_data)->split_current.Iq[(phase_idx)])

/**
 * @brief 获取相数
 */
#define GFL_GET_NUM_PHASES(inst) \
    (((Gfl_Handle *)(inst)->loop_data)->split_current.num_phases)