# 三层验证

本文档详细介绍 spz2glb 的三层验证系统。

## 📋 概述

spz2glb 采用三层验证系统确保转换质量：

```
Layer 1: GLB 结构验证     - 验证输出文件结构正确
Layer 2: MD5 无损验证     - 验证压缩数据完整无损
Layer 3: 解码一致性验证   - 验证解码数据完全一致
```

## 🔍 Layer 1: GLB 结构验证

### 目的

验证输出的 GLB 文件符合 glTF 2.0 规范。

### 检查项

- ✅ **GLB Header**
  - 魔数 (magic): 0x46546C67 ("glTF")
  - 版本 (version): 2
  - 长度 (length): 文件总长度

- ✅ **Chunks**
  - JSON chunk: 类型 0x4E4F534A ("JSON")
  - BIN chunk: 类型 0x004E4942 ("BIN\0")

- ✅ **JSON 结构**
  - 有效的 JSON 格式
  - 必需的 glTF 属性（asset, buffers, bufferViews, accessors）
  - KHR_gaussian_splatting_compression_spz_2 扩展

- ✅ **BIN 数据**
  - 长度对齐（4 字节边界）
  - 数据完整性

### 使用方法

```bash
# 仅验证 Layer 1
spz_verify layer1 output.glb

# 输出示例
✓ GLB 结构验证通过
- Header: 12 bytes
- Chunks: 2 (JSON, BIN)
- JSON: 有效
- BIN: 对齐
```

### 错误示例

```bash
# 魔数错误
✗ GLB 结构验证失败
错误：无效的 GLB 魔数 (期望：0x46546C67, 实际：0x00000000)

# JSON 错误
✗ GLB 结构验证失败
错误：无效的 JSON 格式
```

## 🔐 Layer 2: MD5 无损验证

### 目的

验证 SPZ 压缩数据在转换过程中保持完全一致（无损）。

### 原理

```
SPZ 文件              GLB 文件
  |                     |
  v                     v
提取压缩数据         提取压缩数据
  |                     |
  v                     v
计算 MD5 哈希        计算 MD5 哈希
  |                     |
  +--------+------------+
           |
           v
      比较 MD5 值
```

### 检查项

- ✅ **SPZ 压缩数据 MD5**
  - 从 SPZ 文件提取压缩流
  - 计算 MD5 哈希值

- ✅ **GLB 压缩数据 MD5**
  - 从 GLB buffer 提取压缩数据
  - 计算 MD5 哈希值

- ✅ **MD5 匹配**
  - 比较两个 MD5 值
  - 必须完全相同（无损）

### 使用方法

```bash
# 仅验证 Layer 2
spz_verify layer2 input.spz output.glb

# 输出示例
✓ MD5 验证通过
- SPZ MD5: abc123def456...
- GLB MD5: abc123def456...
- 匹配：是 (16 bytes)
```

### 技术细节

```cpp
// MD5 哈希计算
Md5Hash hash;
hash.update(data, size);
hash.finalize();

// 16 字节哈希值
uint8_t digest[16];
```

### 错误示例

```bash
# MD5 不匹配
✗ MD5 验证失败
- SPZ MD5: abc123...
- GLB MD5: def456...
- 匹配：否
错误：压缩数据损坏
```

## 🔬 Layer 3: 解码一致性验证

### 目的

验证 SPZ 和 GLB 解码后的原始数据完全一致。

### 原理

```
SPZ 文件              GLB 文件
  |                     |
  v                     v
解压缩              解压缩
  |                     |
  v                     v
解码原始数据         解码原始数据
  |                     |
  v                     v
比较字节流
```

### 检查项

- ✅ **解码点数**
  - SPZ 解码点数
  - GLB 解码点数
  - 必须相同

- ✅ **解码数据大小**
  - SPZ 解码数据大小
  - GLB 解码数据大小
  - 必须相同

- ✅ **逐字节比较**
  - 每个属性（position, color, normal, etc.）
  - 每个高斯球的所有字段
  - 必须完全一致

### 使用方法

```bash
# 仅验证 Layer 3
spz_verify layer3 input.spz output.glb

# 输出示例
✓ 解码验证通过
- 解码 SPZ: 1000000 points
- 解码 GLB: 1000000 points
- 数据大小：50.5 MB
- 匹配：是 (逐字节)
```

### 验证的属性

- ✅ `position` (位置) - 3 个 float
- ✅ `color` (颜色) - 3 个 uint8
- ✅ `normal` (法线) - 4 个 uint8 (四元数)
- ✅ `scaling` (缩放) - 3 个 float
- ✅ `rotation` (旋转) - 4 个 float (可选)
- ✅ `f_dc_0, f_dc_1, f_dc_2` (球谐系数 DC) - 3 个 float
- ✅ `f_rest` (球谐系数剩余) - 45 个 float (可选)
- ✅ `opacity` (不透明度) - 1 个 float

### 错误示例

```bash
# 点数不匹配
✗ 解码验证失败
- 解码 SPZ: 1000000 points
- 解码 GLB: 999999 points
错误：点数不匹配

# 数据不匹配
✗ 解码验证失败
错误：在第 12345 字节发现差异
- SPZ: 0x42
- GLB: 0x43
```

## 🎯 组合验证模式

### lossless (无损验证)

```bash
# Layer 1 + Layer 2
spz_verify lossless input.spz output.glb
```

**适用场景**:
- 快速验证压缩数据完整性
- 不需要解码整个文件
- 性能敏感场景

### decode (解码验证)

```bash
# Layer 1 + Layer 3
spz_verify decode input.spz output.glb
```

**适用场景**:
- 验证最终输出质量
- 不关心中间压缩数据
- 终端用户场景

### all (完整验证)

```bash
# Layer 1 + Layer 2 + Layer 3
spz_verify all input.spz output.glb
```

**适用场景**:
- 完整质量验证
- 开发和测试
- 生产环境部署

## 📊 性能对比

| 模式 | 速度 | 内存 | 准确性 |
|------|------|------|--------|
| Layer 1 | 快 (~0.1s) | 低 | 结构验证 |
| Layer 2 | 中 (~1s) | 中 | 压缩数据 |
| Layer 3 | 慢 (~10s) | 高 | 完整数据 |
| lossless | 中 (~1s) | 中 | 高 |
| decode | 慢 (~10s) | 高 | 很高 |
| all | 慢 (~10s) | 高 | 完美 |

*注：时间基于 50MB SPZ 文件，实际性能因文件而异*

## 🛠️ 自动化验证

### 批量验证脚本

```bash
#!/bin/bash
# verify_all.sh

for spz in *.spz; do
    glb="${spz%.spz}.glb"
    if [ -f "$glb" ]; then
        echo "验证：$spz -> $glb"
        ./spz_verify all "$spz" "$glb"
        if [ $? -ne 0 ]; then
            echo "错误：验证失败"
            exit 1
        fi
    fi
done

echo "所有文件验证通过"
```

### CI/CD 集成

```yaml
# GitHub Actions 示例
- name: Verify conversion
  run: |
    ./spz2glb test.spz test.glb
    ./spz_verify all test.spz test.glb
```

## ❓ 常见问题

### Q: Layer 2 和 Layer 3 有什么区别？

A: 
- **Layer 2** 验证压缩数据（MD5），不解码
- **Layer 3** 验证解码后的原始数据，需要完全解码

### Q: 为什么 Layer 3 比较慢？

A: Layer 3 需要完整解码 SPZ 和 GLB 文件，涉及大量计算和内存操作。

### Q: 可以跳过某层验证吗？

A: 可以，但不推荐。完整验证确保最高质量。

### Q: MD5 冲突概率？

A: MD5 冲突概率约为 1/2^128，实际上可以忽略不计。

## 🔗 相关资源

- [使用方法](Usage) - 命令行参考
- [SPZ Format](SPZ-Format) - SPZ 格式详解
- [GLB Structure](GLB-Structure) - GLB 文件结构
- [Performance](Performance) - 性能优化

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
