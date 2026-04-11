#include "gfl_weak_grid.h"
#include <math.h>

/* 弱网检测内部状态 */
struct GflWeakGrid_Internal {
    float threshold;             /* 弱网判定阈值 */
    float Z_grid_est;           /* 电网阻抗估计 */
    
    /* 阻抗估计状态 */
    float Z_alpha;              /* α 轴阻抗 */
    float Z_beta;               /* β 轴阻抗 */
    float Z_count;              /* 估计次数 */
    
    /* 状态 */
    Gfl_WeakGridLevel level;
    bool grid_connected;
    
    /* 用于阻抗估计的临时变量 */
    float P_prev;
    float Q_prev;
    float V_prev;
    float I_prev;
    
    float Ts;
};

void GflWeakGrid_Init(void *h, const Gfl_Config *cfg) {
    GflWeakGrid_Handle *handle = (GflWeakGrid_Handle *)h;
    
    handle->threshold = cfg->weak_grid_thresh;
    handle->Z_grid_est = cfg->Z_grid_est;
    handle->Ts = cfg->Ts;
    
    handle->Z_alpha = 0.0f;
    handle->Z_beta = 0.0f;
    handle->Z_count = 0.0f;
    
    handle->level = GFL_WEAK_GRID_STRONG;
    handle->grid_connected = false;
    
    handle->P_prev = 0.0f;
    handle->Q_prev = 0.0f;
    handle->V_prev = 1.0f;
    handle->I_prev = 0.0f;
}

void GflWeakGrid_Step(void *h, float grid_voltage, float grid_freq,
                       float I_out, float P_active, float Q_reactive,
                       bool pll_locked,
                       GflWeakGrid_Output *output) {
    GflWeakGrid_Handle *handle = (GflWeakGrid_Handle *)h;
    (void)grid_freq;
    
    output->level = handle->level;
    output->Z_grid_estimated = handle->Z_grid_est;
    output->grid_connected = handle->grid_connected;
    
    /* 检查 PLL 锁定状态 */
    if (!pll_locked) {
        handle->grid_connected = false;
        handle->level = GFL_WEAK_GRID_WEAK;
        output->level = GFL_WEAK_GRID_WEAK;
        return;
    }
    
    /* 简单的电网连接判断 */
    if (grid_voltage > 0.8f && grid_voltage < 1.2f) {
        handle->grid_connected = true;
    } else {
        handle->grid_connected = false;
    }
    
    /* 阻抗估计 (简化版) */
    /* Z = V / I，电网阻抗越大，相同电流下的电压变化越明显 */
    /* 使用功率变化率和电压变化率的比值来估计 */
    float dP = P_active - handle->P_prev;
    float dQ = Q_reactive - handle->Q_prev;
    float dV = grid_voltage - handle->V_prev;
    float dI = I_out - handle->I_prev;
    
    handle->P_prev = P_active;
    handle->Q_prev = Q_reactive;
    handle->V_prev = grid_voltage;
    handle->I_prev = I_out;
    
    /* 使用电流变化和电压变化的比值估计阻抗 */
    /* 这是一个简化估计，真实应用需要更复杂的算法 */
    if (fabsf(dI) > 0.01f && fabsf(dV) > 0.001f) {
        float Z_est = dV / dI;
        
        /* 滑动平均 */
        handle->Z_alpha = 0.9f * handle->Z_alpha + 0.1f * Z_est;
        handle->Z_beta = 0.9f * handle->Z_beta + 0.1f * Z_est;
        
        handle->Z_count += 1.0f;
    }
    
    /* 计算最终阻抗估计 */
    if (handle->Z_count > 10.0f) {
        handle->Z_grid_est = sqrtf(handle->Z_alpha * handle->Z_alpha + 
                                    handle->Z_beta * handle->Z_beta);
    }
    
    /* 根据阻抗判断弱网等级 */
    /* SCR (Short Circuit Ratio) = 1 / Z */
    /* SCR > 3: 强电网 */
    /* SCR 2-3: 边缘电网 */
    /* SCR < 2: 弱电网 */
    float SCR = 0.0f;
    if (handle->Z_grid_est > 0.01f) {
        SCR = 1.0f / handle->Z_grid_est;
    }
    
    if (SCR > 3.0f) {
        handle->level = GFL_WEAK_GRID_STRONG;
    } else if (SCR > 2.0f) {
        handle->level = GFL_WEAK_GRID_MARGINAL;
    } else {
        handle->level = GFL_WEAK_GRID_WEAK;
    }
    
    output->level = handle->level;
    output->Z_grid_estimated = handle->Z_grid_est;
    output->grid_connected = handle->grid_connected;
}

Gfl_WeakGridLevel GflWeakGrid_GetLevel(const void *h) {
    const GflWeakGrid_Handle *handle = (const GflWeakGrid_Handle *)h;
    return handle->level;
}

float GflWeakGrid_GetImpedance(const void *h) {
    const GflWeakGrid_Handle *handle = (const GflWeakGrid_Handle *)h;
    return handle->Z_grid_est;
}

bool GflWeakGrid_IsGridReady(const void *h) {
    const GflWeakGrid_Handle *handle = (const GflWeakGrid_Handle *)h;
    return handle->grid_connected;
}

void GflWeakGrid_Reset(void *h) {
    GflWeakGrid_Handle *handle = (GflWeakGrid_Handle *)h;
    
    handle->Z_alpha = 0.0f;
    handle->Z_beta = 0.0f;
    handle->Z_count = 0.0f;
    
    handle->level = GFL_WEAK_GRID_STRONG;
    handle->grid_connected = false;
}