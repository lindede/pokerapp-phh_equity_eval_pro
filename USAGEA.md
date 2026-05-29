# 命令行工具使用说明（USAGEA）

本文档基于当前源码分析 `ps-eval`、`ps-colex`、`ps-lut` 三个可执行程序的**命令语法、输入格式与限制**。构建产物位于 `build/bin/`。

---

## 1. 通用：牌面编码

三个工具凡涉及“具体牌面”时，均采用相同规则：

| 字段 | 合法字符 | 示例 |
|------|----------|------|
| 点数 | `2-9`、`T/t`、`J/j`、`Q/q`、`K/k`、`A/a` | `A`、`t` |
| 花色 | `c/C` clubs、`d/D` diamonds、`h/H` hearts、`s/S` spades | `s` |

- 每张牌占 **2 个字符**：`点数 + 花色`，如 `Ac`、`td`、`KS`
- 多张牌**连续书写**，中间无空格：`AcAs`、`5c8s9h`
- 大小写通常均可（由 `Rank` / `Suit` 解析）

---

## 2. ps-eval — 权益计算

### 2.1 功能

枚举所有合法发牌组合，计算各玩家摊牌权益（Equity）。

### 2.2 命令格式

```text
ps-eval [选项] <手牌1> [<手牌2> ...]
```

### 2.3 选项

| 长选项 | 短选项 | 类型 | 默认值 | 说明 |
|--------|--------|------|--------|------|
| `--help` | `-?` | 标志 | | 显示帮助 |
| `--game` | `-g` | 字符串 | `h` | 游戏类型 |
| `--board` | `-b` | 字符串 | 无 | 公共牌（显式牌面） |
| `--hand` | `-h` | 字符串 | | 手牌（也可用位置参数） |
| `--quiet` | `-q` | 标志 | | 不输出结果 |

**注意：** 帮助是 `-?`，不是 `-h`；`-h` / `--hand` 表示手牌。

### 2.4 手牌输入（CardDistribution::parse）

每个位置参数（或 `--hand`）解析为一个玩家的**手牌分布**，支持以下形式：

#### （1）具体手牌

```bash
ps-eval AcAs Kh4d
ps-eval acas kh4d -b 5c8s9h
```

- 字符串长度必须为**偶数**（每张牌 2 字符）
- 同一手牌内**不能重复**同一张牌

#### （2）逗号分隔的多组合 + 可选权重

```bash
ps-eval "AcAs=0.5,KhKd=0.5" QhQd
ps-eval AcAs,KhKd 9c9d
```

- 逗号 `,` 分隔多个组合
- `手牌=权重`，权重为浮点数；省略时权重为 `1.0`
- `=` 后不能为空

#### （3）随机手牌

```bash
ps-eval AcAs .
```

- 单个 `.` 表示该玩家为**随机手牌**（内部为空 `CardSet`）

### 2.5 玩家数量规则

| 传入手牌数 | 行为 |
|------------|------|
| 1 | 自动追加第 2 个玩家为**全随机手牌**（`fill(handSize)`） |
| ≥ 2 | 按给定玩家数计算，不自动追加 |

```bash
ps-eval AcAs              # 玩家1=AcAs，玩家2=随机
ps-eval AcAs Kh4d         # 两人具体手牌
```

### 2.6 游戏类型（--game）

首字符（及部分完整串）决定评估器，常见值：

| 代码 | 游戏 |
|------|------|
| `h` | Hold'em |
| `O` | Omaha High |
| `o` / `omaha` | Omaha High |
| `o/8` / `o8` / `omaha/8` | Omaha Hi/Lo |
| `o5` / `o6` / `o5/8` / `o6/8` | Omaha 变体 |
| `r` | Razz |
| `s` | Stud |
| `e` | Stud/8 |
| `q` | Stud Hi/Lo（无 8 qualifier） |
| `d` / `D` | Draw High |
| `l` | Lowball A-5 |
| `k` | Kansas City Lowball 2-7 |
| `t` | Triple Draw 2-7 |
| `T` | Triple Draw A-5 |
| `b` | Badugi |
| `3` | Three-card poker |

游戏字符串会先转小写再匹配（`T` 与 `t` 在低牌类游戏中含义不同，见上表）。

### 2.7 输出格式

```text
The hand AcAs has 97.6768 % equity (959.000000 8.000000 0.000000 0.000000)
```

括号内为 `winShares tieShares equity equity2`。

### 2.8 不支持的手牌写法（重要）

`ps-eval` **没有**范围 DSL，以下写法均**非法**：

| 示例 | 原因 |
|------|------|
| `22+` | 对子范围语法 |
| `AKo` / `AKs` | 缩写非对子 |
| `QQ+` | 加号范围 |
| `22` | 仅点数、缺花色（Hold'em 需 4 字符如 `2c2d`） |
| `76s` | 同花连张缩写 |

**案例：`ps-eval AcAs 22+`**

- `AcAs` 合法
- `22+` 无法按 `rank+suit` 解析 → 该玩家分布为空
- 枚举时访问空分布 → 旧版崩溃：`CardDistribution::operator: bounds error`
- 新版会提前报错并提示查看本文档

**替代做法：** 用逗号列出具体组合并赋权，例如：

```bash
ps-eval AcAs "2c2d=1,3c3d=1,4c4d=1,5c5d=1,6c6d=1,7c7d=1,8c8d=1,9c9d=1,TcTd=1,JcJd=1,QcQd=1,KcKd=1"
```

### 2.9 其他限制

1. 手牌张数须符合所选 `--game`（Hold'em 2 张、Omaha 4 张等）
2. 各玩家手牌与公共牌之间**不得重复**同一张牌
3. `--board` 可为空（翻前）；非空时须为合法显式牌面
4. 公共牌未给全时，由 `ShowdownEnumerator` 枚举剩余公共牌
5. 多玩家 + 大分布 + 未锁定公共牌时，计算量可能很大

### 2.10 示例

```bash
ps-eval AcAs Kh4d --board 5c8s9h
ps-eval --game l 7c5c4c3c2c
ps-eval --game O AcKcQdJh 8h7h6h5h -b Tc9c2d
ps-eval -q AcAs Kh4d
```

---

## 3. ps-colex — Colex 索引查看

### 3.1 功能

枚举 `num-cards` 张牌的所有组合，输出**规范花色**下的手牌及其 colex 索引，或仅输出**点数组合**的 rank colex。

### 3.2 命令格式

```text
ps-colex [选项]
```

无位置参数；通过选项控制。

### 3.3 选项

| 长选项 | 短选项 | 类型 | 默认值 | 说明 |
|--------|--------|------|--------|------|
| `--help` | `-?` | 标志 | | 显示帮助 |
| `--num-cards` | `-n` | 整数 | `2` | 每张手的牌数 |
| `--ranks` | | 标志 | 关 | 只按点数输出 rank colex |

### 3.4 输出

**默认（规范花色）：**

```text
2c3c: 123
...
```

**`--ranks`：**

```text
23: 456
...
```

### 3.5 限制

- `num-cards` 须在 `1..52` 合理范围内；过大时输出量爆炸（如 `n=5` 即 C(52,5)=2,598,960 行）
- 不接受手牌字符串参数；仅枚举全 deck 组合
- 与 `ps-eval` 无直接关系，不做权益计算

### 3.6 示例

```bash
ps-colex -n 2
ps-colex --num-cards 3 --ranks
```

---

## 4. ps-lut — 查找表生成

### 4.1 功能

对规范化的 pocket × board 组合调用评估器，打印评估结果与 colex 索引，用于生成/查看 LUT。

### 4.2 命令格式

```text
ps-lut [选项]
```

### 4.3 选项

| 长选项 | 短选项 | 类型 | 默认值 | 说明 |
|--------|--------|------|--------|------|
| `--help` | `-?` | 标志 | | 显示帮助 |
| `--pocket-count` | `-p` | 整数 | `2` | 底牌张数 |
| `--board-count` | `-b` | 整数 | `3` | 公共牌张数 |
| `--game` | `-g` | 字符串 | `O` | 评估游戏 |
| `--ranks` | | 标志 | 关 | 按点数评估（`evaluateRanks`） |

### 4.4 输出示例

```text
2c3c, 4c5c6c: [  12,  345] -> ... [   123456]
```

格式：`pocket, board: [pocketColex, boardColex] -> 评估字符串 [code]`

### 4.5 行为说明

- 默认使用**规范花色**（`SUIT_CANONICAL`）；`--ranks` 时仅用点数（`RANK`）
- 非 `--ranks` 模式下，pocket 与 board **相交**的组合会跳过
- `--game` 与 `ps-eval` 使用同一套 `PokerHandEvaluator::alloc` 规则

### 4.6 限制

- 输出规模为 `|pockets| × |boards|`，`p`/`b` 增大时非常快
- 不接受具体手牌参数；只遍历规范集合
- 默认 `game=O`（Omaha），与 `ps-eval` 默认 `h` 不同

### 4.7 示例

```bash
ps-lut -p 2 -b 3 -g h
ps-lut --pocket-count 4 --board-count 3 --game O
ps-lut -p 2 -b 3 --ranks
```

---

## 5. 三工具对比

| 工具 | 主要用途 | 手牌参数 | 默认游戏 | 典型用户 |
|------|----------|----------|----------|----------|
| `ps-eval` | 权益计算 | 支持（位置参数） | Hold'em `h` | 分析对局胜率 |
| `ps-colex` | Colex 索引 | 不支持 | — | 表/索引研究 |
| `ps-lut` | 评估 LUT | 不支持 | Omaha `O` | 批量评估导出 |

---

## 6. 常见错误速查（ps-eval）

| 现象 | 原因 | 处理 |
|------|------|------|
| `invalid hand for player N` | 手牌串非法（如 `22+`） | 改用显式牌面，见 §2.8 |
| `bounds error`（旧版） | 解析失败后仍枚举 | 升级后会有明确错误；检查手牌格式 |
| `unknown game` | `--game` 无效 | 见 §2.6 |
| `invalid board` | 公共牌格式错误 | 使用 `5c8s9h` 形式 |
| `duplicate board card` | 公共牌重复 | 检查输入 |
| 权益为 0% / 100% 异常 | 牌张数与游戏不匹配 | 确认 `--game` 与手牌长度 |

---

## 7. 参考源码

| 行为 | 文件 |
|------|------|
| ps-eval 入口 | `src/programs/ps-eval/main.cpp` |
| 手牌解析 | `src/lib/pokerstove/penum/CardDistribution.cpp` |
| 权益枚举 | `src/lib/pokerstove/penum/ShowdownEnumerator.cpp` |
| 游戏工厂 | `src/lib/pokerstove/peval/PokerHandEvaluator_Alloc.cpp` |
| CLI 解析 | `src/lib/pokerstove/util/argparse.hpp` |
