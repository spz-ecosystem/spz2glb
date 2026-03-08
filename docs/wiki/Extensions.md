# 扩展说明

本文档详细介绍 KHR_gaussian_splatting_compression_spz_2 扩展。

## 📋 概述

`KHR_gaussian_splatting_compression_spz_2` 是专为 Gaussian Splatting 数据设计的 glTF 2.0 扩展。

### 命名约定

- **KHR**: Khronos Group 标准前缀
- **gaussian_splatting**: 高斯泼溅技术
- **compression**: 压缩格式
- **spz_2**: SPZ 版本 2

### 目标

1. **标准化**: 统一 Gaussian Splatting 数据存储格式
2. **互操作性**: 不同工具间无缝交换数据
3. **高效压缩**: 使用 SPZ 压缩算法
4. **无损验证**: 支持 MD5 哈希验证

## 📐 扩展结构

### JSON 扩展声明

```json
{
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
      "attributes": {...},
      "compression": {...}
    }
  }
}
```

### 顶层扩展属性

| 属性 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `version` | integer | 是 | 扩展版本（2） |
| `num_points` | integer | 是 | 高斯球数量 |
| `attributes` | object | 是 | 属性定义 |
| `compression` | object | 是 | 压缩信息 |
| `copyright` | string | 否 | 版权信息 |
| `generator` | string | 否 | 生成器信息 |

## 📊 属性定义

### attributes 对象

```json
{
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
    },
    "scaling": {
      "bufferView": 2,
      "componentType": 5126,
      "type": "VEC3"
    }
  }
}
```

### 属性详情

#### position (位置)

```json
{
  "position": {
    "bufferView": 0,
    "componentType": 5126,
    "type": "VEC3",
    "normalized": false
  }
}
```

- **用途**: 高斯球的 3D 位置
- **类型**: float32 (5126)
- **分量**: 3 (VEC3) - (x, y, z)
- **单位**: 米（右手坐标系，Y 轴向上）

#### color (颜色)

```json
{
  "color": {
    "bufferView": 1,
    "componentType": 5121,
    "type": "VEC3",
    "normalized": true
  }
}
```

- **用途**: 高斯球的基础颜色
- **类型**: uint8 (5121)
- **分量**: 3 (VEC3) - (r, g, b)
- **范围**: 0-255 (normalized: true 表示 0.0-1.0)

#### normal (法线)

```json
{
  "normal": {
    "bufferView": 2,
    "componentType": 5121,
    "type": "VEC4",
    "normalized": true
  }
}
```

- **用途**: 法线方向（四元数格式）
- **类型**: uint8 (5121)
- **分量**: 4 (VEC4) - (x, y, z, w)
- **范围**: 0-255 映射到 -1.0-1.0

#### scaling (缩放)

```json
{
  "scaling": {
    "bufferView": 3,
    "componentType": 5126,
    "type": "VEC3"
  }
}
```

- **用途**: 高斯球的缩放因子
- **类型**: float32 (5126)
- **分量**: 3 (VEC3) - (sx, sy, sz)

#### rotation (旋转)

```json
{
  "rotation": {
    "bufferView": 4,
    "componentType": 5126,
    "type": "VEC4"
  }
}
```

- **用途**: 高斯球的旋转（四元数）
- **类型**: float32 (5126)
- **分量**: 4 (VEC4) - (x, y, z, w)
- **必需**: 否（可选属性）

#### opacity (不透明度)

```json
{
  "opacity": {
    "bufferView": 5,
    "componentType": 5126,
    "type": "SCALAR"
  }
}
```

- **用途**: 高斯球的不透明度
- **类型**: float32 (5126)
- **分量**: 1 (SCALAR)
- **范围**: 0.0 (完全透明) - 1.0 (完全不透明)

#### f_dc (球谐系数 DC)

```json
{
  "f_dc": {
    "bufferView": 6,
    "componentType": 5126,
    "type": "VEC3"
  }
}
```

- **用途**: 球谐函数的 DC 分量
- **类型**: float32 (5126)
- **分量**: 3 (VEC3)
- **必需**: 否（用于 SH 光照）

#### f_rest (球谐系数剩余)

```json
{
  "f_rest": {
    "bufferView": 7,
    "componentType": 5126,
    "type": "VEC3[15]"
  }
}
```

- **用途**: 球谐函数的高频分量
- **类型**: float32 (5126)
- **分量**: 45 (VEC3[15]) - 15 个 VEC3
- **必需**: 否（用于 SH 光照，Degree 3）

## 🔐 压缩信息

### compression 对象

```json
{
  "compression": {
    "algorithm": "zipline",
    "version": "1.0",
    "mode": "stream",
    "level": "default"
  }
}
```

### 属性详情

| 属性 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `algorithm` | string | 是 | 压缩算法（"zipline"） |
| `version` | string | 否 | 算法版本 |
| `mode` | string | 是 | 压缩模式（"stream"） |
| `level` | string | 否 | 压缩级别 |

### 压缩模式

#### stream (推荐)

```json
{
  "mode": "stream"
}
```

- 保持压缩流完整
- 支持 MD5 哈希验证
- 无损压缩

## 🔍 Buffer 扩展

### MD5 哈希存储

```json
{
  "buffers": [
    {
      "byteLength": 5242880,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "compressed_size": 5242880,
          "md5_hash": "abc123def456789012345678901234ab"
        }
      }
    }
  ]
}
```

### 字段说明

- `compressed_size`: 压缩数据大小（字节）
- `md5_hash`: 32 字符十六进制 MD5 哈希字符串

## 📐 BufferView 扩展

### 属性元数据

```json
{
  "bufferViews": [
    {
      "buffer": 0,
      "byteOffset": 0,
      "byteLength": 12000000,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "attribute": "position",
          "component_type": 5126,
          "type": "VEC3",
          "count": 1000000
        }
      }
    }
  ]
}
```

## 🎯 兼容性要求

### 必需实现

任何实现此扩展的查看器/工具必须：

1. **解析扩展**: 正确读取扩展数据
2. **解压数据**: 使用 Zipline 算法解压
3. **渲染高斯球**: 正确显示 3D 高斯泼溅
4. **验证 MD5**: 支持 Layer 2 验证（推荐）

### 可选实现

- Layer 3 解码验证
- 球谐光照（f_dc, f_rest）
- 动画支持（未来版本）

## 📝 完整示例

```json
{
  "asset": {
    "version": "2.0",
    "generator": "spz_to_glb_fastgltf"
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
      "num_points": 1234567,
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
        },
        "scaling": {
          "bufferView": 2,
          "componentType": 5126,
          "type": "VEC3"
        }
      },
      "compression": {
        "algorithm": "zipline",
        "mode": "stream"
      }
    }
  },
  "buffers": [
    {
      "byteLength": 50000000,
      "extensions": {
        "KHR_gaussian_splatting_compression_spz_2": {
          "compressed_size": 50000000,
          "md5_hash": "d41d8cd98f00b204e9800998ecf8427e"
        }
      }
    }
  ],
  "bufferViews": [
    {
      "buffer": 0,
      "byteOffset": 0,
      "byteLength": 14814804,
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
      "count": 1234567
    }
  ]
}
```

## 🔗 相关资源

- [SPZ Format](SPZ-Format) - SPZ 格式详解
- [GLB Structure](GLB-Structure) - GLB 文件结构
- [Verification](Verification) - 验证工具

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
