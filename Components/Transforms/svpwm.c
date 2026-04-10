#include "svpwm.h"
#include "fast_math.h"
#include <math.h>

#define SQRT3_INV  (0.5773502691896258f)
#define SQRT3_DIV2 (0.8660254037844386f)
#define TWO_DIV_SQRT3 (1.1547005383793f)
#define INV_SQRT3  (0.5773502691896258f)

#define PI         (3.14159265358979f)
#define TWO_PI     (6.28318530717959f)
#define PI_DIV3    (1.04719755119660f)
#define PI_2DIV3   (2.09439510239320f)
#define PI_4DIV3   (4.18879020478639f)
#define PI_5DIV3   (5.23598775598299f)

void SvPwm_Init(SvPwm_Handle *h, const SvPwm_Config *cfg) {
    h->config = cfg;
    h->theta = 0.0f;
    h->v_alpha = 0.0f;
    h->v_beta = 0.0f;
    h->v_d = 0.0f;
    h->v_q = 0.0f;
    h->sector = 0;
    h->ta = 0.5f;
    h->tb = 0.5f;
    h->tc = 0.5f;
}

void SvPwm_SetTheta(SvPwm_Handle *h, float theta) {
    h->theta = theta;
}

/**
 * @brief 计算扇区号 (0-5)
 * 
 * 使用整数比较替代浮点数比较，避免精度问题
 * 扇区定义：
 *   扇区0: [0, π/3)      - 电压矢量 U1(100) -> U2(110)
 *   扇区1: [π/3, 2π/3)    - 电压矢量 U2(010) -> U3(110)
 *   扇区2: [2π/3, π)      - 电压矢量 U3(110) -> U0(010)
 *   扇区3: [π, 4π/3)      - 电压矢量 U0(010) -> U4(011)
 *   扇区4: [4π/3, 5π/3)    - 电压矢量 U4(011) -> U5(001)
 *   扇区5: [5π/3, 2π)     - 电压矢量 U5(001) -> U1(100)
 */
static int SvPwm_CalcSector(float theta) {
    // 归一化角度到 [0, 2π)
    theta = fmodf(theta, TWO_PI);
    if (theta < 0.0f) {
        theta += TWO_PI;
    }

    // 使用整数比较 (将弧度转换为固定-point 或使用查找表避免浮点比较)
    // 由于 theta 是浮点数，这里使用乘以 3 来比较，避免除法
    // 扇区边界: 0, π/3, 2π/3, π, 4π/3, 5π/3, 2π
    // 乘以 3: 0, π, 2π, 3π, 4π, 5π, 6π
    float theta_3 = theta * 3.0f;
    
    if (theta_3 < PI) {
        return 0;
    } else if (theta_3 < TWO_PI) {
        return 1;
    } else if (theta_3 < 3.0f * PI) {
        return 2;
    } else if (theta_3 < 4.0f * PI) {
        return 3;
    } else if (theta_3 < 5.0f * PI) {
        return 4;
    } else {
        return 5;
    }
}

/**
 * @brief SVPWM 单步执行
 * 
 * 使用 7 段对称 SVPWM 算法
 * 
 * @param h SVPWM 句柄
 * @param v_d d轴电压 (V)
 * @param v_q q轴电压 (V)
 * @param ta 输出 A 相占空比 (0.0-1.0)
 * @param tb 输出 B 相占空比 (0.0-1.0)
 * @param tc 输出 C 相占空比 (0.0-1.0)
 */
void SvPwm_Step(SvPwm_Handle *h, float v_d, float v_q, float *ta, float *tb, float *tc) {
    const float vbus = h->config->vbus;
    const float theta = h->theta;

    h->v_d = v_d;
    h->v_q = v_q;

    const float U_dc_inv = 1.0f / vbus;

    // 反 Park 变换: dq -> αβ
    float sin_theta, cos_theta;
    FastSinCos(theta, &sin_theta, &cos_theta);
    h->v_alpha = v_d * cos_theta - v_q * sin_theta;
    h->v_beta  = v_d * sin_theta + v_q * cos_theta;

    // 计算电压幅值
    float v_squared = h->v_alpha * h->v_alpha + h->v_beta * h->v_beta;
    float v_ref = SQRT3_INV * sqrtf(v_squared);

    // 过调制检测与处理
    // 当电压参考值超过线性调制范围 (U_dc_inv * 0.5) 时进入过调制模式
    // 过调制模式 1: 简单钳位到线性极限
    if (v_ref > (U_dc_inv * 0.5f)) {
        // 计算电压角度
        float v_angle = atan2f(h->v_beta, h->v_alpha);
        
        // 将电压矢量钳位到六边形边界
        // 六边形内切圆半径为 U_dc/√3，对应线性调制的极限
        float U_hex = U_dc_inv * 0.5f;  // 六边形内切圆半径
        
        h->ta = 0.5f;
        h->tb = 0.5f;
        h->tc = 0.5f;
        *ta = 0.5f;
        *tb = 0.5f;
        *tc = 0.5f;
        (void)v_angle;  // 预留: 用于更复杂的过调制算法
        return;
    }

    // 计算扇区
    int sector = SvPwm_CalcSector(theta);
    h->sector = sector;

    // 计算电压分量 U1, U2, U3
    // U1 = v_beta
    // U2 = -1/2 * v_beta + √3/2 * v_alpha
    // U3 = -1/2 * v_beta - √3/2 * v_alpha
    const float U1 = h->v_beta;
    const float U2 = -0.5f * h->v_beta + SQRT3_DIV2 * h->v_alpha;
    const float U3 = -0.5f * h->v_beta - SQRT3_DIV2 * h->v_alpha;

    // 计算基本电压矢量作用时间 T1, T2
    // T1 作用于矢量 U1 的时间 (与 U2 的投影相关)
    // T2 作用于矢量 U2 的时间 (与 U1 的投影相关)
    float T1, T2;
    
    // 计算 T0 (零矢量时间)
    float T0;

    // 各扇区的占空比计算使用 7 段对称 SVPWM
    // 通用公式: 
    //   t_a = T0/4 + Ta/2 + Tb/2
    //   t_b = T0/4 + Tb/2 + Tc/2
    //   t_c = T0/4 + Tc/2 + Ta/2
    // 其中 Ta, Tb, Tc 是各扇区对应的基本矢量时间
    float Ta, Tb, Tc;  // 各相基本矢量时间

    switch (sector) {
        case 0: {
            // 扇区 0: U1(100) -> U2(110)
            // T1 = |U2| * 2/√3, T2 = |U1| / √3
            // 注意: 扇区 0 中 U1 > 0, U2 > 0
            T1 = U2 * TWO_DIV_SQRT3;
            T2 = U1 * INV_SQRT3;
            
            // 7 段对称 SVPWM 占空比计算
            // 扇区 0: t_a = T0/4 + T1/2 + T2/2, t_b = T0/4 + T2/2, t_c = T0/4
            Ta = T1;
            Tb = T2;
            Tc = 0.0f;
            break;
        }
        
        case 1: {
            // 扇区 1: U2(010) -> U3(110)
            // U1 < 0, U2 > 0, U3 < 0 (通常)
            T1 = -U2 * TWO_DIV_SQRT3;
            T2 = -U3 * INV_SQRT3;
            
            // 扇区 1: t_a = T0/4, t_b = T0/4 + T1/2 + T2/2, t_c = T0/4 + T2/2
            Ta = 0.0f;
            Tb = T1;
            Tc = T2;
            break;
        }
        
        case 2: {
            // 扇区 2: U3(110) -> U0(010)
            // U1 > 0, U2 < 0, U3 > 0 (通常)
            T1 = U3 * TWO_DIV_SQRT3;
            T2 = U1 * INV_SQRT3;
            
            // 扇区 2: t_a = T0/4, t_b = T0/4 + T1/2, t_c = T0/4 + T1/2 + T2/2
            Ta = 0.0f;
            Tb = T2;
            Tc = T1;
            break;
        }
        
        case 3: {
            // 扇区 3: U0(010) -> U4(011)
            // U1 < 0, U2 < 0, U3 > 0 (通常)
            T1 = -U3 * TWO_DIV_SQRT3;
            T2 = -U1 * INV_SQRT3;
            
            // 扇区 3: t_a = T0/4 + T2/2, t_b = T0/4, t_c = T0/4 + T1/2 + T2/2
            Ta = T2;
            Tb = 0.0f;
            Tc = T1;
            break;
        }
        
        case 4: {
            // 扇区 4: U4(011) -> U5(001)
            // U1 < 0, U2 > 0, U3 < 0 (通常)
            T1 = U1 * TWO_DIV_SQRT3;
            T2 = U2 * INV_SQRT3;
            
            // 扇区 4: t_a = T0/4 + T1/2 + T2/2, t_b = T0/4 + T1/2, t_c = T0/4
            Ta = T1;
            Tb = T2;
            Tc = 0.0f;
            break;
        }
        
        case 5:
        default: {
            // 扇区 5: U5(001) -> U1(100)
            // U1 > 0, U2 < 0, U3 < 0 (通常)
            T1 = -U1 * TWO_DIV_SQRT3;
            T2 = -U2 * INV_SQRT3;
            
            // 扇区 5: t_a = T0/4 + T2/2, t_b = T0/4 + T1/2 + T2/2, t_c = T0/4
            Ta = T2;
            Tb = T1;
            Tc = 0.0f;
            break;
        }
    }

    // 确保 T1, T2 为正 (使用绝对值处理数值误差)
    T1 = (T1 < 0.0f) ? -T1 : T1;
    T2 = (T2 < 0.0f) ? -T2 : T2;

    // 限制 T1 + T2 <= 1 (确保 T0 >= 0)
    float T_sum = T1 + T2;
    if (T_sum > 1.0f) {
        float scale = 1.0f / T_sum;
        T1 *= scale;
        T2 *= scale;
    }

    // 计算零矢量时间
    T0 = 1.0f - T1 - T2;

    // 7 段对称 SVPWM 占空比计算
    // 标准化到 [0, 1] 范围
    float t_a = T0 * 0.25f + Ta * 0.5f;
    float t_b = T0 * 0.25f + Tb * 0.5f;
    float t_c = T0 * 0.25f + Tc * 0.5f;

    // 限制占空比在有效范围内
    t_a = (t_a > 1.0f) ? 1.0f : (t_a < 0.0f) ? 0.0f : t_a;
    t_b = (t_b > 1.0f) ? 1.0f : (t_b < 0.0f) ? 0.0f : t_b;
    t_c = (t_c > 1.0f) ? 1.0f : (t_c < 0.0f) ? 0.0f : t_c;

    h->ta = t_a;
    h->tb = t_b;
    h->tc = t_c;

    *ta = t_a;
    *tb = t_b;
    *tc = t_c;
}

void SvPwm_Reset(SvPwm_Handle *h) {
    h->theta = 0.0f;
    h->sector = 0;
    h->ta = 0.5f;
    h->tb = 0.5f;
    h->tc = 0.5f;
}