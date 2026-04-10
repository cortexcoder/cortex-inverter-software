#include "pi_ctrl.h"
#include <arm_math.h>

/**
 * @brief 初始化 PI 控制器
 */
void PiCtrl_Init(PiCtrl_Handle *h, const PiCtrl_Config *cfg) {
    h->config = cfg;
    h->integrator = 0.0f;
    h->out_prev = 0.0f;
    h->ref_prev = 0.0f;
}

/**
 * @brief PI 控制器单步执行（带抗积分饱和）
 * 
 * 使用条件积分算法防止积分器溢出 (windup):
 * - 当输出饱和时，只有当误差符号与控制目标一致时才进行积分
 * - 这避免了积分器在达到限幅后继续累积误差
 * 
 * @param h PI 控制器句柄
 * @param ref 参考值
 * @param fdb 反馈值
 * @param out 输出值
 */
void PiCtrl_Step(PiCtrl_Handle *h, float ref, float fdb, float *out) {
    const PiCtrl_Config *c = h->config;

    const float error = ref - fdb;

    const float kp = c->kp;
    const float ki = c->ki;
    const float Ts = c->Ts;

    const float out_max = c->out_max;
    const float out_min = c->out_min;

    // 比例项
    float proportional = kp * error;

    // 积分项（带抗饱和）
    // 条件积分：只有当积分器输出不会加剧饱和时才积分
    float u = proportional + h->integrator;
    
    // 检测输出饱和
    bool saturated = false;
    bool saturate_high = false;
    bool saturate_low = false;
    
    if (u > out_max) {
        saturated = true;
        saturate_high = true;
    } else if (u < out_min) {
        saturated = true;
        saturate_low = true;
    }
    
    // 抗积分饱和逻辑：
    // 如果饱和且误差方向试图进一步加剧饱和，则停止积分
    // 否则正常积分
    if (saturated) {
        // 输出已饱和的情况
        if (saturate_high && error > 0.0f) {
            // 输出超过上限且误差为正（试图进一步增加输出）
            // 此时不应继续积分
        } else if (saturate_low && error < 0.0f) {
            // 输出低于下限且误差为负（试图进一步减小输出）
            // 此时不应继续积分
        } else {
            // 误差方向与饱和方向相反，可以继续积分（退饱和）
            h->integrator += ki * Ts * error;
        }
    } else {
        // 正常积分
        h->integrator += ki * Ts * error;
    }

    // 重新计算输出
    u = proportional + h->integrator;

    // 输出限幅
    if (u > out_max) {
        u = out_max;
    } else if (u < out_min) {
        u = out_min;
    }

    h->out_prev = u;
    h->ref_prev = ref;
    *out = u;
}

/**
 * @brief 重置 PI 控制器
 */
void PiCtrl_Reset(PiCtrl_Handle *h) {
    h->integrator = 0.0f;
    h->out_prev = 0.0f;
    h->ref_prev = 0.0f;
}

/**
 * @brief 设置积分器值
 */
void PiCtrl_SetIntegral(PiCtrl_Handle *h, float value) {
    h->integrator = value;
}

/**
 * @brief 获取积分器值
 */
float PiCtrl_GetIntegral(const PiCtrl_Handle *h) {
    return h->integrator;
}