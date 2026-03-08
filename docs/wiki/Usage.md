# 使用方法

本文档介绍 spz2glb 和 spz_verify 的完整命令行用法。

## 🔄 spz2glb 转换器

### 语法

```bash
spz2glb [选项] <输入文件.spz> <输出文件.glb>
```

### 参数

| 参数 | 说明 | 必需 |
|------|------|------|
| `<输入文件.spz>` | SPZ 格式输入文件 | 是 |
| `<输出文件.glb>` | GLB 格式输出文件 | 是 |

### 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-v, --verbose` | 详细输出模式 | 关闭 |
| `-h, --help` | 显示帮助信息 | - |
| `--version` | 显示版本号 | - |

### 示例

#### 基本转换

```bash
# 最简单用法
spz2glb input.spz output.glb

# 带详细输出
spz2glb -v input.spz output.glb
```

#### 批量转换

```bash
# Windows (PowerShell)
Get-ChildItem *.spz | ForEach-Object {
    $output = $_.BaseName + ".glb"
    .\spz2glb.exe $_.Name $output
}

# Linux/macOS
for file in *.spz; do
    output="${file%.spz}.glb"
    ./spz2glb "$file" "$output"
done
```

#### 带路径转换

```bash
# 指定完整路径
spz2glb /path/to/input.spz /path/to/output.glb

# Windows
spz2glb.exe C:\data\input.spz D:\output\output.glb
```

## ✅ spz_verify 验证器

### 语法

```bash
spz_verify <模式> [参数]
```

### 模式

| 模式 | 参数 | 说明 |
|------|------|------|
| `layer1` | `<file.glb>` | 仅验证 GLB 结构 |
| `layer2` | `<file.spz> <file.glb>` | 仅验证 MD5 无损 |
| `layer3` | `<file.spz> <file.glb>` | 仅验证解码一致性 |
| `lossless` | `<file.spz> <file.glb>` | Layer 1 + Layer 2 |
| `decode` | `<file.spz> <file.glb>` | Layer 1 + Layer 3 |
| `all` | `<file.spz> <file.glb>` | 完整三层验证 |

### 示例

#### Layer 1: GLB 结构验证

```bash
# 验证 GLB 文件结构
spz_verify layer1 output.glb

# 输出示例
# ✓ GLB 结构验证通过
# - Header: 12 bytes
# - Chunks: 2 (JSON, BIN)
```

#### Layer 2: MD5 无损验证

```bash
# 验证压缩数据无损
spz_verify layer2 input.spz output.glb

# 输出示例
# ✓ MD5 验证通过
# - SPZ MD5: abc123...
# - GLB MD5: abc123...
# - 匹配：是
```

#### Layer 3: 解码一致性验证

```bash
# 验证解码数据一致性
spz_verify layer3 input.spz output.glb

# 输出示例
# ✓ 解码验证通过
# - 解码 SPZ: 1000000 points
# - 解码 GLB: 1000000 points
# - 匹配：是
```

#### 组合验证

```bash
# 无损验证（Layer 1 + 2）
spz_verify lossless input.spz output.glb

# 解码验证（Layer 1 + 3）
spz_verify decode input.spz output.glb

# 完整验证（Layer 1 + 2 + 3）
spz_verify all input.spz output.glb
```

## 📊 输出说明

### 成功输出

```
✓ Layer 1: GLB 结构验证通过
✓ Layer 2: MD5 无损验证通过
✓ Layer 3: 解码一致性验证通过

验证完成：所有检查通过
总耗时：1.234 秒
```

### 失败输出

```
✗ Layer 1: GLB 结构验证失败
错误：无效的 GLB 魔数

验证失败：1 个检查未通过
```

## 🎯 最佳实践

### 1. 转换后立即验证

```bash
# 转换
spz2glb input.spz output.glb

# 立即验证
spz_verify all input.spz output.glb
```

### 2. 批量验证

```bash
# Windows (PowerShell)
Get-ChildItem *.glb | ForEach-Object {
    $spz = $_.BaseName + ".spz"
    if (Test-Path $spz) {
        .\spz_verify.exe all $spz $_.Name
    }
}

# Linux/macOS
for glb in *.glb; do
    spz="${glb%.glb}.spz"
    if [ -f "$spz" ]; then
        ./spz_verify all "$spz" "$glb"
    fi
done
```

### 3. 自动化脚本

创建验证脚本（见 [Batch Processing](Batch-Processing)）：

```bash
# verify.sh / verify.bat
spz2glb -v input.spz output.glb
spz_verify all input.spz output.glb
```

## ⚙️ 高级用法

### 压缩流模式

SPZ 文件必须使用**压缩流模式**（compression stream mode），这是默认模式。

```cpp
// SPZ 编码器设置
encoder.setMode(CompressionStreamMode);
```

### 自定义验证

```bash
# 仅验证特定层
spz_verify layer2 input.spz output.glb

# 跳过特定层
# 先验证 Layer 1
spz_verify layer1 output.glb
# 再验证 Layer 2
spz_verify layer2 input.spz output.glb
```

### 性能测试

```bash
# 计时转换
time spz2glb input.spz output.glb

# 计时验证
time spz_verify all input.spz output.glb
```

## ❓ 常见问题

### Q: 转换速度慢？

A: 检查：
- 文件大小（大文件需要更长时间）
- 系统内存（内存不足会降速）
- 磁盘速度（SSD 比 HDD 快）

### Q: 验证失败？

A: 检查：
- SPZ 文件是否损坏
- GLB 文件是否完整写入
- 是否使用正确的 SPZ_2 扩展

### Q: 内存不足？

A: 尝试：
- 关闭其他应用程序
- 使用 64 位系统
- 分批处理大文件

## 🔗 相关资源

- [Quick Start](Quick-Start) - 快速上手
- [Verification](Verification) - 三层验证详解
- [Batch Processing](Batch-Processing) - 批量处理
- [Troubleshooting](Troubleshooting) - 故障排除

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
