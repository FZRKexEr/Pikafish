# Pikafish HardCode 版本实施计划

## 项目概述

将 Pikafish（中国象棋引擎）从 NNUE（高效神经网络评估）替换为硬编码的评估函数，同时保持其他部分不变。

---

## 一、现状分析

### 1.1 Pikafish 代码结构

```
src/
├── main.cpp              # 引擎入口
├── engine.cpp/h          # 引擎核心
├── evaluate.cpp/h        # 评估函数入口 ⭐关键
├── position.cpp/h        # 局面表示
├── search.cpp/h          # 搜索算法
├── uci.cpp/h             # UCI协议
├── ucioption.cpp/h       # 配置选项
├── nnue/                 # NNUE 目录 ⭐关键
│   ├── network.cpp/h     # 神经网络实现
│   ├── nnue_common.h     # 通用定义
│   ├── nnue_accumulator.h
│   ├── nnue_feature_transformer.h
│   ├── features/         # 特征提取
│   └── layers/           # 神经网络层
└── ...
```

### 1.2 NNUE 评估流程

```
evaluate() [evaluate.cpp]
    └── 调用 networks.big.evaluate() [nnue/network.cpp]
        └── 特征转换 + 神经网络前向传播
            └── 返回评估值 (Value 类型)
```

### 1.3 关键接口

```cpp
// evaluate.h - 评估函数声明
Value evaluate(const NNUE::Networks& networks,
               const Position& pos,
               Eval::NNUE::AccumulatorStack& accumulators,
               Eval::NNUE::AccumulatorCaches& caches,
               int optimism);

// nnue/network.h - 网络评估
class Network {
    Value evaluate(const Position& pos, ...);
};
```

---

## 二、实施策略

### 2.1 核心思路

**替换策略**: 创建一个硬编码评估函数，替换 NNUE 网络评估，但保持接口完全一致。

```
原始流程:
 evaluate() -> networks.big.evaluate() -> NNUE网络计算

HardCode流程:
 evaluate() -> hardcoded_evaluate() -> 硬编码计算
```

### 2.2 需要修改的文件

1. **新增文件** (创建 HardCode 评估模块):
   - `src/hardcode_eval.h` - 硬编码评估头文件
   - `src/hardcode_eval.cpp` - 硬编码评估实现

2. **修改文件** (适配接口):
   - `src/evaluate.cpp` - 修改 evaluate() 函数，调用硬编码评估
   - `src/evaluate.h` - 可能需要移除 NNUE 依赖
   - `src/engine.cpp` - 可能不需要修改，取决于引擎初始化
   - `src/Makefile` - 添加新文件的编译规则

3. **可选修改** (清理 NNUE 依赖):
   - 可以考虑保留但不使用 `src/nnue/` 目录，或者完全移除
   - 为简单起见，建议保留但不编译 NNUE 文件

---

## 三、详细实施步骤

### Phase 1: 代码分析与准备 (Day 1)

#### 任务 1.1: 复制官方代码
```bash
# 复制官方代码到工作目录
cp -r /Users/xinpeng/Desktop/playground/pikafish-official/* /Users/xinpeng/Desktop/playground/Pikafish/
```

#### 任务 1.2: 理解评估接口
- 阅读 `src/evaluate.cpp` 和 `src/evaluate.h`
- 理解 `Value evaluate(...)` 函数的实现
- 确定需要替换的核心逻辑

#### 任务 1.3: 理解 Position 类
- 阅读 `src/position.h`
- 了解如何获取：
  - 当前轮到哪方走棋
  - 每个位置上的棋子
  - 王的位置
  - 棋子列表

### Phase 2: 硬编码评估函数设计 (Day 2)

#### 任务 2.1: 定义评估要素

中国象棋硬编码评估应包含：

1. **子力价值** (Material Value):
   ```cpp
   // 基础子力价值表 (以兵 = 100 为基准)
   Pawn:   100
   Advisor: 200    // 士
   Bishop:  200    // 象
   Knight:  400    // 马
   Cannon:  450    // 炮
   Rook:    900    // 车
   King:    20000  // 将/帅 (极高价值)
   ```

2. **位置价值表** (Piece-Square Tables, PST):
   ```cpp
   // 每个棋子在不同位置的价值加成
   // 例如：马在中心位置更好
   // 兵过河后价值增加
   // 车在开放线路更好
   ```

3. **机动性评估** (Mobility):
   ```cpp
   // 计算每个棋子的可移动位置数
   //  mobility_bonus = count_legal_moves(piece)
   ```

4. **基本战术评估**:
   ```cpp
   // 将军检测
   // 简单威胁检测
   // 王的安全性
   ```

#### 任务 2.2: 设计 HardCodeEval 类

```cpp
// src/hardcode_eval.h
#ifndef HARDCODE_EVAL_H
#define HARDCODE_EVAL_H

#include "types.h"
#include "position.h"

namespace HardCodeEval {

// 评估结果类型
struct EvalResult {
    Value score;  // 从当前行棋方的角度
};

// 主评估函数
Value evaluate(const Position& pos);

// 内部评估组件
namespace Internal {
    // 子力价值评估
    Value evaluate_material(const Position& pos);

    // 位置价值评估 (Piece-Square Tables)
    Value evaluate_pst(const Position& pos);

    // 机动性评估
    Value evaluate_mobility(const Position& pos);

    // 王安全性评估
    Value evaluate_king_safety(const Position& pos);

    // 基础战术评估
    Value evaluate_tactics(const Position& pos);
}

} // namespace HardCodeEval

#endif // HARDCODE_EVAL_H
```

### Phase 3: 实现硬编码评估 (Day 3-4)

#### 任务 3.1: 实现基础评估表

```cpp
// src/hardcode_eval.cpp - 基础数据定义

#include "hardcode_eval.h"
#include "bitboard.h"

namespace HardCodeEval {

// ==================== 子力基础价值 ====================
// 以兵 = 100 为基准
enum PieceValue : int {
    PAWN_VALUE = 100,
    ADVISOR_VALUE = 200,   // 士
    BISHOP_VALUE = 200,    // 象
    KNIGHT_VALUE = 400,    // 马
    CANNON_VALUE = 450,    // 炮
    ROOK_VALUE = 900,      // 车
    KING_VALUE = 20000     // 将/帅
};

// 子力价值表 (按 Piece 类型索引)
const int PIECE_VALUES[PIECE_TYPE_NB] = {
    0,                    // NO_PIECE_TYPE
    PAWN_VALUE,           // PAWN
    ADVISOR_VALUE,        // ADVISOR
    BISHOP_VALUE,         // BISHOP
    KNIGHT_VALUE,         // KNIGHT
    CANNON_VALUE,         // CANNON
    ROOK_VALUE,           // ROOK
    KING_VALUE            // KING
};

// ==================== 位置价值表 (Piece-Square Tables) ====================
// 中国象棋棋盘: 9列 x 10行
// 红方在底部 (0-4行), 黑方在顶部 (5-9行)

// 兵的位置价值表 (红方视角，需要镜像给黑方)
// 鼓励兵过河和向前推进
const int PST_PAWN[90] = {
    // 第0行 (黑方底线)
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 第1行
    0,  0,  0,  0,  0,  0,  0,  0,  0,
    // 第2行 (楚河汉界 - 黑方河界)
    0,  0,  0, 10, 10, 10,  0,  0,  0,
    // 第3行 (黑方过河后)
    5,  5, 10, 20, 30, 20, 10,  5,  5,
    // 第4行
    10, 10, 20, 30, 40, 30, 20, 10, 10,
    // 第5行 (楚河汉界 - 红方河界)
    10, 10, 20, 30, 40, 30, 20, 10, 10,
    // 第6行 (红方过河后)
    5,  5, 10, 20, 30, 20, 10,  5,  5,
    // 第7行
    0,  0,  0, 10, 10, 10,  0,  0,  0,
    // 第8行 (红方底线)
    0,  0,  0,  0,  0,  0,  0,  0,  0,
};

// 车的位置价值表 - 喜欢开放线路和中心
const int PST_ROOK[90] = {
    // ... (类似定义)
};

// 马的位置价值表 - 喜欢中心，避免边线和马脚
const int PST_KNIGHT[90] = {
    // ... (类似定义)
};

// 炮的位置价值表
const int PST_CANNON[90] = {
    // ... (类似定义)
};

// 士/象的位置价值表 - 主要限制在九宫内
const int PST_ADVISOR[90] = {
    // ... (类似定义)
};

const int PST_BISHOP[90] = {
    // ... (类似定义)
};

// 将/帅的位置价值表 - 安全第一，喜欢中心
const int PST_KING[90] = {
    // ... (类似定义)
};

// 位置价值表指针数组
const int* PIECE_SQUARE_TABLES[PIECE_TYPE_NB] = {
    nullptr,        // NO_PIECE_TYPE
    PST_PAWN,       // PAWN
    PST_ADVISOR,    // ADVISOR
    PST_BISHOP,     // BISHOP
    PST_KNIGHT,     // KNIGHT
    PST_CANNON,     // CANNON
    PST_ROOK,       // ROOK
    PST_KING        // KING
};

} // namespace HardCodeEval
```

#### 任务 3.2: 实现评估函数

```cpp
// src/hardcode_eval.cpp - 评估函数实现

#include "hardcode_eval.h"
#include "bitboard.h"
#include "position.h"

namespace HardCodeEval {

// ==================== 辅助函数 ====================

// 获取棋子在棋盘上的位置索引 (0-89)
inline int square_index(Square s) {
    return static_cast<int>(s);
}

// 镜像位置 (用于黑方)
// 中国象棋棋盘: 红方在下方(0-4行), 黑方在上方(5-9行)
// 镜像公式: mirrored = 8 - rank, file 不变
inline int mirror_square(int sq) {
    int file = sq % 9;
    int rank = sq / 9;
    int mirrored_rank = 9 - rank;
    return mirrored_rank * 9 + file;
}

// ==================== 子力价值评估 ====================

Value Internal::evaluate_material(const Position& pos) {
    int material = 0;

    // 遍历所有棋子类型
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        // 红方棋子
        Piece red_piece = make_piece(WHITE, pt);
        int red_count = popcount(pos.pieces(red_piece));
        material += red_count * PIECE_VALUES[pt];

        // 黑方棋子
        Piece black_piece = make_piece(BLACK, pt);
        int black_count = popcount(pos.pieces(black_piece));
        material -= black_count * PIECE_VALUES[pt];
    }

    return Value(material);
}

// ==================== 位置价值评估 ====================

Value Internal::evaluate_pst(const Position& pos) {
    int pst_score = 0;

    // 遍历所有棋子类型
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        const int* pst_table = PIECE_SQUARE_TABLES[pt];
        if (!pst_table) continue;

        // 红方棋子
        Bitboard red_pieces = pos.pieces(WHITE, pt);
        while (red_pieces) {
            Square sq = pop_lsb(red_pieces);
            int idx = square_index(sq);
            pst_score += pst_table[idx];
        }

        // 黑方棋子 (使用镜像位置)
        Bitboard black_pieces = pos.pieces(BLACK, pt);
        while (black_pieces) {
            Square sq = pop_lsb(black_pieces);
            int idx = mirror_square(square_index(sq));
            pst_score -= pst_table[idx];
        }
    }

    return Value(pst_score);
}

// ==================== 机动性评估 ====================

Value Internal::evaluate_mobility(const Position& pos) {
    // 简化实现：计算各方可移动位置数
    // 实际实现需要生成所有合法移动并计数

    int mobility = 0;

    // 这里简化处理，实际应该调用 movegen
    // mobility = count_legal_moves(WHITE) - count_legal_moves(BLACK);

    // 临时简单实现：基于棋子数量和类型估算
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        int mobility_per_piece = 0;
        switch (pt) {
            case PAWN:   mobility_per_piece = 2; break;
            case ADVISOR: mobility_per_piece = 4; break;
            case BISHOP: mobility_per_piece = 4; break;
            case KNIGHT: mobility_per_piece = 8; break;
            case CANNON: mobility_per_piece = 10; break;
            case ROOK:   mobility_per_piece = 14; break;
            case KING:   mobility_per_piece = 4; break;
            default: break;
        }

        mobility += popcount(pos.pieces(WHITE, pt)) * mobility_per_piece;
        mobility -= popcount(pos.pieces(BLACK, pt)) * mobility_per_piece;
    }

    // 机动性价值因子 (可以调整)
    const int MOBILITY_FACTOR = 5;
    return Value(mobility * MOBILITY_FACTOR);
}

// ==================== 王安全性评估 ====================

Value Internal::evaluate_king_safety(const Position& pos) {
    // 简化实现：评估王的安全性
    int safety = 0;

    // 获取王的位置
    Square red_king = pos.square(WHITE, KING);
    Square black_king = pos.square(BLACK, KING);

    // 九宫安全评估
    // 王在中心更安全
    // 检查是否有士保护
    // 检查是否暴露在开放线路

    // 简化：基于王的位置给予评分
    // 王在中心(4,1)或(4,8)位置最好

    // 红方王安全性
    int red_king_file = red_king % 9;  // 0-8
    int red_king_rank = red_king / 9;  // 0-9

    // 理想位置是九宫中心 (4, 8) 附近
    int red_center_bonus = 20 - (abs(red_king_file - 4) + abs(red_king_rank - 8)) * 5;
    safety += std::max(0, red_center_bonus);

    // 黑方王安全性
    int black_king_file = black_king % 9;
    int black_king_rank = black_king / 9;

    // 理想位置是九宫中心 (4, 1) 附近
    int black_center_bonus = 20 - (abs(black_king_file - 4) + abs(black_king_rank - 1)) * 5;
    safety -= std::max(0, black_center_bonus);

    return Value(safety);
}

// ==================== 战术评估 ====================

Value Internal::evaluate_tactics(const Position& pos) {
    // 简化实现：基础战术检测
    int tactics = 0;

    // 检测将军
    if (pos.checkers()) {
        // 被将军方处于劣势
        if (pos.side_to_move() == WHITE) {
            tactics -= 50;  // 红方被将军
        } else {
            tactics += 50;  // 黑方被将军
        }
    }

    // 这里可以添加更多战术检测：
    // - 吃子威胁
    // - 牵制
    // - 双将等

    return Value(tactics);
}

// ==================== 主评估函数 ====================

Value evaluate(const Position& pos) {
    // 1. 子力评估
    Value material = Internal::evaluate_material(pos);

    // 2. 位置评估
    Value pst = Internal::evaluate_pst(pos);

    // 3. 机动性评估
    Value mobility = Internal::evaluate_mobility(pos);

    // 4. 王安全性评估
    Value king_safety = Internal::evaluate_king_safety(pos);

    // 5. 战术评估
    Value tactics = Internal::evaluate_tactics(pos);

    // 总和
    int total = material + pst + mobility + king_safety + tactics;

    // 确保在有效范围内
    return Value(std::clamp(total, VALUE_MATED_IN_MAX_PLY + 1, VALUE_MATE_IN_MAX_PLY - 1));
}

} // namespace HardCodeEval
```

### Phase 3: 集成到 Pikafish (Day 5)

#### 任务 3.1: 修改 evaluate.cpp/h

```cpp
// evaluate.h - 修改后
#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "position.h"

// 前向声明 (不再需要 NNUE)
namespace NNUE { class Networks; }
namespace Eval::NNUE {
    class AccumulatorStack;
    struct AccumulatorCaches;
}

namespace Eval {

// 硬编码评估开关
#define USE_HARDCODED_EVAL 1

// 简化的评估函数声明
Value evaluate(const Position& pos);

// 调试用的跟踪评估 (可选实现)
std::string trace(const Position& pos);

// 初始化评估模块
void init();

} // namespace Eval

#endif // EVALUATE_H
```

```cpp
// evaluate.cpp - 修改后
#include "evaluate.h"
#include "position.h"
#include "hardcode_eval.h"  // 包含硬编码评估

namespace Eval {

// 初始化评估模块
void init() {
    // 硬编码评估不需要特殊初始化
    // 但可以进行一些预计算优化
}

// 主评估函数
Value evaluate(const Position& pos) {
    // 调用硬编码评估
    return HardCodeEval::evaluate(pos);
}

// 调试跟踪 (简化版)
std::string trace(const Position& pos) {
    std::stringstream ss;
    ss << "HardCode Evaluation Trace\n";
    ss << "========================\n";

    // 可以添加各个评估组件的详细分数
    // ...

    ss << "Final evaluation: " << evaluate(pos) << "\n";
    return ss.str();
}

} // namespace Eval
```

#### 任务 3.2: 修改引擎初始化

```cpp
// engine.cpp - 修改 initialize() 函数

void Engine::initialize() {
    // 原有初始化代码...

    // 移除或注释掉 NNUE 网络加载
    // networks = std::make_unique<NNUE::Networks>(...);

    // 硬编码评估初始化
    Eval::init();
}

// 修改搜索中的评估调用
// 原来: evaluate(networks, pos, accumulators, caches, optimism)
// 改为: Eval::evaluate(pos)
```

#### 任务 3.3: 修改 Makefile

```makefile
# Makefile - 添加 hardcode_eval 编译规则

# 源文件列表中添加
SRCS = ... \
       hardcode_eval.cpp \
       ...

# 头文件依赖
hardcode_eval.o: hardcode_eval.cpp hardcode_eval.h position.h types.h bitboard.h
	$(CXX) $(CXXFLAGS) -c hardcode_eval.cpp -o $@
```

### Phase 4: 测试与优化 (Day 6-7)

#### 任务 4.1: 编译测试

```bash
# 清理并重新编译
cd /Users/xinpeng/Desktop/playground/Pikafish/src
make clean
make -j$(nproc)

# 检查编译错误并修复
```

#### 任务 4.2: 功能测试

```bash
# 启动引擎
./pikafish

# 基本 UCI 测试
uci
isready
position startpos
go depth 10
quit

# 检查是否有崩溃或错误输出
```

#### 任务 4.3: 评估准确性测试

```cpp
// 创建测试脚本 test_eval.cpp
// 测试几个已知局面的评估值是否合理

void test_positions() {
    // 测试1: 初始局面 (应该接近0)
    // 测试2: 红方多一个兵 (红方略优)
    // 测试3: 红方少一个车 (黑方大优)
    // 测试4: 将军局面
}
```

#### 任务 4.4: 性能优化

1. **预计算优化**:
   - 预计算所有位置价值表
   - 预计算子力价值

2. **循环优化**:
   - 使用位运算加速
   - 减少函数调用开销

3. **缓存友好**:
   - 优化数据结构访问模式

---

## 四、任务清单 (Task Breakdown)

### 团队分工建议

创建 3 个并行任务组：

#### 小组 A: 代码分析与接口设计
- **任务 A1**: 深入分析 Pikafish 评估接口
- **任务 A2**: 设计 HardCodeEval 接口
- **任务 A3**: 确定 Position 类的使用方式
- **交付物**: 接口设计文档 + 头文件

#### 小组 B: 硬编码评估实现
- **任务 B1**: 实现子力价值评估
- **任务 B2**: 实现位置价值表 (PST)
- **任务 B3**: 实现机动性评估
- **任务 B4**: 实现王安全性评估
- **任务 B5**: 实现战术评估
- **交付物**: hardcode_eval.cpp/h

#### 小组 C: 集成与测试
- **任务 C1**: 修改 evaluate.cpp/h
- **任务 C2**: 修改引擎初始化代码
- **任务 C3**: 修改 Makefile
- **任务 C4**: 编译测试
- **任务 C5**: 功能测试
- **交付物**: 可运行的 HardCode Pikafish

### 依赖关系图

```
A1, A2, A3 (接口设计)
    ↓
B1-B5 (评估实现) ← 并行 → C1-C3 (集成准备)
    ↓                        ↓
    └──────────→ C4-C5 (编译测试)
```

### 时间表

| 天数 | 小组 A | 小组 B | 小组 C |
|-----|--------|--------|--------|
| Day 1 | 代码分析 | - | - |
| Day 2 | 接口设计 | 开始实现 | - |
| Day 3 | - | 继续实现 | 准备集成 |
| Day 4 | - | 完成实现 | 开始集成 |
| Day 5 | - | - | 完成集成 |
| Day 6 | - | - | 测试修复 |
| Day 7 | - | - | 优化完成 |

---

## 五、风险评估与应对

### 风险 1: Position 类接口复杂
- **风险**: 不熟悉 Position 类的所有方法
- **应对**: 仔细阅读 position.h，必要时询问原作者

### 风险 2: 硬编码评估强度不足
- **风险**: 硬编码评估比 NNUE 弱很多
- **应对**: 接受这是预期行为，重点是功能正确

### 风险 3: 编译错误难以解决
- **风险**: C++ 模板和类型错误
- **应对**: 仔细检查头文件包含，使用编译器错误信息

### 风险 4: 修改范围过大
- **风险**: 需要修改太多文件
- **应对**: 保持 NNUE 代码存在，只修改调用点

---

## 六、验收标准

### 功能验收
- [ ] 引擎能够正常编译通过
- [ ] UCI 协议正常工作
- [ ] 能够接受局面并搜索
- [ ] 能够返回评估值
- [ ] 不会崩溃或产生段错误

### 代码验收
- [ ] 代码结构清晰，有适当注释
- [ ] 硬编码评估参数易于调整
- [ ] 不破坏原有代码结构（尽量）
- [ ] Makefile 正确配置

### 性能验收
- [ ] 搜索速度在可接受范围内
- [ ] 评估函数执行快速（< 1ms/局面）
- [ ] 内存使用合理

---

## 七、附录

### A. 参考资料

1. **Pikafish 官方仓库**: https://github.com/official-pikafish/Pikafish
2. **Stockfish 文档**: https://stockfishchess.org/docs/
3. **中国象棋规则**: 了解各棋子走法和特殊规则

### B. 中国象棋棋子价值参考

| 棋子 | 英文 | 价值(兵=100) | 备注 |
|-----|------|-------------|------|
| 兵/卒 | Pawn | 100 | 过河后价值增加 |
| 士/仕 | Advisor | 200 | 只能在九宫内移动 |
| 象/相 | Bishop | 200 | 不能过河 |
| 马/傌 | Knight | 400 | 有"马脚"限制 |
| 炮/炮 | Cannon | 450 | 需要炮架 |
| 车/俥 | Rook | 900 | 最强力的棋子 |
| 帅/将 | King | 20000 | 不能被吃 |

### C. 棋盘坐标系统

```
   0  1  2  3  4  5  6  7  8   (File/列)
0  .  .  .  .  .  .  .  .  .
1  .  .  .  .  .  .  .  .  .
2  .  .  .  .  .  .  .  .  .   <- 楚河汉界 (黑方)
3  .  .  .  .  .  .  .  .  .
4  .  .  .  .  .  .  .  .  .
5  .  .  .  .  .  .  .  .  .
6  .  .  .  .  .  .  .  .  .   <- 楚河汉界 (红方)
7  .  .  .  .  .  .  .  .  .
8  .  .  .  .  .  .  .  .  .
9  .  .  .  .  .  .  .  .  .
(Rank/行)

红方在下方 (Rank 7-9)
黑方在上方 (Rank 0-2)
```

---

**文档版本**: 1.0
**创建日期**: 2026-02-08
**作者**: Claude Code
**项目**: Pikafish HardCode Edition
