/*
  Hardcoded Evaluation for Pikafish
  Copyright (C) 2026

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
*/

#ifndef HARDCODE_EVAL_H
#define HARDCODE_EVAL_H

#include "types.h"

namespace Stockfish {

class Position;

namespace HardCodeEval {

// 主评估函数 - 从当前行棋方的角度返回评估值
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

} // namespace Stockfish

#endif // HARDCODE_EVAL_H
