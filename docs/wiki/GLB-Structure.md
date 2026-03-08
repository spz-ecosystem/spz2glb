# GLB 结构

本文档详细介绍 spz2glb 输出的 GLB 文件结构。

## 📋 概述

GLB（glTF Binary）是 glTF 2.0 的二进制格式，包含所有资源在单个文件中。

### 特点

- **自包含**: JSON + BIN 数据在单个文件
- **高效**: 零拷贝访问，快速加载
- **标准**: Khronos Group 官方标准
- **扩展**: 支持 KHR_gaussian_splatting_compression_spz_2

## 📐 文件结构

### 整体布局

```
GLB 文件
├── Header (12 bytes)
│   ├── magic (4 bytes)
│   ├── version (4 bytes)
│   └── length (4 bytes)
├── JSON Chunk (可变大小)
│   ├── chunkLength (4 bytes)
│   ├── chunkType (4 bytes) - "JSON"
│   └── JSON Data
└── BIN Chunk (可变大小)
    ├── chunkLength (4 bytes)
    ├── chunkType (4 bytes) - "BIN\0"
    └── Binary Data
```

### Header 详解

```
Offset  Size  Field   Value
0       4     magic   0x46546C67 ("glTF")
4       4     version 2
8       4     length  文件总长度
```

### JSON Chunk

```
Offset  Size  Field       Value
0       4     length      JSON 数据长度（4 字节对齐）
4       4     type        0x4E4F534A ("JSON")
8       N     data        JSON 文本（UTF-8）
```

### BIN Chunk

```
Offset  Size  Field       Value
0       4     length      二进制数据长度（4 字节对齐）
4       4     type        0x004E4942 ("BIN\0")
8       N     data        二进制数据
```

## 📝 JSON 结构

### 完整示例

```json
{
  "asset": {
    "version": "2.0",
    "generator": "spz_to_glb_fastgltf",
    "copyright": ""
  },
  "extensionsUsed": [
    "KHR_gaussian_splatting_compression_spz_2"
  ],
  "extensionsRequired": [
    "KHR_gaussian_splatting_compression_spz_2"
  ],
  "extensions": {
    "KHR_gaussian_splatting_compression_spz_2": {
      "version": 2,
      "num_points": 1000000,
      "compression": {
        "algorithm": "zipline",
        "mode": "stream"
      }
    }
  },
  "buffers": [
    {
      "byteLength": 5242880,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "compressed_size": 5242880,
          "md5_hash": "abc123def456..."
        }
      }
    }
  ],
  "bufferViews": [
    {
      "buffer": 0,
      "byteOffset": 0,
      "byteLength": 5242880,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "attribute": "position",
          "component_type": 5126,
          "type": "VEC3"
        }
      }
    }
  ],
  "accessors": [
    {
      "bufferView": 0,
      "componentType": 5126,
      "type": "VEC3",
      "count": 1000000
    }
  ]
}
```

### 必需字段

#### asset

```json
{
  "asset": {
    "version": "2.0"
  }
}
```

#### buffers

```json
{
  "buffers": [
    {
      "byteLength": 5242880
    }
  ]
}
```

#### bufferViews

```json
{
  "bufferViews": [
    {
      "buffer": 0,
      "byteOffset": 0,
      "byteLength": 5242880
    }
  ]
}
```

#### accessors

```json
{
  "accessors": [
    {
      "bufferView": 0,
      "componentType": 5126,
      "type": "VEC3",
      "count": 1000000
    }
  ]
}
```

### 扩展字段

#### KHR_gaussian_splatting_compression_spz_2

```json
{
  "extensions": {
    "KHR_gaussian_splatting_compression_spz_2": {
      "version": 2,
      "num_points": 1000000,
      "attributes": {
        "position": {
          "bufferView": 0,
          "componentType": 5126,
          "type": "VEC3"
        },
        "color": {
          "bufferView": 1,
          "componentType": 5121,
          "type": "VEC3"
        }
      },
      "compression": {
        "algorithm": "zipline",
        "mode": "stream"
      }
    }
  }
}
```

## 📊 属性映射

### SPZ → GLB

| SPZ 属性 | GLB bufferView | componentType | type |
|----------|----------------|---------------|------|
| position | 0 | 5126 (FLOAT) | VEC3 |
| color | 1 | 5121 (UNSIGNED_BYTE) | VEC3 |
| normal | 2 | 5121 (UNSIGNED_BYTE) | VEC4 |
| scaling | 3 | 5126 (FLOAT) | VEC3 |
| rotation | 4 | 5126 (FLOAT) | VEC4 |
| opacity | 5 | 5126 (FLOAT) | SCALAR |
| f_dc | 6 | 5126 (FLOAT) | VEC3 |
| f_rest | 7 | 5126 (FLOAT) | VEC3[15] |

### Component Types

| 值 | 常量 | 类型 |
|----|------|------|
| 5120 | BYTE | int8 |
| 5121 | UNSIGNED_BYTE | uint8 |
| 5122 | SHORT | int16 |
| 5123 | UNSIGNED_SHORT | uint16 |
| 5125 | UNSIGNED_INT | uint32 |
| 5126 | FLOAT | float32 |

### Types

| 常量 | 组件数 | 说明 |
|------|--------|------|
| SCALAR | 1 | 标量 |
| VEC2 | 2 | 2 维向量 |
| VEC3 | 3 | 3 维向量 |
| VEC4 | 4 | 4 维向量 |
| MAT3 | 9 | 3x3 矩阵 |
| MAT4 | 16 | 4x4 矩阵 |

## 🔍 BIN 数据布局

### 内存布局

```
BIN Chunk Data
├── Attribute 0: position (num_points * 3 * 4 bytes)
├── Attribute 1: color (num_points * 3 * 1 bytes)
├── Attribute 2: normal (num_points * 4 * 1 bytes)
├── Attribute 3: scaling (num_points * 3 * 4 bytes)
├── Attribute 4: rotation (num_points * 4 * 4 bytes)
├── Attribute 5: opacity (num_points * 1 * 4 bytes)
├── Attribute 6: f_dc (num_points * 3 * 4 bytes)
└── Attribute 7: f_rest (num_points * 45 * 4 bytes)
```

### 对齐要求

所有 bufferView 的 byteOffset 必须 4 字节对齐：

```cpp
// 计算对齐后的偏移
byteOffset = alignTo4(currentOffset);

// 对齐函数
size_t alignTo4(size_t value) {
    return (value + 3) & ~3;
}
```

### 大小计算

```
总大小 = 12 (Header) 
       + 8 + JSON 大小 (对齐后)
       + 8 + BIN 大小 (对齐后)

JSON 大小对齐 = (JSON 大小 + 3) & ~3
BIN 大小对齐 = (BIN 大小 + 3) & ~3
```

## 📏 文件大小示例

### 示例：100 万点

```
属性          大小计算                      字节数
-----------------------------------------------------------
position      1000000 * 3 * 4              12,000,000
color         1000000 * 3 * 1               3,000,000
normal        1000000 * 4 * 1               4,000,000
scaling       1000000 * 3 * 4              12,000,000
rotation      1000000 * 4 * 4              16,000,000
opacity       1000000 * 1 * 4               4,000,000
f_dc          1000000 * 3 * 4              12,000,000
f_rest        1000000 * 45 * 4            180,000,000
-----------------------------------------------------------
原始数据总计                                  243,000,000

压缩后 (20:1)                                   12,150,000
JSON                                              5,000
GLB Header                                         12
Chunk Headers                                      16
-----------------------------------------------------------
GLB 文件总计                                      ~12.2 MB
```

## 🔐 MD5 哈希

### 位置

MD5 哈希存储在 buffer 扩展中：

```json
{
  "buffers": [
    {
      "byteLength": 5242880,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "md5_hash": "abc123def456789..."
        }
      }
    }
  ]
}
```

### 计算

```cpp
// 提取压缩数据
const uint8_t* compressedData = binData + byteOffset;
size_t compressedSize = bufferView.byteLength;

// 计算 MD5
Md5Hash hash;
hash.update(compressedData, compressedSize);
hash.finalize();

// 获取 16 字节哈希
uint8_t digest[16];
hash.getDigest(digest);
```

### 验证

```bash
# Layer 2 验证
spz_verify layer2 input.spz output.glb

# 输出
# ✓ MD5 验证通过
# - SPZ MD5: abc123...
# - GLB MD5: abc123...
```

## ❓ 常见问题

### Q: 为什么使用 GLB 而不是 glTF？

A: GLB 是二进制格式，更紧凑，加载更快，适合运行时使用。

### Q: GLB 文件大小？

A: 取决于点数和压缩率，典型 100 万点约 10-20 MB。

### Q: 如何查看 GLB 内容？

A: 使用 glTF 查看器或 Three.js/Babylon.js。

## 🔗 相关资源

- [SPZ Format](SPZ-Format) - SPZ 格式详解
- [Extensions](Extensions) - SPZ_2 扩展
- [Verification](Verification) - 验证工具

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
