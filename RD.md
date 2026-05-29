# 需求文档（RD）

## 1. 项目背景

本项目基于开源项目 [PokerStove](https://github.com/andrewprock/pokerstove) 进行改造，原项目是一个高度优化的 C++ 扑克牌型评估与权益（Equity）计算库。当前代码基线存在以下问题：

- 语言标准为 C++14，未利用现代 C++ 特性
- 强依赖 Boost（program_options、format、lexical_cast、algorithm、operators、math、chrono 等），安装与跨平台构建成本高
- CMake 配置较旧（3.5），未采用现代 target-based 写法
- 部分 API 仍保留 Boost 时代的设计痕迹

改造目标是在**保持核心评估与权益计算能力不变**的前提下，使工程符合现代 C++ 规范，消除运行时第三方依赖，降低构建与集成成本。

## 2. 项目目标

| 目标 | 说明 |
|------|------|
| 现代化 | 升级至 C++20，使用标准库替代 Boost 能力 |
| 零运行时依赖 | 核心库（peval、penum、util）不依赖任何第三方运行时库 |
| 可维护性 | 采用现代 CMake 实践，清晰的模块边界与编译选项 |
| 功能保持 | 14 种扑克变体的牌型评估与权益枚举行为与改造前一致 |
| 可验证 | 现有 GoogleTest 单元测试全部通过，CLI 工具行为兼容 |

## 3. 功能范围

### 3.1 核心库（必须保留）

#### peval — 牌型评估库

- `CardSet`：52 张牌的位集表示及位运算
- `Card` / `Rank` / `Suit`：底层牌面抽象
- 低级评估器：高牌、A-5 低牌、2-7 低牌、Badugi 等
- 游戏评估器：Hold'em、Omaha、Stud、Razz、Draw 等 14 种变体
- `PokerHandEvaluator::alloc()` 工厂接口
- Colex 索引工具

#### penum — 权益枚举库

- `CardDistribution`：带权重的手牌分布
- `ShowdownEnumerator::calculateEquity()`：多玩家权益计算
- `PartitionEnumerator` 等枚举辅助工具

#### util — 工具头文件

- `combinations`：组合枚举
- `timing.hpp`：性能计时（测试/开发用途）
- 其他位运算辅助

### 3.2 命令行工具（必须保留）

| 工具 | 功能 |
|------|------|
| `ps-eval` | 牌型评估与权益计算演示 |
| `ps-colex` | Colex 索引查看 |
| `ps-lut` | 查找表生成/查看 |

CLI 参数语义与现有行为保持一致（`--game`、`--board`、`--hand`、`--quiet` 等）。

### 3.3 可选模块（首版不强制）

| 模块 | 说明 | 首版策略 |
|------|------|----------|
| Python 绑定（SWIG） | 通过 `BUILD_PYTHON` / wheel 构建 | 保留 CMake 选项，但不作为首版验收阻塞项 |
| Android / win32 安装包 | 历史遗留分发物 | 不在改造范围内 |

## 4. 非功能需求

### 4.1 性能

- 改造后核心评估路径性能不得显著退化（目标：与改造前相比偏差 < 5%，以现有测试基准为准）
- 不因引入不必要的抽象或动态分配而降低热路径效率

### 4.2 兼容性

- **ABI/API**：允许因移除 Boost 产生少量源码级不兼容（如移除 Boost 类型暴露），但公开接口语义保持一致
- `std::shared_ptr` 作为 `PokerHandEvaluator` 工厂返回类型（已在 v1.2 迁移，保持不变）
- 命名空间保持 `pokerstove::`

### 4.3 可移植性

- 支持 GCC / G++（Linux、MinGW-w64）
- 首版以 Linux + MinGW 为主要验证平台
- MSVC 支持作为后续迭代目标，不阻塞首版

### 4.4 代码质量

- 开启合理编译警告（`-Wall -Wextra`，MSVC 等价选项）
- 消除已知的 deprecated 用法
- 新代码遵循 C++ Core Guidelines 基本原则（RAII、const 正确性、避免裸 new/delete）

## 5. 技术要求

### 5.1 构建与语言

| 项 | 要求 |
|----|------|
| 项目名称 | `pokerapp-phh_equity_eval` |
| 构建系统 | CMake ≥ 3.20 |
| 语言标准 | C++20 |
| 编译器 | GCC / G++ ≥ 11（MinGW 或 Linux） |
| 核心依赖 | 零第三方运行时依赖 |
| 测试依赖 | GoogleTest（FetchContent 或现有 submodule，仅测试期链接） |
| CLI 依赖 | 零第三方库（不使用 Boost.Program_options） |

### 5.2 CMake 基础配置

```cmake
cmake_minimum_required(VERSION 3.20)
project(pokerapp-phh_equity_eval VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### 5.3 CMake 现代化要求

- 使用 `target_*` 接口（`target_include_directories`、`target_link_libraries`、`target_compile_features`）替代全局 `include_directories`
- 库目标：`peval`、`penum`（util 为 INTERFACE 库）
- 可执行目标：`ps-eval`、`ps-colex`、`ps-lut`
- 测试目标：`peval_tests`、`penum_tests`、`util_tests`
- 编译选项通过 `target_compile_options` 或 `CMAKE_CXX_FLAGS` 按配置（Debug/Release）管理
- 移除 `link_directories` 与全局 Boost 查找

### 5.4 Boost 移除清单

以下 Boost 组件必须用 C++20 标准库或项目内轻量实现替代：

| Boost 组件 | 使用位置 | 替代方案 |
|------------|----------|----------|
| `boost::program_options` | ps-eval、ps-colex、ps-lut | 轻量 CLI 解析（标准库实现） |
| `boost::format` | CardSet、PokerEvaluation、Suit 等 | `std::format` |
| `boost::lexical_cast` | 多处字符串/数值转换 | `std::to_string` / `std::from_chars` / `std::stod` |
| `boost::algorithm::string` | 字符串 split、to_lower、contains | 标准库字符串操作或小型 util 函数 |
| `boost::operators` | Suit、PokerEvaluation | 手动运算符重载 |
| `boost::math::binomial_coefficient` | combinations.h | 项目内 `choose(n,k)` 实现 |
| `boost::cstdint` | PokerEvaluationTables.h | `<cstdint>` |
| `boost::chrono` / `boost::timer` | timing.hpp | `<chrono>` |

## 6. 验收标准

### 6.1 构建验收

- [ ] 在无 Boost 环境下，`cmake -B build && cmake --build build` 成功
- [ ] 生成 `peval`、`penum` 静态/动态库及三个 CLI 工具
- [ ] `ctest` 全部测试通过

### 6.2 功能验收

- [ ] `ps-eval` 现有示例命令输出与改造前一致（允许浮点末位差异）
- [ ] 14 种游戏类型的 `PokerHandEvaluator::alloc()` 均可正常实例化
- [ ] `ShowdownEnumerator::calculateEquity()` 多玩家场景结果正确

### 6.3 依赖验收

- [ ] 核心库链接产物不依赖 Boost 共享库/静态库
- [ ] `ldd`（Linux）/ `dumpbin /dependents`（Windows）验证无 Boost 符号

### 6.4 CI 验收

- [ ] GitHub Actions 工作流移除 `libboost-all-dev` 安装步骤
- [ ] CI 构建与测试绿灯

## 7. 范围外（Out of Scope）

- 新增扑克变体或评估算法
- 多线程权益计算优化
- 重写 Python 绑定为 pybind11
- 项目重命名导致的公开 API 大规模变更
- 性能极致优化（SIMD、LUT 重构等）

## 8. 里程碑

| 阶段 | 内容 | 交付物 |
|------|------|--------|
| M1 | CMake 现代化 + Boost 依赖分析 | 可编译骨架、PD.md |
| M2 | peval / penum / util 去 Boost 化 | 库目标零 Boost 依赖 |
| M3 | CLI 工具去 Boost 化 | ps-eval / ps-colex / ps-lut |
| M4 | 测试与 CI 更新 | 全量测试通过、CI 绿灯 |
| M5 | 文档更新 | README、构建说明 |

## 9. 风险与约束

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| `std::format` 编译器支持 | MinGW 旧版本可能不支持 | 要求 GCC ≥ 13 或提供 `{fmt}` 作为可选编译开关（首版优先纯 std） |
| 浮点权益结果微小差异 | 回归测试失败 | 使用容差比较（epsilon） |
| Python 绑定仍依赖 SWIG | 构建复杂度保留 | 首版标记为可选，不阻塞核心交付 |
