# SPZ 格式

本文档详细介绍 SPZ（SParse Zipline）文件格式。

## 📋 概述

SPZ 是一种用于 Gaussian Splatting 数据的压缩格式，基于 Zipline 压缩算法。

### 特点

- **高压缩率**: 相比原始数据压缩 10-50 倍
- **无损压缩**: 压缩流模式保持数据完整
- **流式处理**: 支持边解压边解码
- **快速解压**: 优化的解压算法

### 版本

- **SPZ v1**: 初始版本
- **SPZ v2**: 支持 KHR_gaussian_splatting_compression_spz_2 扩展（当前版本）

## 📐 文件格式

### 整体结构

```
SPZ 文件
├── Header (头部)
│   ├── Magic Number (魔数)
│   ├── Version (版本号)
│   └── Metadata (元数据)
├── Compression Stream (压缩流)
│   ├── Compressed Data (压缩数据)
│   └── Checksum (校验和)
└── Footer (尾部，可选)
```

### Header 结构

```
Offset  Size  Field           Description
0       4     magic           魔数："SPZ\0" (0x53505A00)
4       4     version         版本号：2
8       8     uncompressed_size 解压后大小
16      16    md5_hash        MD5 哈希值（压缩数据）
32      ...   metadata        JSON 元数据（可选）
```

### 压缩流

SPZ 使用**压缩流模式**（Compression Stream Mode）：

```
原始数据 → [压缩] → 压缩流 → [存储] → SPZ 文件
                        ↓
                   MD5 哈希
```

**特点**:
- 压缩数据保持完整
- 可以提取原始压缩流
- MD5 哈希验证无损

## 🔍 元数据

### JSON 结构

```json
{
  "version": 2,
  "num_points": 1000000,
  "attributes": {
    "position": { "type": "float", "count": 3 },
    "color": { "type": "uint8", "count": 3 },
    "normal": { "type": "uint8", "count": 4 },
    "scaling": { "type": "float", "count": 3 },
    "rotation": { "type": "float", "count": 4 },
    "opacity": { "type": "float", "count": 1 },
    "f_dc": { "type": "float", "count": 3 },
    "f_rest": { "type": "float", "count": 45 }
  },
  "compression": {
    "algorithm": "zipline",
    "level": "default",
    "mode": "stream"
  }
}
```

### 必需字段

- `version`: SPZ 格式版本（整数）
- `num_points`: 高斯球数量（整数）
- `attributes`: 属性定义（对象）
- `compression`: 压缩信息（对象）

### 可选字段

- `copyright`: 版权信息
- `generator`: 生成器信息
- `timestamp`: 时间戳

## 📊 属性说明

### 必需属性

| 属性 | 类型 | 数量 | 说明 |
|------|------|------|------|
| `position` | float32 | 3 | 位置 (x, y, z) |
| `color` | uint8 | 3 | 颜色 (r, g, b) |
| `scaling` | float32 | 3 | 缩放 (sx, sy, sz) |

### 推荐属性

| 属性 | 类型 | 数量 | 说明 |
|------|------|------|------|
| `normal` | uint8 | 4 | 法线（四元数） |
| `opacity` | float32 | 1 | 不透明度 |

### 可选属性

| 属性 | 类型 | 数量 | 说明 |
|------|------|------|------|
| `rotation` | float32 | 4 | 旋转（四元数） |
| `f_dc` | float32 | 3 | 球谐系数 DC |
| `f_rest` | float32 | 45 | 球谐系数剩余 |

## 🔐 压缩算法

### Zipline 压缩

SPZ 使用 Zipline 压缩算法：

```
原始数据 → [预处理] → [熵编码] → [后处理] → 压缩数据
```

### 压缩模式

#### 1. Compression Stream Mode（推荐）

```
完整压缩流 → 存储
```

**优点**:
- 保持压缩数据完整
- 可以计算 MD5 哈希
- 支持无损验证

**缺点**:
- 文件稍大（包含完整压缩流）

#### 2. Decompressed Mode（不推荐）

```
压缩流 → 解压 → 存储原始数据
```

**优点**:
- 快速访问

**缺点**:
- 丢失压缩信息
- 无法验证无损
- 文件更大

## 📏 文件大小

### 典型压缩率

| 场景 | 原始大小 | SPZ 大小 | 压缩率 |
|------|----------|----------|--------|
| 室内场景 | 100 MB | 5 MB | 20:1 |
| 室外场景 | 500 MB | 25 MB | 20:1 |
| 复杂物体 | 200 MB | 10 MB | 20:1 |
| 简单物体 | 50 MB | 2 MB | 25:1 |

*注：实际压缩率因数据而异*

### 文件大小计算

```
SPZ 文件大小 ≈ Header + 压缩数据 + Metadata

压缩数据 ≈ 原始大小 / 压缩率
Header = 48 bytes (固定)
Metadata ≈ 500-2000 bytes (JSON)
```

## 🔍 验证 SPZ 文件

### 使用 spz_verify

```bash
# 验证 Layer 1（结构）
spz_verify layer1 input.spz

# 验证 MD5
# 需要提取压缩数据并计算 MD5
```

### 手动检查

```bash
# 检查文件大小
ls -lh input.spz

# 检查魔数（前 4 字节）
xxd -l 16 input.spz

# 输出示例
# 00000000: 5350 5a00 0200 0000 ...  SPZ.....
```

## 🛠️ 创建 SPZ 文件

### SPZ 编码器要求

1. **格式兼容**:
   - 遵循 SPZ v2 规范
   - 使用正确的魔数和版本号

2. **压缩模式**:
   - 使用 Compression Stream Mode
   - 保持压缩流完整

3. **元数据**:
   - 包含所有必需字段
   - 使用正确的属性定义

4. **MD5 哈希**:
   - 计算压缩数据的 MD5
   - 存储在 Header 中

### 示例代码（伪代码）

```cpp
// 创建 SPZ 编码器
SPZEncoder encoder;
encoder.setVersion(2);
encoder.setMode(CompressionStreamMode);

// 添加属性
encoder.addAttribute("position", DataType::Float32, 3);
encoder.addAttribute("color", DataType::UInt8, 3);
encoder.addAttribute("scaling", DataType::Float32, 3);

// 设置数据
encoder.setData(points, numPoints);

// 压缩并保存
encoder.compress();
encoder.save("output.spz");
```

## ❓ 常见问题

### Q: SPZ 和 Draco 有什么区别？

A:
- **SPZ**: 专为 Gaussian Splatting 设计，支持球谐系数
- **Draco**: 通用 3D 压缩，不支持 SPZ 的特殊属性

### Q: SPZ 支持动画吗？

A: 当前版本不支持动画，仅支持静态高斯泼溅。

### Q: 如何更新 SPZ 文件？

A: 重新编码整个文件，SPZ 不支持增量更新。

## 🔗 相关资源

- [GLB Structure](GLB-Structure) - GLB 文件结构
- [Extensions](Extensions) - SPZ_2 扩展说明
- [Verification](Verification) - 验证工具

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
