# 正负序分解模块设计分析

## 1. 代码逻辑图

### 当前实现数据流

```mermaid
graph TD
    A[三相电压输入 v_a, v_b, v_c] --> B[Clarke变换]
    B --> C[αβ电压 v_alpha, v_beta]
    C --> D[PLL模块]
    D --> E[θ_pll, x[0], x[1]]
    C --> F[正负序分解]
    E --> F
    
    subgraph "正负序分解内部"
        F --> G[零序计算<br/>zero = 0]
        F --> H[正序Park变换<br/>θ_pos = θ_pll]
        F --> I[负序Park变换<br/>θ_neg = θ_pll + π]
        
        G --> J[输出 zero]
        H --> K[输出 pos_d, pos_q]
        I --> L[输出 neg_d, neg_q]
    end
```

### 关键算法流程

```mermaid
flowchart TD
    Start[SeqDecomp_Step_AlphaBeta] --> CheckInit{init == true?}
    CheckInit -- 否 --> Return[返回]
    CheckInit -- 是 --> SetZero[zero = 0]
    SetZero --> SetThetaPos[θ_pos = θ_pll]
    SetThetaPos --> SetThetaNeg[θ_neg = θ_pll + π]
    
    SetThetaNeg --> CalcCosSinPos[计算 cos(-θ_pos), sin(-θ_pos)]
    CalcCosSinPos --> ParkPos[正序Park变换<br/>pos_d = v_α·cos + v_β·sin<br/>pos_q = -v_α·sin + v_β·cos]
    
    SetThetaNeg --> CalcCosSinNeg[计算 cos(-θ_neg), sin(-θ_neg)]
    CalcCosSinNeg --> ParkNeg[负序Park变换<br/>neg_d = v_α·cos + v_β·sin<br/>neg_q = -v_α·sin + v_β·cos]
    
    ParkPos --> UpdateHistory[更新历史值]
    ParkNeg --> UpdateHistory
    UpdateHistory --> Output[输出结果]
```

## 2. 时序分析

### 2.1 ISR (中断服务程序) 时序

假设典型运行条件：
- 开关频率: 50kHz → 控制周期 T_s = 20μs
- MCU: ARM Cortex-M4 @ 168MHz
- 浮点单元 (FPU) 可用

| 操作 | 指令周期估计 | 时间 @168MHz |
|------|--------------|--------------|
| Clarke变换 (3相→αβ) | ~10 cycles | 0.06μs |
| PLL SOGI更新 (2状态) | ~30 cycles | 0.18μs |
| PLL PI控制器更新 | ~50 cycles | 0.30μs |
| 三角函数计算 (cos/sin) ×2 | ~100 cycles ×2 | 1.19μs |
| Park变换 (正序+负序) ×2 | ~40 cycles ×2 | 0.48μs |
| 其他逻辑 | ~20 cycles | 0.12μs |
| **总计** | **~350 cycles** | **~2.08μs** |

**时序结论**: 当前实现约占用 2.08μs，满足 20μs 控制周期要求，但有优化空间。

### 2.2 最坏情况时序路径

最坏情况发生在所有三角函数都需计算时：

$$
T_{worst} = T_{clarke} + T_{pll} + 2 \times T_{trig} + 2 \times T_{park}
$$

$$
T_{worst} \approx 0.06 + (0.18+0.30) + 2 \times 1.19 + 2 \times 0.24 = 3.40 \mu s
$$

**安全裕度**: 
$$
Margin = \frac{T_s - T_{worst}}{T_s} \times 100\% = \frac{20 - 3.40}{20} \times 100\% = 83\%
$$

裕度充足，但三角函数计算是主要瓶颈。

### 2.3 中断抢占风险

如果 PLL 和正负序分解在同一个 ISR 中顺序执行：
- 无抢占风险（单 ISR 设计）
- 但需注意函数调用深度导致的栈使用

## 3. 兼容性分析

### 3.1 三相三桥臂逆变器 (Three-Leg Inverter)

**当前代码支持情况**:
- ✅ αβ变换正确（Clarke变换）
- ✅ 零序分量强制为 0，符合理论（三相三线制无零序通路）
- ✅ 正负序分解基本功能完整

**问题**:
- ❌ Clarke变换公式有误：第90行 `v_beta = (v_a + 2.0f * v_b) * SQRT3_INV`
  正确公式应为：`v_beta = (v_b - v_c) * INV_SQRT3` 或 `v_beta = (v_a + 2*v_b)/sqrt(3)` 当 `v_a + v_b + v_c = 0` 时

### 3.2 三相四桥臂逆变器 (Four-Leg Inverter)

**当前代码缺陷**:
1. **零序分量处理**:
   - 第116行：`*zero = 0.0f` 硬编码为零
   - 无法提取零序电流/电压用于第四桥臂控制

2. **Clarke变换不完整**:
   - 需要输出零序分量：`v_zero = (v_a + v_b + v_c) / 3`
   - 当前实现丢失了零序信息

3. **控制接口缺失**:
   - 无零序控制回路输入/输出

**修改建议**:
```c
// 修改 Clarke 变换以支持零序
void Clarke_Transform(float v_a, float v_b, float v_c, 
                      float* v_alpha, float* v_beta, float* v_zero) {
    *v_alpha = v_a;  // 或使用 2/3 变换系数
    *v_beta = (v_a + 2.0f * v_b) * SQRT3_INV;
    *v_zero = (v_a + v_b + v_c) / 3.0f;
}

// 修改 SeqDecomp_Step_AlphaBeta 函数签名
void SeqDecomp_Step_AlphaBeta(SeqDecomp_Handle *h,
                              float v_alpha, float v_beta, float v_zero,
                              float theta_pll,
                              float *pos_d, float *pos_q,
                              float *neg_d, float *neg_q,
                              float *zero);
```

## 4. PLL 结果复用优化

### 4.1 SOGI 输出复用分析

PLL 模块已计算：
- `h->x[0]`: v_alpha 的同相分量（滤波后）
- `h->x[1]`: v_alpha 的正交分量（90°延迟）

**当前浪费的计算**:
- 正负序分解中重复生成正交信号
- 重复计算三角函数

**优化方案**:

1. **共享正交信号**:
   ```c
   // PLL 已计算 x[1] 作为 v_alpha 的正交分量
   // 可直接用于负序提取的延迟信号法
   float v_alpha_ortho = pll_handle->x[1];
   ```

2. **共享角度和三角函数**:
   ```c
   // PLL 已计算 θ，可在同一 ISR 中传递 sin/cos 值
   // 避免重复计算
   typedef struct {
       float theta;
       float sin_theta;
       float cos_theta;
   } Pll_Output;
   ```

### 4.2 具体优化实现

**方案A: 修改函数接口**
```c
// 新函数：利用 PLL 中间结果加速
void SeqDecomp_Step_Fast(SeqDecomp_Handle *h,
                         float v_alpha, float v_beta,
                         float theta_pll,
                         float sin_theta, float cos_theta,
                         float v_alpha_ortho,  // PLL SOGI 正交输出
                         float *pos_d, float *pos_q,
                         float *neg_d, float *neg_q,
                         float *zero);
```

**方案B: 结构体封装**
```c
typedef struct {
    float theta;
    float sin_theta;
    float cos_theta;
    float v_alpha_filtered;     // SOGI 同相输出
    float v_alpha_ortho;        // SOGI 正交输出
} Pll_ExtendedOutput;

// 修改 PLL_Step 返回扩展信息
bool Pll_Step_Extended(Pll_Handle *h, float v_alpha, float v_beta,
                       Pll_ExtendedOutput *out);
```

### 4.3 相位误差影响分析

**电网不平衡时的 PLL 行为**:
- 正序 PLL 在电网不平衡时会产生 2ω 纹波
- 相位误差可能导致正负序交叉耦合

**建议**:
1. **使用 DSRF-PLL** (双同步参考坐标系 PLL) 减少不平衡影响
2. **增加正负序分离的前置滤波**:
   ```c
   // 在 PLL 前加入正负序分离网络
   // 或使用多重复频率 PLL
   ```

## 5. 代码逻辑问题审查

### 5.1 负序角度计算

**当前实现**:
```c
h->theta_neg = normalize_angle_pi(theta_pll + 3.14159265358979f);
```

**数学分析**:
- 负序分量旋转方向与正序相反：θ_neg = -θ_pos
- 当前公式：θ_neg = θ_pos + π
- 关系：-θ_pos ≡ θ_pos + π (模 2π)

**验证**:
```
设 θ_pos = 30° = 0.5236 rad
-θ_pos = -30° = 330° = 5.7596 rad
θ_pos + π = 30° + 180° = 210° = 3.6652 rad
210° ≠ 330° (相差 120°)
```

**结论**: 公式错误！正确应为：
```c
h->theta_neg = normalize_angle_pi(-theta_pll);
```

### 5.2 Park 变换公式

**当前实现** (第133-134行):
```c
h->pos_d = v_alpha * cos_pos + v_beta * sin_pos;
h->pos_q = -v_alpha * sin_pos + v_beta * cos_pos;
```

其中 `cos_pos = cosf(-theta_pll)`, `sin_pos = sinf(-theta_pll)`

**标准 Park 变换**:
```
V_d = V_α·cos(θ) + V_β·sin(θ)
V_q = -V_α·sin(θ) + V_β·cos(θ)
```

**分析**: 
- 使用 `cos(-θ) = cos(θ)`, `sin(-θ) = -sin(θ)`
- 实际相当于旋转角度为 -θ
- 对于正序同步旋转坐标系，通常使用 θ（而非 -θ）

**建议**: 统一角度符号约定
```c
// 方案1：使用正角度
float cos_pos = cosf(theta_pll);
float sin_pos = sinf(theta_pll);
h->pos_d = v_alpha * cos_pos + v_beta * sin_pos;
h->pos_q = -v_alpha * sin_pos + v_beta * cos_pos;

// 方案2：使用负角度但调整公式
```

### 5.3 数值稳定性问题

1. **三角函数计算频率**:
   - 每个控制周期计算 2 次 cosf/sinf
   - 可改用查表法或近似计算

2. **角度归一化**:
   - `normalize_angle_pi` 使用 `fmodf`，可能较慢
   - 可优化为整数运算

3. **浮点误差累积**:
   - SOGI 离散化使用前向欧拉，数值稳定性较差
   - 建议改用梯形积分或双线性变换

## 6. 技术改进建议

### 6.1 底层优化建议

| 优化项 | 当前问题 | 建议方案 | 预计节省 |
|--------|----------|----------|----------|
| 三角函数计算 | 每个周期 2 次 cosf/sinf | 使用查表法或 Cordic | ~1.5μs |
| SOGI 重复计算 | PLL 和序列分解各自计算正交 | 共享 PLL 的 SOGI 输出 | ~0.2μs |
| 零序计算 | 硬编码为零 | 根据拓扑动态选择 | - |
| Clarke变换 | 公式可能错误 | 修正并支持零序输出 | - |

### 6.2 控制策略优化

**离散化误差改善**:
```c
// 当前：前向欧拉 (稳定性差)
float x0_next = x0 + Ts * (-omega_n * omega_n * x1 + omega_n * u);

// 建议：双线性变换 (Tustin)
float a = omega_n * Ts / 2;
float denom = 1 + a*a;
float x0_next = ((1 - a*a)*x0 - 2*a*omega_n*x1 + 2*a*omega_n*u) / denom;
float x1_next = (2*a*x0 + (1 - a*a)*x1) / denom;
```

**抗饱和策略**:
```c
// 当前：无抗饱和
// 建议：增加积分抗饱和
if (fabsf(h->integrator) > max_integrator) {
    h->integrator = copysignf(max_integrator, h->integrator);
}
```

### 6.3 鲁棒性加固

**异常处理增强**:
```c
// 增加输入有效性检查
bool SeqDecomp_Step_AlphaBeta(SeqDecomp_Handle *h,
                              float v_alpha, float v_beta,
                              float theta_pll,
                              float *pos_d, float *pos_q,
                              float *neg_d, float *neg_q,
                              float *zero) {
    // 检查 NaN/Inf
    if (!isfinite(v_alpha) || !isfinite(v_beta)) {
        SeqDecomp_Reset(h);
        return false;
    }
    
    // 检查角度范围
    if (theta_pll > 100.0f * PI) {  // 防止积分溢出
        theta_pll = normalize_angle_2pi(theta_pll);
    }
    
    // ... 正常处理
    return true;
}
```

**边界条件处理**:
```c
// 电压过低时的降级策略
float v_amplitude = sqrtf(v_alpha*v_alpha + v_beta*v_beta);
if (v_amplitude < 0.1f) {  // 低于 10% 额定电压
    // 使用上一次有效值或开环估计
    *pos_d = h->pos_d;
    *pos_q = h->pos_q;
    *neg_d = 0.0f;
    *neg_q = 0.0f;
    *zero = 0.0f;
    return;
}
```

## 7. 具体修改方案

### 7.1 紧急修复（必须修改）

1. **修正负序角度计算** (seq_decomp.c 第123行):
   ```c
   // 错误：
   h->theta_neg = normalize_angle_pi(theta_pll + 3.14159265358979f);
   // 正确：
   h->theta_neg = normalize_angle_pi(-theta_pll);
   ```

2. **修正 Clarke 变换公式** (seq_decomp.c 第90行):
   ```c
   // 当前（可能错误）：
   float v_beta = (v_a + 2.0f * v_b) * SQRT3_INV;
   // 标准公式（假设 v_a + v_b + v_c = 0）：
   float v_beta = (v_a - v_c) * INV_SQRT3;
   // 或更通用的 Clark-Concordia 变换：
   float v_alpha = (2.0f/3.0f) * (v_a - 0.5f*v_b - 0.5f*v_c);
   float v_beta = (2.0f/3.0f) * (SQRT3_DIV2 * (v_b - v_c));
   ```

### 7.2 功能增强（建议修改）

1. **增加零序支持**:
   - 修改函数签名，增加 `v_zero` 输入参数
   - 根据逆变器拓扑配置零序处理模式

2. **PLL 结果复用**:
   - 创建共享数据结构
   - 修改调用接口传递中间结果

### 7.3 性能优化（可选修改）

1. **三角函数优化**:
   - 实现 12-bit 查表法（4KB 表）
   - 或使用 ARM CMSIS DSP 库的快速三角函数

2. **SOGI 离散化改进**:
   - 改用双线性变换提高数值稳定性

## 8. 总结

**关键发现**:
1. ❌ **负序角度计算公式错误** - 必须修复
2. ❌ **Clarke 变换公式存疑** - 需要验证和修正  
3. ⚠️ **零序处理不支持四桥臂逆变器** - 功能缺失
4. ⚠️ **PLL 中间结果未复用** - 性能优化空间
5. ✅ **时序满足实时性要求** - 但可进一步优化

**推荐优先级**:
1. **P0**: 修复负序角度和 Clarke 变换公式
2. **P1**: 增加零序分量支持
3. **P2**: 复用 PLL 结果优化性能
4. **P3**: 数值稳定性改进

**预计工作量**:
- 紧急修复: 2-4 小时
- 功能增强: 8-16 小时  
- 性能优化: 4-8 小时

**验证建议**:
1. 单元测试：验证负序角度计算正确性
2. 硬件在环：测试三相四桥臂支持
3. 性能分析：测量优化前后 ISR 执行时间