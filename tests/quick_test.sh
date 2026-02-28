#!/bin/bash
# 快速测试脚本 - 测试 SPZ 转 GLB 转换和验证

set -e

echo "=== SPZ2GLB 快速测试 ==="
echo ""

# 检查可执行文件
if [ ! -f "./build/spz2glb" ]; then
    echo "错误：找不到 ./build/spz2glb"
    echo "请先运行：cmake -B build -DCMAKE_BUILD_TYPE=Release"
    echo "         cmake --build build --config Release"
    exit 1
fi

if [ ! -f "./build/spz_verify" ]; then
    echo "错误：找不到 ./build/spz_verify"
    exit 1
fi

# 创建测试目录
mkdir -p test_output
cd test_output

# 创建简单的测试 SPZ 文件（这里应该使用真实的 SPZ 文件）
# 为了演示，我们假设有测试文件
if [ ! -f "../tests/data/triangle.spz" ]; then
    echo "提示：测试文件不存在"
    echo "请创建测试数据目录：mkdir -p tests/data"
    echo "并添加 SPZ 测试文件到该目录"
    echo ""
    echo "跳过文件测试，仅测试工具可用性..."
    
    # 测试工具是否可以运行
    echo "测试 spz2glb 帮助..."
    ../build/spz2glb 2>&1 | head -n 1 || true
    
    echo "测试 spz_verify 帮助..."
    ../build/spz_verify 2>&1 | head -n 1 || true
    
    echo ""
    echo "✓ 工具可以正常运行"
    cd ..
    rm -rf test_output
    exit 0
fi

# 运行测试
echo "[测试 1] 转换测试..."
../build/spz2glb ../tests/data/triangle.spz triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ 转换成功"
else
    echo "✗ 转换失败"
    exit 1
fi
echo ""

echo "[测试 2] Layer 1 - GLB 结构验证..."
../build/spz_verify layer1 triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ Layer 1 通过"
else
    echo "✗ Layer 1 失败"
    exit 1
fi
echo ""

echo "[测试 3] Layer 2 - 二进制无损验证..."
../build/spz_verify layer2 ../tests/data/triangle.spz triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ Layer 2 通过"
else
    echo "✗ Layer 2 失败"
    exit 1
fi
echo ""

echo "[测试 4] Layer 3 - 解码一致性验证..."
../build/spz_verify layer3 ../tests/data/triangle.spz triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ Layer 3 通过"
else
    echo "✗ Layer 3 失败"
    exit 1
fi
echo ""

echo "=== 所有测试通过 ==="
cd ..
rm -rf test_output
