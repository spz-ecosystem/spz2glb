# 批量处理

本文档介绍如何批量转换和验证 SPZ 文件。

## 📝 概述

批量处理适用于：
- 多个 SPZ 文件需要转换
- 自动化测试流程
- 生产环境部署
- CI/CD 流水线

## 🔄 批量转换

### Windows (PowerShell)

```powershell
# 基本批量转换
Get-ChildItem *.spz | ForEach-Object {
    $output = $_.BaseName + ".glb"
    Write-Host "转换：$($_.Name) -> $output"
    .\spz2glb.exe $_.Name $output
}

# 带错误处理
Get-ChildItem *.spz | ForEach-Object {
    $output = $_.BaseName + ".glb"
    try {
        .\spz2glb.exe $_.Name $output
        Write-Host "✓ 成功：$($_.Name)" -ForegroundColor Green
    } catch {
        Write-Host "✗ 失败：$($_.Name)" -ForegroundColor Red
    }
}

# 带验证
Get-ChildItem *.spz | ForEach-Object {
    $spz = $_.Name
    $glb = $_.BaseName + ".glb"
    
    # 转换
    .\spz2glb.exe $spz $glb
    
    # 验证
    .\spz_verify.exe all $spz $glb
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ 验证通过：$spz" -ForegroundColor Green
    } else {
        Write-Host "✗ 验证失败：$spz" -ForegroundColor Red
    }
}
```

### Linux/macOS (Bash)

```bash
# 基本批量转换
for file in *.spz; do
    output="${file%.spz}.glb"
    echo "转换：$file -> $output"
    ./spz2glb "$file" "$output"
done

# 带错误处理
for file in *.spz; do
    output="${file%.spz}.glb"
    if ./spz2glb "$file" "$output"; then
        echo "✓ 成功：$file"
    else
        echo "✗ 失败：$file"
    fi
done

# 带验证
for file in *.spz; do
    spz="$file"
    glb="${file%.spz}.glb"
    
    # 转换
    ./spz2glb "$spz" "$glb"
    
    # 验证
    if ./spz_verify all "$spz" "$glb"; then
        echo "✓ 验证通过：$spz"
    else
        echo "✗ 验证失败：$spz"
        exit 1
    fi
done
```

## ✅ 批量验证

### 验证所有 GLB 文件

```powershell
# Windows (PowerShell)
Get-ChildItem *.glb | ForEach-Object {
    $spz = $_.BaseName + ".spz"
    if (Test-Path $spz) {
        Write-Host "验证：$spz -> $($_.Name)"
        .\spz_verify.exe all $spz $_.Name
    }
}
```

```bash
# Linux/macOS
for glb in *.glb; do
    spz="${glb%.glb}.spz"
    if [ -f "$spz" ]; then
        echo "验证：$spz -> $glb"
        ./spz_verify all "$spz" "$glb"
    fi
done
```

### 仅验证 Layer 1（快速检查）

```powershell
# Windows
Get-ChildItem *.glb | ForEach-Object {
    .\spz_verify.exe layer1 $_.Name
}
```

```bash
# Linux/macOS
for glb in *.glb; do
    ./spz_verify layer1 "$glb"
done
```

## 📜 自动化脚本

### verify.bat (Windows)

```batch
@echo off
setlocal enabledelayedexpansion

echo SPZ 批量验证工具
echo =================

set count=0
set failed=0

for %%f in (*.spz) do (
    set /a count+=1
    set glb=%%~nf.glb
    
    if exist "!glb!" (
        echo [!count!] 验证：%%f -> !glb!
        spz_verify.exe all "%%f" "!glb!"
        if errorlevel 1 (
            echo 失败：%%f
            set /a failed+=1
        ) else (
            echo 成功：%%f
        )
    ) else (
        echo 跳过：!glb! 不存在
    )
)

echo.
echo 总计：%count% 个文件
echo 成功：%count% - %failed% 个文件
echo 失败：%failed% 个文件

if %failed% gtr 0 exit /b 1
```

### verify.sh (Linux/macOS)

```bash
#!/bin/bash

echo "SPZ 批量验证工具"
echo "================"

count=0
failed=0

for spz in *.spz; do
    ((count++))
    glb="${spz%.spz}.glb"
    
    if [ -f "$glb" ]; then
        echo "[$count] 验证：$spz -> $glb"
        ./spz_verify all "$spz" "$glb"
        if [ $? -eq 0 ]; then
            echo "成功：$spz"
        else
            echo "失败：$spz"
            ((failed++))
        fi
    else
        echo "跳过：$glb 不存在"
    fi
done

echo ""
echo "总计：$count 个文件"
echo "成功：$((count - failed)) 个文件"
echo "失败：$failed 个文件"

if [ $failed -gt 0 ]; then
    exit 1
fi
```

### convert_and_verify.sh (完整流程)

```bash
#!/bin/bash

echo "SPZ 批量转换和验证工具"
echo "======================"

count=0
converted=0
verified=0
failed=0

for spz in *.spz; do
    ((count++))
    glb="${spz%.spz}.glb"
    
    echo "[$count/$count] 处理：$spz"
    
    # 转换
    echo "  转换中..."
    if ./spz2glb -v "$spz" "$glb" 2>&1 | tee /tmp/convert.log; then
        ((converted++))
        echo "  ✓ 转换成功"
    else
        echo "  ✗ 转换失败"
        ((failed++))
        continue
    fi
    
    # 验证
    echo "  验证中..."
    if ./spz_verify all "$spz" "$glb" 2>&1 | tee /tmp/verify.log; then
        ((verified++))
        echo "  ✓ 验证通过"
    else
        echo "  ✗ 验证失败"
        ((failed++))
    fi
    
    echo ""
done

echo "======================"
echo "总计：$count 个文件"
echo "转换成功：$converted 个"
echo "验证通过：$verified 个"
echo "失败：$failed 个"

if [ $failed -gt 0 ]; then
    echo ""
    echo "错误：$failed 个文件失败"
    exit 1
fi

echo ""
echo "所有文件处理完成"
```

## 🎯 高级用法

### 并行处理（加速）

```bash
# Linux/macOS - GNU Parallel
export -f spz2glb
export -f spz_verify

ls *.spz | parallel -j 4 '
    spz={};
    glb=${spz%.spz}.glb;
    ./spz2glb "$spz" "$glb" && ./spz_verify all "$spz" "$glb"
'

# Windows - PowerShell 并行
$files = Get-ChildItem *.spz
$files | ForEach-Object -Parallel {
    $spz = $_.Name
    $glb = $_.BaseName + ".glb"
    .\spz2glb.exe $spz $glb
    .\spz_verify.exe all $spz $glb
} -ThrottleLimit 4
```

### 子目录处理

```bash
# 递归处理所有子目录
find . -name "*.spz" | while read spz; do
    dir=$(dirname "$spz")
    base=$(basename "$spz" .spz)
    glb="$dir/$base.glb"
    
    ./spz2glb "$spz" "$glb"
    ./spz_verify all "$spz" "$glb"
done
```

### 日志记录

```bash
# 记录到文件
for spz in *.spz; do
    glb="${spz%.spz}.glb"
    
    {
        echo "================================"
        echo "文件：$spz"
        echo "时间：$(date)"
        echo "================================"
        
        ./spz2glb "$spz" "$glb" 2>&1
        ./spz_verify all "$spz" "$glb" 2>&1
        
        echo ""
    } >> conversion.log
done
```

## 📊 性能优化

### 1. SSD 存储

使用 SSD 可以显著提升批量处理速度：
- HDD: ~50 MB/s
- SSD: ~500 MB/s
- NVMe: ~3000 MB/s

### 2. 并行处理

```bash
# 根据 CPU 核心数调整
nproc=$(nproc)  # Linux
# nproc=$(sysctl -n hw.ncpu)  # macOS

parallel -j $nproc ...
```

### 3. 内存管理

大文件批量处理时，限制并发数避免内存溢出：

```bash
# 限制同时处理 2 个文件
parallel -j 2 ...
```

## 📋 检查清单

批量处理前：

- [ ] 检查磁盘空间（至少 2 倍于输入文件总大小）
- [ ] 确保所有 SPZ 文件有效
- [ ] 测试单个文件转换
- [ ] 准备日志记录
- [ ] 设置错误处理

批量处理中：

- [ ] 监控进度
- [ ] 检查错误日志
- [ ] 监控系统资源（CPU、内存、磁盘）

批量处理后：

- [ ] 验证所有输出文件
- [ ] 检查日志完整性
- [ ] 清理临时文件
- [ ] 备份结果

## ❓ 常见问题

### Q: 批量处理中断怎么办？

A: 脚本会从断点继续，或者使用 `--resume` 选项（如果支持）。

### Q: 如何处理大量文件（1000+）？

A: 
1. 分批处理（每批 100 个）
2. 使用并行处理
3. 监控磁盘空间
4. 定期清理临时文件

### Q: 如何验证批量处理结果？

A: 使用批量验证脚本，或抽样检查。

## 🔗 相关资源

- [使用方法](Usage) - 命令行参考
- [Quick Start](Quick-Start) - 快速开始
- [Performance](Performance) - 性能优化

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
