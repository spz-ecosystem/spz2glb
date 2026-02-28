# 测试示例

本目录包含 SPZ 转 GLB 的测试示例文件。

## 示例文件

### 1. 基础测试模型

- `triangle.spz` - 简单的三角形测试模型
- `cube.spz` - 立方体测试模型
- `sphere.spz` - 球体测试模型（高密度点云）

### 2. 压力测试模型

- `10k_points.spz` - 1 万高斯点
- `100k_points.spz` - 10 万高斯点
- `1m_points.spz` - 100 万高斯点

## 运行测试

### 快速测试

```bash
# 下载测试模型（示例）
wget https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.0/test-models.zip
unzip test-models.zip

# 运行转换和验证
./build/spz2glb triangle.spz triangle.glb
./build/spz_verify all triangle.spz triangle.glb
```

### 批量测试

```bash
# 转换所有测试模型
for file in *.spz; do
    echo "Testing: $file"
    ./build/spz2glb "$file" "${file%.spz}.glb"
    ./build/spz_verify all "$file" "${file%.spz}.glb"
    echo ""
done
```

### 自动化测试脚本

```bash
#!/bin/bash
# test_all.sh

set -e

echo "=== SPZ2GLB Test Suite ==="
echo ""

# Test 1: Basic conversion
echo "[Test 1] Basic conversion..."
./build/spz2glb triangle.spz triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ Conversion successful"
else
    echo "✗ Conversion failed"
    exit 1
fi
echo ""

# Test 2: Three-layer verification
echo "[Test 2] Three-layer verification..."
./build/spz_verify all triangle.spz triangle.glb
if [ $? -eq 0 ]; then
    echo "✓ All layers passed"
else
    echo "✗ Verification failed"
    exit 1
fi
echo ""

# Test 3: Batch conversion
echo "[Test 3] Batch conversion..."
count=0
for file in *.spz; do
    if [ -f "$file" ]; then
        ./build/spz2glb "$file" "${file%.spz}.glb"
        ((count++))
    fi
done
echo "✓ Converted $count files"
echo ""

echo "=== All Tests Passed ==="
```

## 测试检查清单

- [ ] **Layer 1**: GLB 结构验证
  - [ ] Magic number 正确 (0x46546C67)
  - [ ] Version 为 2
  - [ ] extensionsUsed 包含必需扩展
  - [ ] buffers 配置正确

- [ ] **Layer 2**: 二进制无损验证
  - [ ] MD5 哈希匹配
  - [ ] 数据完整性确认

- [ ] **Layer 3**: 解码一致性验证
  - [ ] GLB 结构完整
  - [ ] 扩展引用正确

## 预期输出

```
=== SPZ2GLB Test Suite ===

[Test 1] Basic conversion...
[INFO] Loading SPZ: triangle.spz
[INFO] SPZ version: 2
[INFO] Num points: 1000
[INFO] SH degree: 2
[INFO] SPZ size (raw compressed): 0.5 MB
[INFO] Creating glTF Asset with KHR extensions
[INFO] Exporting GLB...
[SUCCESS] GLB exported: triangle.glb
[INFO] GLB size: 0.6 MB
✓ Conversion successful

[Test 2] Three-layer verification...
Layer 1: GLB Structure Validation
  ✓ Magic number: 0x46546C67 ("glTF")
  ✓ Version: 2
  ✓ extensionsUsed contains KHR_gaussian_splatting
  ✓ extensionsUsed contains KHR_gaussian_splatting_compression_spz_2
  ✓ buffers configuration correct
  ✓ Compression stream mode (attributes empty)
  [PASS] Layer 1 validation passed

Layer 2: Lossless Binary Validation
  ✓ Original SPZ MD5: abc123...
  ✓ Extracted data MD5: abc123...
  ✓ MD5 match confirmed
  [PASS] Layer 2 validation passed

Layer 3: Decode Consistency Validation
  ✓ GLB structure valid
  ✓ Extension integrity check passed
  [PASS] Layer 3 validation passed

[SUCCESS] All 3 layers validation passed!
✓ All layers passed

[Test 3] Batch conversion...
✓ Converted 3 files

=== All Tests Passed ===
```

## 故障排除

### 问题：找不到测试文件

**解决方案**：
```bash
# 检查文件是否存在
ls -la *.spz

# 如果文件不存在，下载测试数据集
wget https://example.com/test-data.zip
unzip test-data.zip
```

### 问题：验证失败

**解决方案**：
```bash
# 单独运行每层验证，定位问题
./build/spz_verify layer1 output.glb
./build/spz_verify layer2 input.spz output.glb
./build/spz_verify layer3 input.spz output.glb

# 检查错误信息，根据提示修复
```

### 问题：转换后文件过大

**说明**：
- SPZ 压缩率约 10x
- 如果 GLB 文件过大，检查 SPZ 源文件质量
- 可以使用更低的 SH degree 或更少的点

## 性能基准

| 模型 | SPZ 大小 | GLB 大小 | 转换时间 | 验证时间 |
|------|---------|---------|---------|---------|
| triangle | 0.5 MB | 0.6 MB | <0.1s | <0.1s |
| cube | 1 MB | 1.2 MB | <0.1s | <0.1s |
| 100k_points | 15 MB | 16 MB | 0.5s | 0.3s |
| 1m_points | 150 MB | 160 MB | 3s | 2s |

*测试环境：Intel i7-12700K, 32GB RAM, NVMe SSD*

## 贡献测试用例

欢迎提交测试用例！请确保：

1. SPZ 文件符合 SPZ_2 规范
2. 提供预期输出
3. 包含验证步骤
4. 文档清晰

提交至：https://github.com/spz-ecosystem/spz2glb/issues
