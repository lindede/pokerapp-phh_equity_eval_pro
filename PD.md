# 概要设计（PD）

## 1. 概述

### 1.1 文档目的

本文档描述 `pokerapp-phh_equity_eval` 项目现代化改造的系统架构、模块划分、关键技术方案及实施路径，作为 RD.md 需求文档的实现指导。

### 1.2 设计原则

1. **最小侵入**：仅修改与现代化相关的代码，不改变核心评估算法逻辑
2. **标准优先**：优先使用 C++20 标准库，避免引入新的第三方运行时依赖
3. **渐进替换**：按模块分批去除 Boost，每批完成后运行全量测试
4. **接口稳定**：保持 `pokerstove::` 命名空间及现有公开类/方法语义

### 1.3 术语

| 术语 | 含义 |
|------|------|
| peval | Poker Evaluation，牌型评估核心库 |
| penum | Poker Enumeration，权益枚举库 |
| CardSet | 64 位位集表示的扑克牌集合 |
| Equity | 一手牌在摊牌中获胜的概率份额 |

## 2. 系统架构

### 2.1 总体架构

```
┌─────────────────────────────────────────────────────────┐
│                    CLI Applications                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ ps-eval  │  │ ps-colex │  │ ps-lut   │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │             │             │                      │
│       └─────────────┼─────────────┘                      │
│                     │                                    │
│              ┌──────┴──────┐                            │
│              │  penum lib  │  权益枚举                    │
│              └──────┬──────┘                            │
│                     │                                    │
│              ┌──────┴──────┐                            │
│              │  peval lib  │  牌型评估                    │
│              └──────┬──────┘                            │
│                     │                                    │
│              ┌──────┴──────┐                            │
│              │ util (hdr)  │  组合/位运算/计时            │
│              └─────────────┘                            │
└─────────────────────────────────────────────────────────┘

可选：Python 绑定 (SWIG) ──► peval + penum
测试：GoogleTest ──► 各模块 *.test.cpp
```

### 2.2 依赖关系

```
util (INTERFACE)
  ↑
peval (STATIC/SHARED)
  ↑
penum (STATIC/SHARED)
  ↑
ps-eval / ps-colex / ps-lut (EXECUTABLE)
```

- **util**：仅头文件，提供 `combinations`、`timing`、`lastbit` 等
- **peval**：独立编译单元，不依赖 penum
- **penum**：依赖 peval，提供 `ShowdownEnumerator`
- **CLI**：依赖 penum + peval（ps-eval）或 peval（ps-colex、ps-lut）

### 2.3 分层说明

| 层次 | 职责 | 改造重点 |
|------|------|----------|
| 应用层 | 命令行解析、结果输出 | 去除 Boost.Program_options，使用 std::format 输出 |
| 枚举层 | 权益计算、分布管理 | 去除 Boost 字符串/格式化依赖 |
| 评估层 | 牌型评估、游戏工厂 | 去除 Boost operators/lexical_cast |
| 工具层 | 组合数学、计时 | 自实现 choose()，使用 std::chrono |

## 3. 模块设计

### 3.1 peval 模块

#### 3.1.1 核心类

| 类 | 文件 | 职责 |
|----|------|------|
| `CardSet` | CardSet.h/cpp | 位集牌面，str()/parse() |
| `PokerHandEvaluator` | PokerHandEvaluator.h/cpp | 评估器抽象基类 + 工厂 |
| `PokerHandEvaluation` | PokerEvaluation.h/cpp | 评估结果，比较运算符 |
| `EquityResult` | PokerHandEvaluator.h | 权益结果结构体 |
| `Suit` | Suit.h/cpp | 花色，显示格式 |
| 各游戏 Evaluator | *HandEvaluator.h/cpp | 具体游戏规则实现 |

#### 3.1.2 工厂模式

```cpp
// PokerHandEvaluator_Alloc.cpp
std::shared_ptr<PokerHandEvaluator> PokerHandEvaluator::alloc(const std::string& strid);
```

- 输入首字符映射游戏类型（h/O/o/r/s/e/...）
- 改造点：`boost::algorithm::to_lower` → 标准库实现

```cpp
inline void to_lower_inplace(std::string& s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
```

#### 3.1.3 运算符重载（替代 boost::operators）

`Suit` 和 `PokerHandEvaluation` 当前使用 `boost::operators` 提供比较运算符。改造为显式声明：

```cpp
// 示例：PokerHandEvaluation
bool operator==(const PokerHandEvaluation& rhs) const;
bool operator!=(const PokerHandEvaluation& rhs) const { return !(*this == rhs); }
bool operator<(const PokerHandEvaluation& rhs) const;
// ... 其余按需声明，或使用 C++20 三路比较 operator<=>
```

### 3.2 penum 模块

#### 3.2.1 核心类

| 类 | 文件 | 职责 |
|----|------|------|
| `CardDistribution` | CardDistribution.h/cpp | 手牌分布解析与权重 |
| `ShowdownEnumerator` | ShowdownEnumerator.h/cpp | 权益枚举入口 |
| `PartitionEnumerator` | PartitionEnumerator.h | 分区枚举 |

#### 3.2.2 权益计算流程

```
calculateEquity(dists, board, peval)
    │
    ├─► 遍历所有合法手牌组合（排除 dead cards）
    │
    ├─► peval->evaluateHand(hand, board) 逐手评估
    │
    └─► 累加 winShares / tieShares → vector<EquityResult>
```

`CardDistribution::parse()` 改造：
- `boost::split` → 按 `,` 手动 split 或使用 `std::string_view` 解析
- `boost::contains` → `str.find('=') != npos`
- `boost::lexical_cast<double>` → `std::stod` 或 `std::from_chars`

### 3.3 util 模块

#### 3.3.1 combinations

```cpp
// 替代 boost::math::binomial_coefficient
inline uint64_t choose(int n, int m) {
    if (m < 0 || m > n) return 0;
    if (m > n - m) m = n - m;  // 对称性优化
    uint64_t result = 1;
    for (int i = 0; i < m; ++i)
        result = result * (n - i) / (i + 1);
    return result;
}
```

#### 3.3.2 timing.hpp

```cpp
#include <chrono>

using clock = std::chrono::steady_clock;

inline double elapsed_seconds(clock::time_point start) {
    return std::chrono::duration<double>(clock::now() - start).count();
}
```

移除 `boost::timer::cpu_timer` 依赖。

### 3.4 CLI 模块

#### 3.4.1 轻量 CLI 解析器

新建 `src/lib/pokerstove/util/argparse.hpp`（或 `src/programs/common/cli.hpp`），提供最小子集：

```cpp
struct Option {
    std::string long_name;   // "game"
    char short_name;         // 'g'
    bool has_value;
    std::string default_value;
};

class ArgParser {
public:
    void add_option(const Option& opt);
    void add_positional(const std::string& name);
    bool parse(int argc, char** argv);
    std::string get(const std::string& name) const;
    std::vector<std::string> get_vector(const std::string& name) const;
    bool flag_set(const std::string& name) const;
};
```

设计约束：
- 支持 `--long value`、`-s value`、`--long=value` 形式
- 支持 positional 参数（如 `ps-eval AcAs Kh4d`）
- 不引入任何第三方库
- 三个 CLI 工具共用此解析器

#### 3.4.2 输出格式化

所有 `boost::format` 替换为 `std::format`：

```cpp
// Before
cout << boost::format("%s: %d\n") % it->first % it->second;

// After
std::cout << std::format("{}: {}\n", it->first, it->second);
```

## 4. Boost 替换详细方案

### 4.1 替换映射总表

| 原 Boost 用法 | 新实现 | 涉及文件 |
|---------------|--------|----------|
| `boost::program_options` | `ArgParser`（util） | ps-eval/main.cpp, ps-colex/main.cpp, ps-lut/main.cpp |
| `boost::format` | `std::format` | CardSet.cpp, PokerEvaluation.cpp, Suit.cpp, CardDistribution.cpp, ps-colex, ps-lut |
| `boost::lexical_cast<T>(s)` | `std::to_string` / `std::stod` / `std::from_chars` | 多处 |
| `boost::algorithm::to_lower` | `to_lower_inplace()` | PokerHandEvaluator_Alloc.cpp |
| `boost::algorithm::split` | `split()` util 函数 | CardDistribution.cpp |
| `boost::algorithm::contains` | `str.find(sub) != npos` | CardDistribution.cpp |
| `boost::operators` | 显式运算符 | Suit.h, PokerEvaluation.h |
| `boost::math::binomial_coefficient` | `choose()` | combinations.h |
| `boost::cstdint` | `<cstdint>` | PokerEvaluationTables.h |
| `boost::timer` / `boost::chrono` | `<chrono>` | timing.hpp |

### 4.2 新增 util 函数（header-only）

建议在 `src/lib/pokerstove/util/strings.hpp` 中集中提供：

```cpp
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace pokerstove::util {

void to_lower_inplace(std::string& s);

std::vector<std::string_view> split(std::string_view s, char delim);

bool contains(std::string_view s, std::string_view sub);

}  // namespace pokerstove::util
```

## 5. CMake 改造设计

### 5.1 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(pokerapp-phh_equity_eval VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_PYTHON "Build Python bindings" OFF)
option(BUILD_WHEEL "Build Python wheel" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# GoogleTest via FetchContent (when BUILD_TESTING)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(googletest ...)
    FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src/lib)

if(NOT BUILD_WHEEL)
    add_subdirectory(src/programs)
endif()

if(BUILD_PYTHON OR BUILD_WHEEL)
    add_subdirectory(src/lib/python)
endif()
```

关键变更：
- 移除全部 Boost `find_package` / `link_directories`
- GoogleTest 改用 FetchContent（可选保留 submodule 方式）
- 保留 `BUILD_PYTHON` / `BUILD_WHEEL` 选项

### 5.2 库目标 CMake 示例（peval）

```cmake
add_library(peval ${lib_sources})
add_library(pokerstove::peval ALIAS peval)

target_include_directories(peval
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(peval PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
)

if(BUILD_TESTING)
    add_executable(peval_tests ${test_sources})
    target_link_libraries(peval_tests PRIVATE peval GTest::gtest_main)
    add_test(NAME TestPeval COMMAND peval_tests)
endif()
```

### 5.3 util 作为 INTERFACE 库

```cmake
add_library(util INTERFACE)
add_library(pokerstove::util ALIAS util)

target_include_directories(util INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
```

## 6. 目录结构

改造后目录结构保持不变，新增少量 util 头文件：

```
pokerapp-phh_equity_eval_pro/
├── CMakeLists.txt              # 现代化顶层配置
├── RD.md                       # 需求文档
├── PD.md                       # 概要设计（本文档）
├── README.md                   # 更新构建说明
├── src/
│   ├── lib/
│   │   ├── CMakeLists.txt
│   │   └── pokerstove/
│   │       ├── peval/          # 牌型评估库
│   │       ├── penum/          # 权益枚举库
│   │       └── util/           # 工具头文件
│   │           ├── combinations.h
│   │           ├── strings.hpp     # [新增] 字符串工具
│   │           ├── argparse.hpp      # [新增] CLI 解析
│   │           ├── timing.hpp        # [改造] std::chrono
│   │           └── ...
│   ├── programs/
│   │   ├── ps-eval/
│   │   ├── ps-colex/
│   │   └── ps-lut/
│   └── ext/
│       └── googletest/         # 测试框架（可选改 FetchContent）
└── .github/workflows/
    └── cmake-build-test.yml    # 移除 Boost 安装
```

## 7. API 兼容性策略

### 7.1 保持不变

- 命名空间 `pokerstove::`
- 类名、方法签名（`PokerHandEvaluator::alloc`、`ShowdownEnumerator::calculateEquity` 等）
- `CardSet` 位集编码方式
- CLI 命令行参数名称与语义
- 库目标名称 `peval`、`penum`

### 7.2 允许变更

- 移除头文件中的 Boost include（使用者不应直接依赖 Boost 类型）
- `EquityResult::str()` 内部实现变更（输出格式保持一致）
- CMake 变量与构建流程变更
- 项目名称从 `pokerstove` 改为 `pokerapp-phh_equity_eval`（CMake project 名）

### 7.3 版本策略

- CMake `project()` 版本设为 `0.1.0`，标志现代化改造的首个发布
- 在 README 中注明与上游 PokerStove v1.2 的关系及差异

## 8. 测试策略

### 8.1 现有测试

| 测试目标 | 源文件模式 | 覆盖范围 |
|----------|------------|----------|
| peval_tests | peval/*.test.cpp | CardSet、Rank、Suit、各 Evaluator |
| penum_tests | penum/*.test.cpp | CardDistribution、ShowdownEnumerator |
| util_tests | util/*.test.cpp | combinations |

### 8.2 改造验证步骤

每完成一个 Boost 组件替换后：

1. 编译对应模块
2. 运行模块单元测试
3. 运行全量 `ctest`
4. 手动执行 CLI 示例命令对比输出

### 8.3 浮点比较

权益计算涉及浮点累加，测试断言使用容差：

```cpp
EXPECT_NEAR(result.equity, expected, 1e-6);
```

## 9. CI/CD 改造

### 9.1 GitHub Actions 变更

```yaml
# 移除
- sudo apt-get -y install libboost-all-dev

# 保留/调整
- sudo apt-get -y install swig pipx  # 仅 wheel 构建需要
- cmake -B build -DCMAKE_BUILD_TYPE=Release
- cmake --build build
- ctest --test-dir build
```

### 9.2 建议新增

- MinGW 构建矩阵（后续迭代）
- 编译器版本下限检查（GCC ≥ 11）

## 10. 实施计划

### 10.1 任务分解

```
Phase 1: 基础设施
├── 更新顶层 CMakeLists.txt（C++20、移除 Boost）
├── 新增 util/strings.hpp、util/argparse.hpp
├── util/combinations.h 自实现 choose()
└── util/timing.hpp 改用 std::chrono

Phase 2: peval 去 Boost
├── PokerEvaluationTables.h: boost/cstdint → cstdint
├── Suit.h/PokerEvaluation.h: 移除 boost::operators
├── 所有 lexical_cast/format 替换
└── 运行 peval_tests

Phase 3: penum 去 Boost
├── CardDistribution.cpp 字符串处理改造
├── PartitionEnumerator.h lexical_cast 替换
└── 运行 penum_tests

Phase 4: CLI 去 Boost
├── ps-eval/ps-colex/ps-lut 接入 ArgParser
├── 输出改用 std::format
└── 手动验证 CLI 行为

Phase 5: 收尾
├── 更新 README、CI
├── 清理残留 Boost 引用
└── 全量测试 + 文档
```

### 10.2 文件改造优先级

| 优先级 | 文件 | 原因 |
|--------|------|------|
| P0 | CMakeLists.txt（全部） | 构建基础 |
| P0 | combinations.h, timing.hpp | util 层被多处依赖 |
| P1 | CardSet.cpp, PokerEvaluation.cpp, Suit.cpp | peval 核心 |
| P1 | CardDistribution.cpp, PartitionEnumerator.h | penum 核心 |
| P2 | ps-eval/main.cpp 等 CLI | 依赖 ArgParser |
| P3 | UniversalHandEvaluator.h 等 | 次要 Boost 引用 |

## 11. 风险与对策

| 风险 | 概率 | 影响 | 对策 |
|------|------|------|------|
| GCC < 13 不支持 std::format | 中 | 编译失败 | 要求 GCC ≥ 13；或临时用 `fmt` 作为 PRIVATE 依赖并文档说明 |
| 自实现 choose() 溢出 | 低 | 错误结果 | 使用 uint64_t，边界检查，对比 Boost 结果写单元测试 |
| CLI 解析行为差异 | 中 | 用户脚本兼容 | 对照现有 program_options 行为写 CLI 测试用例 |
| Python 绑定编译失败 | 中 | 可选模块不可用 | 首版标记 BUILD_PYTHON=OFF 为默认 |

## 12. 附录

### 12.1 现有 Boost 引用文件清单

**库代码：**
- `src/lib/pokerstove/util/combinations.h`
- `src/lib/pokerstove/util/timing.hpp`
- `src/lib/pokerstove/peval/CardSet.cpp`
- `src/lib/pokerstove/peval/PokerEvaluation.cpp`
- `src/lib/pokerstove/peval/PokerEvaluation.h`
- `src/lib/pokerstove/peval/PokerEvaluationTables.h`
- `src/lib/pokerstove/peval/PokerHandEvaluator.h`
- `src/lib/pokerstove/peval/PokerHandEvaluator_Alloc.cpp`
- `src/lib/pokerstove/peval/Suit.h`
- `src/lib/pokerstove/peval/Suit.cpp`
- `src/lib/pokerstove/peval/UniversalHandEvaluator.h`
- `src/lib/pokerstove/penum/CardDistribution.cpp`
- `src/lib/pokerstove/penum/PartitionEnumerator.h`

**CLI：**
- `src/programs/ps-eval/main.cpp`
- `src/programs/ps-colex/main.cpp`
- `src/programs/ps-lut/main.cpp`

### 12.2 参考

- [PokerStove 上游仓库](https://github.com/andrewprock/pokerstove)
- [C++20 std::format](https://en.cppreference.com/w/cpp/utility/format/format)
- [CMake Modern Best Practices](https://cliutils.gitlab.io/modern-cmake/)
