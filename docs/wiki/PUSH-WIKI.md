# Wiki 推送指南

本文档介绍如何将本地 Wiki 文档推送到 GitHub Wiki。

## 📋 前置要求

1. **Git 已安装**
   ```bash
   git --version
   ```

2. **GitHub Token**（推荐）
   - 访问 https://github.com/settings/tokens
   - 创建新的 token（勾选 `public_repo` 权限）
   - 保存 token 值

## 🚀 推送方法

### 方法一：PowerShell 脚本（Windows 推荐）

```powershell
# 1. 设置 GitHub Token（可选，但推荐）
$env:GITHUB_TOKEN = "your_github_token_here"

# 2. 运行推送脚本
.\push_wiki.ps1
```

### 方法二：Bash 脚本（Linux/macOS 推荐）

```bash
# 1. 设置 GitHub Token（可选，但推荐）
export GITHUB_TOKEN="your_github_token_here"

# 2. 赋予执行权限
chmod +x push_wiki.sh

# 3. 运行推送脚本
./push_wiki.sh
```

### 方法三：手动推送

```bash
# 1. 克隆 Wiki 仓库
git clone https://github.com/spz-ecosystem/spz2glb.wiki.git temp_wiki
cd temp_wiki

# 2. 复制所有 Wiki 文件
cp ../docs/wiki/*.md .

# 3. 提交并推送
git config user.name "Your Name"
git config user.email "your.email@example.com"
git add .
git commit -m "docs: 创建完整 Wiki 文档"
git push origin main

# 4. 清理
cd ..
rm -rf temp_wiki
```

## 🔍 脚本功能

推送脚本会自动：

1. ✅ 检查源目录和文件
2. ✅ 验证 Git 是否可用
3. ✅ 克隆 Wiki 仓库
4. ✅ 复制所有 `.md` 文件
5. ✅ 提交更改
6. ✅ 推送到 GitHub
7. ✅ 清理临时文件

## 📊 Wiki 文件列表

脚本会推送以下 16 个文件：

### 入门指南（3 个）
- Home.md
- Installation.md
- Quick-Start.md

### 使用文档（4 个）
- Usage.md
- Verification.md
- Batch-Processing.md
- Performance.md

### 技术文档（3 个）
- SPZ-Format.md
- GLB-Structure.md
- Extensions.md

### 高级主题（3 个）
- Troubleshooting.md
- FAQ.md
- Building.md

### 开发文档（3 个）
- Testing.md
- Contributing.md
- Wiki-Structure.md

## ⚠️ 常见问题

### Q: 提示权限错误？

**A**: 使用 GitHub Token：
```bash
export GITHUB_TOKEN="your_token_here"
```

### Q: Wiki 仓库不存在？

**A**: GitHub 会自动创建。第一次推送时会自动初始化 Wiki。

### Q: 推送失败？

**A**: 检查：
- 网络连接
- Token 权限
- Git 配置

### Q: 如何更新 Wiki？

**A**: 修改 `docs/wiki/` 中的文件后，重新运行推送脚本即可。

## 🔗 查看 Wiki

推送成功后，访问：
https://github.com/spz-ecosystem/spz2glb/wiki

## 📝 手动更新单个页面

```bash
# 克隆 Wiki
git clone https://github.com/spz-ecosystem/spz2glb.wiki.git
cd spz2glb.wiki

# 编辑文件
# 修改对应的 .md 文件

# 提交推送
git add Home.md
git commit -m "docs: 更新 Home 页面"
git push origin main
```

## 🎯 最佳实践

1. **本地预览**: 推送前在本地预览 Markdown 文件
2. **小步提交**: 每次更新少量页面
3. **清晰描述**: 提交信息说明更新内容
4. **定期同步**: 保持 Wiki 与代码同步更新

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
