#!/bin/bash

# Pikafish HardCode - macOS Apple Silicon (M1/M2/M3) Optimized Build
# 针对 Apple Silicon 完全优化的编译脚本

set -e

# 颜色
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================"
echo "  Pikafish HardCode for macOS Apple Silicon"
echo "========================================"
echo

# 进入源码目录
cd "$(dirname "$0")/src"

echo "Cleaning previous build..."
make clean > /dev/null 2>&1 || true

echo

echo "${BLUE}Build Configuration:${NC}"
echo "  Target:     apple-silicon"
echo "  Arch:       arm64 (ARMv8.2-A)"
echo "  SIMD:       NEON (128-bit)"
echo "  DotProd:    YES (ARMv8.2-A dot product)"
echo "  LTO:        Full"
echo "  Threads:    $(sysctl -n hw.ncpu) cores"
echo "  Opt Level:  -O3"
echo

# 编译 - 使用所有可用核心
echo "Compiling..."
make build ARCH=apple-silicon -j$(sysctl -n hw.ncpu)

echo
echo "${GREEN}========================================"
echo "  Build Complete!"
echo "========================================${NC}"
echo

# 显示二进制信息
echo "Binary info:"
ls -lh ./pikafish
file ./pikafish
echo

# 快速测试
echo "Running quick test..."
echo "uci" | ./pikafish | grep -E "(Pikafish|id name|uciok)"
echo

echo "Usage:"
echo "  ./src/pikafish           # Run engine"
echo "  ./src/pikafish bench     # Run benchmark"
echo
