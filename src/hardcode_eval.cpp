/*
  Hardcoded Evaluation for Pikafish
  Copyright (C) 2026

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*/

#include "hardcode_eval.h"
#include "bitboard.h"
#include "position.h"
#include <algorithm>
#include <sstream>

namespace Stockfish {

namespace HardCodeEval {

// ==================== 子力基础价值 ====================
// 使用 types.h 中定义的 PieceValue

// ==================== 位置价值表 (Piece-Square Tables) ====================
// 中国象棋棋盘: 9列 x 10行
// 红方在底部 (0-4行), 黑方在顶部 (5-9行)

// 兵的位置价值表 (红方视角)
// 鼓励兵过河和向前推进
const int PST_PAWN[SQUARE_NB] = {
    // 第0行 (黑方底线)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第1行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第2行 (楚河汉界 - 黑方河界)
    0,   0,   0,  10,  10,  10,   0,   0,   0,
    // 第3行 (黑方过河后)
    5,   5,  10,  20,  30,  20,  10,   5,   5,
    // 第4行
    10,  10,  20,  30,  40,  30,  20,  10,  10,
    // 第5行 (楚河汉界 - 红方河界)
    10,  10,  20,  30,  40,  30,  20,  10,  10,
    // 第6行 (红方过河后)
    5,   5,  10,  20,  30,  20,  10,   5,   5,
    // 第7行
    0,   0,   0,  10,  10,  10,   0,   0,   0,
    // 第8行 (红方底线)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第9行 (红方底线，兵的起始位置)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
};

// 车的位置价值表 - 喜欢开放线路和中心
const int PST_ROOK[SQUARE_NB] = {
    // 第0行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第1行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第2行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第3行
    15,  20,  20,  25,  25,  25,  20,  20,  15,
    // 第4行
    15,  20,  20,  25,  30,  25,  20,  20,  15,
    // 第5行
    15,  20,  20,  25,  30,  25,  20,  20,  15,
    // 第6行
    15,  20,  20,  25,  25,  25,  20,  20,  15,
    // 第7行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第8行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第9行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
};

// 马的位置价值表 - 喜欢中心，避免边线
const int PST_KNIGHT[SQUARE_NB] = {
    // 第0行
    0,   5,   5,  10,  10,  10,   5,   5,   0,
    // 第1行
    5,  10,  15,  20,  20,  20,  15,  10,   5,
    // 第2行
    5,  15,  20,  25,  25,  25,  20,  15,   5,
    // 第3行
    10,  20,  25,  30,  35,  30,  25,  20,  10,
    // 第4行
    10,  20,  25,  35,  40,  35,  25,  20,  10,
    // 第5行
    10,  20,  25,  35,  40,  35,  25,  20,  10,
    // 第6行
    10,  20,  25,  30,  35,  30,  25,  20,  10,
    // 第7行
    5,  15,  20,  25,  25,  25,  20,  15,   5,
    // 第8行
    5,  10,  15,  20,  20,  20,  15,  10,   5,
    // 第9行
    0,   5,   5,  10,  10,  10,   5,   5,   0,
};

// 炮的位置价值表
const int PST_CANNON[SQUARE_NB] = {
    // 第0行
    10,  10,  10,  15,  15,  15,  10,  10,  10,
    // 第1行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第2行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第3行
    15,  20,  20,  25,  30,  25,  20,  20,  15,
    // 第4行
    15,  20,  25,  30,  35,  30,  25,  20,  15,
    // 第5行
    15,  20,  25,  30,  35,  30,  25,  20,  15,
    // 第6行
    15,  20,  20,  25,  30,  25,  20,  20,  15,
    // 第7行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第8行
    10,  15,  15,  20,  20,  20,  15,  15,  10,
    // 第9行
    10,  10,  10,  15,  15,  15,  10,  10,  10,
};

// 士的位置价值表 - 限制在九宫内
const int PST_ADVISOR[SQUARE_NB] = {
    // 第0行 (黑方九宫)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第1行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第2行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第3行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第4行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第5行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第6行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第7行 (红方九宫开始)
    0,   0,   0,  10,   0,  10,   0,   0,   0,
    // 第8行
    0,   0,   0,   0,  20,   0,   0,   0,   0,
    // 第9行 (红方底线)
    0,   0,   0,  10,   0,  10,   0,   0,   0,
};

// 象的位置价值表 - 不能过河
const int PST_BISHOP[SQUARE_NB] = {
    // 第0行 (黑方)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第1行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第2行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第3行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第4行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第5行 (楚河汉界)
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第6行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第7行 (红方)
    0,   0,  10,   0,   0,   0,  10,   0,   0,
    // 第8行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第9行 (红方底线)
    0,   0,  10,   0,   0,   0,  10,   0,   0,
};

// 将/帅的位置价值表
const int PST_KING[SQUARE_NB] = {
    // 第0行 (黑方九宫)
    0,   0,   0,  10,  20,  10,   0,   0,   0,
    // 第1行
    0,   0,   0,  20,  30,  20,   0,   0,   0,
    // 第2行
    0,   0,   0,  10,  20,  10,   0,   0,   0,
    // 第3行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第4行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第5行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第6行
    0,   0,   0,   0,   0,   0,   0,   0,   0,
    // 第7行 (红方九宫)
    0,   0,   0,  10,  20,  10,   0,   0,   0,
    // 第8行
    0,   0,   0,  20,  30,  20,   0,   0,   0,
    // 第9行 (红方底线)
    0,   0,   0,  10,  20,  10,   0,   0,   0,
};

// 位置价值表指针数组
const int* PIECE_SQUARE_TABLES[PIECE_TYPE_NB] = {
    nullptr,        // NO_PIECE_TYPE
    PST_ROOK,       // ROOK
    PST_ADVISOR,    // ADVISOR
    PST_CANNON,     // CANNON
    PST_PAWN,       // PAWN
    PST_KNIGHT,     // KNIGHT
    PST_BISHOP,     // BISHOP
    PST_KING        // KING
};

// ==================== 辅助函数 ====================

// 镜像位置 (用于黑方)
// 中国象棋棋盘: 红方在下方(0-4行), 黑方在上方(5-9行)
inline int mirror_square(int sq) {
    int file = sq % FILE_NB;
    int rank = sq / FILE_NB;
    int mirrored_rank = (RANK_NB - 1) - rank;
    return mirrored_rank * FILE_NB + file;
}

// ==================== 子力价值评估 ====================

Value Internal::evaluate_material(const Position& pos) {
    int material = 0;

    // 遍历所有棋子类型
    for (PieceType pt = ROOK; pt < PIECE_TYPE_NB; ++pt) {
        // 红方棋子 (WHITE)
        int red_count = popcount(pos.pieces(WHITE, pt));
        material += red_count * PieceValue[pt];

        // 黑方棋子 (BLACK)
        int black_count = popcount(pos.pieces(BLACK, pt));
        material -= black_count * PieceValue[pt];
    }

    return Value(material);
}

// ==================== 位置价值评估 ====================

Value Internal::evaluate_pst(const Position& pos) {
    int pst_score = 0;

    // 遍历所有棋子类型
    for (PieceType pt = ROOK; pt < PIECE_TYPE_NB; ++pt) {
        const int* pst_table = PIECE_SQUARE_TABLES[pt];
        if (!pst_table) continue;

        // 红方棋子
        Bitboard red_pieces = pos.pieces(WHITE, pt);
        while (red_pieces) {
            Square sq = pop_lsb(red_pieces);
            pst_score += pst_table[sq];
        }

        // 黑方棋子 (使用镜像位置)
        Bitboard black_pieces = pos.pieces(BLACK, pt);
        while (black_pieces) {
            Square sq = pop_lsb(black_pieces);
            int mirrored_sq = mirror_square(sq);
            pst_score -= pst_table[mirrored_sq];
        }
    }

    return Value(pst_score);
}

// ==================== 机动性评估 ====================

Value Internal::evaluate_mobility(const Position& pos) {
    // 简化实现：基于棋子数量估算机动性
    int mobility = 0;

    for (PieceType pt = ROOK; pt < PIECE_TYPE_NB; ++pt) {
        int mobility_per_piece = 0;
        switch (pt) {
            case ROOK:    mobility_per_piece = 14; break;
            case KNIGHT:  mobility_per_piece = 8;  break;
            case CANNON:  mobility_per_piece = 10; break;
            case PAWN:    mobility_per_piece = 2;  break;
            case ADVISOR: mobility_per_piece = 4;  break;
            case BISHOP:  mobility_per_piece = 4;  break;
            case KING:    mobility_per_piece = 4;  break;
            default:      mobility_per_piece = 0;  break;
        }

        mobility += popcount(pos.pieces(WHITE, pt)) * mobility_per_piece;
        mobility -= popcount(pos.pieces(BLACK, pt)) * mobility_per_piece;
    }

    // 机动性价值因子
    const int MOBILITY_FACTOR = 3;
    return Value(mobility * MOBILITY_FACTOR);
}

// ==================== 王安全性评估 ====================

Value Internal::evaluate_king_safety(const Position& pos) {
    int safety = 0;

    // 获取王的位置
    Square red_king = pos.king_square(WHITE);
    Square black_king = pos.king_square(BLACK);

    // 红方王安全性 (红方九宫在第7-9行)
    int red_file = red_king % FILE_NB;
    int red_rank = red_king / FILE_NB;

    // 理想位置是九宫中心 (E, 第8行)
    int red_file_bonus = 2 - std::abs(red_file - FILE_E);
    int red_rank_bonus = 1 - std::abs(red_rank - 8);
    int red_safety = (red_file_bonus + red_rank_bonus) * 10;
    safety += red_safety;

    // 检查红方是否有士保护王
    Bitboard red_advisors = pos.pieces(WHITE, ADVISOR);
    if (popcount(red_advisors) >= 1)
        safety += 15;

    // 黑方王安全性 (黑方九宫在第0-2行)
    int black_file = black_king % FILE_NB;
    int black_rank = black_king / FILE_NB;

    // 理想位置是九宫中心 (E, 第1行)
    int black_file_bonus = 2 - std::abs(black_file - FILE_E);
    int black_rank_bonus = 1 - std::abs(black_rank - 1);
    int black_safety = (black_file_bonus + black_rank_bonus) * 10;
    safety -= black_safety;

    // 检查黑方是否有士保护王
    Bitboard black_advisors = pos.pieces(BLACK, ADVISOR);
    if (popcount(black_advisors) >= 1)
        safety -= 15;

    return Value(safety);
}

// ==================== 战术评估 ====================

Value Internal::evaluate_tactics(const Position& pos) {
    int tactics = 0;

    // 检测将军
    Bitboard checkers_bb = pos.checkers();
    if (checkers_bb) {
        // 被将军方处于劣势
        if (pos.side_to_move() == WHITE) {
            tactics -= 50;  // 红方被将军
        } else {
            tactics += 50;  // 黑方被将军
        }
    }

    // 简单的子力威胁评估
    // 可以扩展为检测被攻击的无保护棋子等

    return Value(tactics);
}

// ==================== 主评估函数 ====================

Value evaluate(const Position& pos) {
    // 从红方角度计算评估值
    int score = 0;

    // 1. 子力评估
    score += Internal::evaluate_material(pos);

    // 2. 位置评估
    score += Internal::evaluate_pst(pos);

    // 3. 机动性评估
    score += Internal::evaluate_mobility(pos);

    // 4. 王安全性评估
    score += Internal::evaluate_king_safety(pos);

    // 5. 战术评估
    score += Internal::evaluate_tactics(pos);

    // 根据当前行棋方调整分数
    // 如果是黑方行棋，需要反转分数
    if (pos.side_to_move() == BLACK) {
        score = -score;
    }

    // 确保在有效范围内
    return Value(std::clamp(score, VALUE_MATED_IN_MAX_PLY + 1, VALUE_MATE_IN_MAX_PLY - 1));
}

} // namespace HardCodeEval

} // namespace Stockfish
