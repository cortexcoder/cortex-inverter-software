# 分相功率控制和N桥臂扩展架构审查

## 概述
本文档对新增的分相功率控制和N桥臂扩展模块进行最终审查，涵盖逻辑图、时序分析和具体改进建议。

## 1. 代码逻辑图

### 1.1 分相功率控制主流程
```mermaid
graph TD
    A[开始: 分相功率转电流] --> B{电流模式?};
    B -->|对称模式| C[计算三相总功率 P_total, Q_total];
    B -->|不对称模式| D[各相独立功率 P_i, Q_i];
    
    C --> E[计算平均电压 V_avg];
    E --> F[计算单相电流 Id, Iq = f(P_avg, Q_avg, V_avg)];
    F --> G[三相赋值相同 Id/Iq];
    G --> H[N相电流置零];
    
    D --> I[遍历各相];
    I --> J[单相功率转电流 Id_i, Iq_i = f(P_i, Q_i, V_i)];
    J --> K{是否最后一相?};
    K -->|否| I;
    K -->|是| L[输出分相电流];
    
    H --> L;
    
    L --> M[竞争仲裁];
    M --> N[两阶段限幅];
    N --> O[输出最终电流];
```

### 1.2 竞争仲裁逻辑
```mermaid
graph TD
    A[开始: 竞争仲裁] --> B[初始化输出为零];
    B --> C[遍历所有请求];
    C --> D{请求有效?};
    D -->|否| E[跳过];
    D -->|是| F[比较优先级];
    
    F --> G{优先级 > 当前最高?};
    G -->|是| H[更新最高优先级<br/>清空同优先级缓存];
    G -->|否| I{优先级 == 当前最高?};
    I -->|是| J[加入同优先级缓存];
    I -->|否| E;
    
    H --> J;
    J --> K{是否最后一个请求?};
    K -->|否| C;
    K -->|是| L[计算同优先级加权平均];
    
    L --> M[加权平均 Id_avg, Iq_avg];
    M --> N[应用到所有相<br/>(仅对称模式)];
    N --> O[输出仲裁结果];
```

### 1.3 两阶段限幅逻辑
```mermaid
graph TD
    A[开始: 两阶段限幅] --> B{电流模式?};
    B -->|对称模式| C[取第一相 Id[0], Iq[0]];
    C --> D[计算电流矢量幅值 I_mag];
    D --> E{I_mag > I_max?};
    E -->|是| F[计算缩放因子 scale = I_max / I_mag];
    F --> G[所有三相按比例缩放];
    E -->|否| H[进入矩形限幅];
    G --> H;
    
    H --> I[遍历各相];
    I --> J[d轴限幅: Id_min ≤ Id ≤ Id_max];
    J --> K[q轴限幅: -Iq_max ≤ Iq ≤ Iq_max];
    K --> L[圆形限幅: √(Id²+Iq²) ≤ I_max];
    L --> M{是否最后一相?};
    M -->|否| I;
    M -->|是| N[N相电流置零];
    
    B -->|不对称模式| O[遍历所有相];
    O --> P[单相矩形限幅];
    P --> Q[单相圆形限幅];
    Q --> R{是否最后一相?};
    R -->|否| O;
    R -->|是| S[输出限幅后电流];
    
    N --> S;
```

## 2. 时序分析

### 2.1 执行时间估算
假设使用 STM32H743 (480MHz, 双精度 FPU)，各操作指令周期近似估算：

| 操作 | 指令周期 | 时间 (480MHz) |
|------|----------|---------------|
| 浮点乘法 | 4 cycles | 8.33 ns |
| 浮点除法 | 14 cycles | 29.17 ns |
| 浮点平方根 | 28 cycles | 58.33 ns |
| 条件分支 | 2 cycles | 4.17 ns |

### 2.2 分相功率计算时序
- **对称模式** (三相三线):
  - 功率求和: 3 × (加法 + 除法) ≈ 3 × (4 + 14) = 54 cycles
  - 电压平均: 3次加法 + 1次除法 = 3×4 + 14 = 26 cycles
  - 单相电流计算: 2次除法 + 2次乘法 = 2×14 + 2×4 = 36 cycles
  - 三相赋值: 6次赋值 = 6×2 = 12 cycles
  - **合计**: 128 cycles ≈ 0.27 μs

- **不对称模式** (6桥臂):
  - 每相独立计算: 6 × 36 cycles = 216 cycles
  - **合计**: 216 cycles ≈ 0.45 μs

### 2.3 竞争仲裁时序
- 遍历请求 (最大6个): 6 × (有效性检查 + 优先级比较) ≈ 6 × (4 + 4) = 48 cycles
- 加权平均计算: 乘积累加 + 除法 ≈ 6×8 + 14 = 62 cycles
- **最坏情况**: 110 cycles ≈ 0.23 μs

### 2.4 两阶段限幅时序
- **对称模式**:
  - 矢量幅值计算: 2次乘法 + 1次平方根 = 2×4 + 28 = 36 cycles
  - 比例缩放: 6次乘法 = 6×4 = 24 cycles
  - 每相矩形限幅: 3 × (4次比较 + 2次赋值) ≈ 3×16 = 48 cycles
  - 每相圆形限幅: 3 × (36 cycles) = 108 cycles
  - **合计**: 216 cycles ≈ 0.45 μs

- **不对称模式** (6桥臂):
  - 每相独立限幅: 6 × (48 + 36) = 504 cycles
  - **合计**: 504 cycles ≈ 1.05 μs

### 2.5 总体时序预算
- **GFL_Task_1ms** 总时间预算: 1ms = 1000 μs
- 现有 GFL 环路: ~2.5 μs (估算)
- 新增分相控制:
  - 对称模式: 0.27 + 0.23 + 0.45 = 0.95 μs
  - 不对称模式: 0.45 + 0.23 + 1.05 = 1.73 μs
- **余量充足**: 即使在 6 桥臂不对称模式下，总耗时 < 5 μs << 1000 μs

### 2.6 实时性关键路径
在 24kHz 快速电流环中，分相电流参考通过宏 `GFL_GET_ID_REF_PHASE` 直接访问，无额外计算延迟。

**中断服务程序时序链**:
```
ADC中断 (41.67μs) 
  → PLL锁相 (5μs) 
  → Park变换 (2μs) 
  → PI控制 (3μs) 
  → 获取分相电流参考 (直接内存访问, <0.1μs)
  → SVPWM (8μs)
```
**总 ISR 时间**: ~18.17 μs < 41.67 μs (24kHz 周期)，满足实时性要求。

## 3. 技术改进建议

### 3.1 组件化程度优化
**现状**: 新增模块 `gfl_split_phase.c/h` 与现有框架隔离良好，通过配置结构体 `Gfl_Config` 传递参数。

**改进建议**:
1. **依赖注入**: 在 `Gfl_Config` 中增加分相控制回调函数，允许用户自定义功率分配算法。
   ```c
   typedef void (*Gfl_SplitPhaseCb)(void *context, 
                                   const Gfl_PhasePower *power,
                                   Gfl_SplitCurrentRef *output);
   ```
2. **编译时选择**: 添加预处理器宏 `GFL_ENABLE_SPLIT_PHASE`，允许完全禁用分相功能以减小代码体积。

### 3.2 扩展性增强
**现状**: `Gfl_SplitCurrentRef` 支持 6 桥臂，但循环常硬编码 `i < 3`。

**改进建议**:
1. **动态相数处理**: 修改所有循环使用 `output->num_phases` 而非固定值 3。
   ```c
   for (uint8_t i = 0; i < output->num_phases; i++) {
       // 处理所有相
   }
   ```
2. **N相电流计算**: 在不对称模式下，自动计算 N 相电流为其他相电流之和的相反数：
   $$ I_{N,d} = -\sum_{i=A,B,C} I_{i,d} $$
   $$ I_{N,q} = -\sum_{i=A,B,C} I_{i,q} $$
3. **桥臂间耦合约束**: 添加总功率/总电流限制，即使各相独立限幅，也需满足系统总容量。

### 3.3 宏切换方式改进
**现状**: `GFL_GRID_IS_ABC3` 和 `GFL_GRID_IS_ABCN4` 宏定义清晰。

**改进建议**:
1. **运行时验证**: 在初始化时检查 `grid_type` 与 `num_phases` 的一致性：
   ```c
   if (grid_type == GRID_TYPE_ABC3 && num_phases != 3) {
       return GFL_ERROR_INVALID_CONFIG;
   }
   if (grid_type == GRID_TYPE_ABCN4 && num_phases != 4) {
       return GFL_ERROR_INVALID_CONFIG;
   }
   ```
2. **扩展电网类型**: 预留枚举值支持更多拓扑：
   ```c
   GRID_TYPE_ABCD5 = 5,   // 四相系统
   GRID_TYPE_ABCDE6 = 6,  // 五相系统
   ```

### 3.4 限幅逻辑完善
**现状**: 两阶段限幅逻辑基本正确，但存在优化空间。

**改进建议**:
1. **限幅顺序优化**: 改为 **矩形限幅 → 圆形限幅** 更合理，避免矩形限幅破坏已缩放的圆形边界：
   ```c
   // 阶段1: 矩形限幅
   Gfl_LimitPhaseCurrent(&Id, &Iq, limits);
   // 阶段2: 圆形限幅
   float I_mag = sqrtf(Id*Id + Iq*Iq);
   if (I_mag > limits->I_max) {
       float scale = limits->I_max / I_mag;
       Id *= scale; Iq *= scale;
   }
   ```
2. **不对称模式耦合限幅**: 添加相间耦合检查，确保任意两相电流矢量和不超限：
   ```c
   for (int i = 0; i < num_phases; i++) {
       for (int j = i+1; j < num_phases; j++) {
           float Id_sum = Id[i] + Id[j];
           float Iq_sum = Iq[i] + Iq[j];
           float I_mag_sum = sqrtf(Id_sum*Id_sum + Iq_sum*Iq_sum);
           if (I_mag_sum > limits->I_max * 1.732f) { // √3 因子
               // 调整算法
           }
       }
   }
   ```
3. **抗饱和策略**: 在 PI 控制器前加入抗饱和补偿，避免积分器 windup：
   ```c
   float error = ref - fdb;
   float integral = prev_integral + Ki * error;
   // 限幅后补偿
   if (integral > integral_max) {
       integral = integral_max;
       // 反向补偿误差
       error = (integral_max - prev_integral) / Ki;
   }
   ```

### 3.5 对称/不对称统一设计
**现状**: `calc_single_phase_current` 函数统一了单相计算，但对称模式使用平均电压可能引入误差。

**改进建议**:
1. **对称模式精确计算**: 使用各相实际电压而非平均值：
   ```c
   float Id_total = 0.0f, Iq_total = 0.0f;
   for (i = 0; i < 3; i++) {
       float Id_i, Iq_i;
       calc_single_phase_current(P_total/3, Q_total/3, V[i], &Id_i, &Iq_i);
       Id_total += Id_i; Iq_total += Iq_i;
   }
   // 取平均作为三相公共电流
   Id_common = Id_total / 3;
   Iq_common = Iq_total / 3;
   ```
2. **模式平滑切换**: 添加过渡状态，避免模式切换时电流跳变：
   ```c
   if (mode_transition) {
       // 使用一阶低通滤波平滑过渡
       Id = Id_old + (Id_new - Id_old) * transition_factor;
       transition_factor += Ts / transition_time;
   }
   ```

### 3.6 编译验证与代码质量
**现状**: 代码结构清晰，但需验证编译通过性和内存对齐。

**改进建议**:
1. **静态分析**: 添加 `static_assert` 确保结构体大小和对齐：
   ```c
   static_assert(sizeof(Gfl_SplitCurrentRef) == 52, 
                 "Gfl_SplitCurrentRef size mismatch");
   static_assert(offsetof(Gfl_SplitCurrentRef, Id) == 0,
                 "Id array misaligned");
   ```
2. **单元测试框架**: 为分相控制添加测试用例，验证边界条件：
   ```c
   void test_split_phase_symmetric(void) {
       Gfl_PhasePower power[3] = { ... };
       Gfl_SplitCurrentRef output;
       GflSplitPhase_CalcCurrent(NULL, power, CURRENT_MODE_SYMMETRIC, &output);
       assert(fabs(output.Id[0] - output.Id[1]) < 1e-6);
   }
   ```
3. **编译器优化**: 对关键路径函数使用 `__attribute__((optimize("O3")))` 和 `__attribute__((section(".fast_code")))` 将代码放入 ITCM 内存。

## 4. 问题解答

### 4.1 组件化程度
**评价**: 良好，但可进一步优化。
- **隔离性**: `gfl_split_phase` 模块与现有框架通过配置结构体交互，无直接依赖。
- **接口清晰**: 函数接口使用标准类型 `Gfl_PhasePower`, `Gfl_SplitCurrentRef`。
- **改进点**: 建议增加依赖注入回调，允许用户自定义算法。

### 4.2 扩展性
**评价**: 基本支持，但有硬编码限制。
- **数据结构**: `Gfl_SplitCurrentRef` 支持最多 6 桥臂，扩展性好。
- **算法限制**: 多个循环硬编码 `i < 3`，仅处理前三相，需改为 `i < num_phases`。
- **N相处理**: 缺少自动计算 N 相电流 (基尔霍夫定律)，需补充。

### 4.3 宏切换方式
**评价**: 合理，但需运行时验证。
- **宏定义**: `GFL_GRID_IS_ABC3`, `GFL_GRID_IS_ABCN4` 清晰易懂。
- **类型安全**: 使用枚举 `Gfl_GridType` 而非裸数字。
- **改进建议**: 增加初始化时 `grid_type` 与 `num_phases` 一致性检查。

### 4.4 限幅逻辑
**评价**: 基本正确，但顺序可优化。
- **两阶段设计**: 圆形限幅+矩形限幅概念正确。
- **顺序问题**: 应先矩形限幅后圆形限幅，避免矩形限幅破坏圆形边界。
- **耦合缺失**: 不对称模式下缺少相间耦合约束，可能导致总电流超限。

### 4.5 对称/不对称统一
**评价**: 底层函数统一，但对称模式计算可优化。
- **统一计算**: `calc_single_phase_current` 函数同时服务两种模式。
- **对称模式误差**: 使用平均电压而非各相实际电压，引入小误差。
- **改进建议**: 对称模式使用各相电压独立计算后取平均。

### 4.6 编译验证
**评价**: 结构可编译，但存在依赖问题。
- **本模块编译**: `gfl_split_phase.c` 无语法错误，可独立编译。
- **项目整体**: LSP 报告其他模块头文件缺失 (`fast_math.h`, `pi_ctrl.h`)，这与分相控制无关。
- **建议**: 运行完整构建验证链接是否成功。

## 5. 总结
分相功率控制和N桥臂扩展架构在组件化、扩展性方面设计良好，宏定义清晰，限幅逻辑基本正确。通过实施上述改进建议，可进一步提升代码的鲁棒性、实时性和可维护性。

**关键风险点**:
1. 不对称模式下 N 相电流未自动计算，可能导致基尔霍夫电流定律违反。
2. 竞争仲裁仅支持对称模式，不对称模式需扩展。
3. 限幅顺序可能引起二次越限，建议调整顺序。

**推荐优先级**:
1. **高**: 修复 N 相电流计算和循环硬编码问题。
2. **中**: 优化限幅顺序，添加抗饱和策略。
3. **低**: 完善编译验证和单元测试。

--- 
*文档生成时间: 2025-04-11*  
*审查专家: 嵌入式电力电子控制软件专家*