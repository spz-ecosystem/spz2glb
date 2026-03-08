# 故障排除

本文档帮助解决 spz2glb 使用中的常见问题。

## 🔍 诊断步骤

### 1. 收集信息

```bash
# 检查版本
spz2glb --version
spz_verify --version

# 检查系统
# Windows
systeminfo

# Linux
uname -a
lscpu
free -h

# macOS
system_profiler SPHardwareDataType
```

### 2. 启用详细输出

```bash
# 转换时带详细输出
spz2glb -v input.spz output.glb

# 验证时带详细输出
spz_verify -v all input.spz output.glb
```

### 3. 记录日志

```bash
# 保存完整日志
spz2glb -v input.spz output.glb 2>&1 | tee convert.log
spz_verify -v all input.spz output.glb 2>&1 | tee verify.log
```

## ❌ 转换错误

### 错误：无法打开输入文件

```
错误：无法打开输入文件 'input.spz'
原因：没有那个文件或目录
```

**解决方案**:
```bash
# 检查文件是否存在
ls -l input.spz

# 检查路径
realpath input.spz

# 检查权限
chmod +r input.spz
```

### 错误：无效的 SPZ 魔数

```
错误：无效的 SPZ 魔数
期望：0x53505A00
实际：0x00000000
```

**原因**:
- 文件损坏
- 不是 SPZ 格式
- 文件为空

**解决方案**:
```bash
# 检查文件大小
ls -lh input.spz

# 检查魔数
xxd -l 16 input.spz

# 重新生成 SPZ 文件
```

### 错误：内存不足

```
错误：std::bad_alloc
无法分配 536870912 字节
```

**解决方案**:
```bash
# 1. 关闭其他程序
# 2. 增加交换空间（Linux）
sudo fallocate -l 8G /swapfile
sudo swapon /swapfile

# 3. 分批处理大文件
# 4. 升级系统内存
```

### 错误：磁盘空间不足

```
错误：写入失败：磁盘空间不足
需要：10 GB
可用：2 GB
```

**解决方案**:
```bash
# 检查磁盘空间
df -h

# 清理空间
rm -rf /tmp/*
rm -rf ~/.cache

# Windows
del /q %TEMP%\*
```

### 错误：ZLIB 未找到

```
CMake 错误：Could NOT find ZLIB
```

**解决方案**:
```bash
# Windows - 使用 vcpkg
vcpkg install zlib:x64-windows
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake ...

# Ubuntu/Debian
sudo apt-get install zlib1g-dev

# Fedora/RHEL
sudo dnf install zlib-devel

# macOS
brew install zlib
```

## ❌ 验证错误

### Layer 1 失败：无效的 GLB 魔数

```
✗ Layer 1: GLB 结构验证失败
错误：无效的 GLB 魔数
期望：0x46546C67
实际：0x00000000
```

**原因**:
- GLB 文件损坏
- 文件未完整写入
- 不是 GLB 格式

**解决方案**:
```bash
# 1. 重新转换
spz2glb input.spz output.glb

# 2. 检查文件大小
ls -lh output.glb

# 3. 检查文件头
xxd -l 16 output.glb
```

### Layer 2 失败：MD5 不匹配

```
✗ Layer 2: MD5 无损验证失败
SPZ MD5: abc123...
GLB MD5: def456...
```

**原因**:
- 压缩数据损坏
- 转换过程出错
- 文件被篡改

**解决方案**:
```bash
# 1. 重新转换
rm output.glb
spz2glb input.spz output.glb

# 2. 验证 SPZ 文件
spz_verify layer1 input.spz

# 3. 检查 SPZ MD5
# 使用其他工具提取并计算 MD5
```

### Layer 3 失败：解码数据不匹配

```
✗ Layer 3: 解码一致性验证失败
SPZ 点数：1000000
GLB 点数：999999
```

**原因**:
- 解码器不兼容
- 数据损坏
- 版本不匹配

**解决方案**:
```bash
# 1. 检查版本兼容性
# SPZ 和 GLB 必须使用相同版本

# 2. 重新生成 SPZ
# 使用最新版本的 SPZ 编码器

# 3. 联系工具作者
```

## ⚠️ 警告信息

### 警告：非 4 字节对齐

```
警告：bufferView 0 未 4 字节对齐
```

**影响**: 可能导致性能下降或兼容性问题

**解决方案**:
```cpp
// 确保对齐
byteOffset = (byteOffset + 3) & ~3;
```

### 警告：缺少可选属性

```
警告：缺少 'rotation' 属性
```

**影响**: 某些查看器可能无法正确显示

**解决方案**:
```bash
# 1. 忽略（可选属性）
# 2. 在 SPZ 编码器中添加该属性
# 3. 使用默认值
```

## 🔧 运行时问题

### 问题：程序崩溃

```
Segmentation fault (core dumped)
```

**诊断**:
```bash
# Linux - 启用 core dump
ulimit -c unlimited

# 运行程序
./spz2glb input.spz output.glb

# 分析 core 文件
gdb ./spz2glb core
bt
```

**解决方案**:
1. 更新到最新版本
2. 检查文件是否损坏
3. 减少文件大小测试
4. 联系开发者

### 问题：性能低下

**症状**:
- 转换速度慢
- CPU 使用率低
- 磁盘 I/O 瓶颈

**诊断**:
```bash
# 监控资源使用
# Linux
top -d 1
iostat -x 1

# Windows
Get-Process | Sort-Object CPU -Descending
```

**解决方案**:
1. 使用 SSD/NVMe
2. 增加并发数
3. 优化系统设置
4. 检查散热

### 问题：输出文件过大

**症状**:
- GLB 文件比预期大很多

**诊断**:
```bash
# 检查压缩率
ls -lh input.spz output.glb

# 检查 JSON 大小
# GLB 中的 JSON 应该很小（< 100 KB）
```

**解决方案**:
1. 检查 SPZ 压缩模式（应使用 stream）
2. 验证压缩数据是否正确
3. 重新编码 SPZ 文件

## 📋 检查清单

### 转换前

- [ ] 输入文件存在且可读
- [ ] 磁盘空间充足（2 倍文件大小）
- [ ] 内存充足（2 倍文件大小）
- [ ] 工具版本最新

### 转换中

- [ ] 无错误信息
- [ ] 进度正常
- [ ] 资源使用合理

### 转换后

- [ ] 输出文件存在
- [ ] 文件大小合理
- [ ] 验证通过（所有层）

## 🆘 获取帮助

### 1. 查看文档

- [Wiki](https://github.com/spz-ecosystem/spz2glb/wiki)
- [README](https://github.com/spz-ecosystem/spz2glb/blob/main/README.md)
- [Issues](https://github.com/spz-ecosystem/spz2glb/issues)

### 2. 提交 Issue

包含以下信息：
- 系统信息（OS、CPU、内存）
- 工具版本
- 完整错误信息
- 重现步骤
- 示例文件（如果可能）

### 3. 社区支持

- GitHub Discussions
- 相关论坛
- Discord/Slack 频道

## 🔗 相关资源

- [FAQ](FAQ) - 常见问题
- [Performance](Performance) - 性能优化
- [Usage](Usage) - 使用方法

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
