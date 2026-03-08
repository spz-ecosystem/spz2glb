# 贡献指南

欢迎为 spz2glb 项目做出贡献！

## 🎯 贡献方式

### 1. 代码贡献

- **Bug 修复**: 修复发现的问题
- **新功能**: 添加有用的功能
- **性能优化**: 提升性能
- **重构**: 改进代码质量

### 2. 文档贡献

- **Wiki 改进**: 完善文档
- **示例代码**: 提供使用示例
- **翻译**: 多语言支持

### 3. 社区贡献

- **回答问题**: 帮助其他用户
- **反馈问题**: 报告 Bug
- **功能建议**: 提出改进建议
- **宣传推广**: 分享使用经验

## 🚀 开始贡献

### 1. Fork 仓库

```bash
# GitHub 上点击 Fork 按钮
# 或克隆后创建分支
git clone https://github.com/your-username/spz2glb.git
cd spz2glb
```

### 2. 创建分支

```bash
# 基于 main 创建功能分支
git checkout -b feature/your-feature-name

# 或修复分支
git checkout -b fix/issue-123
```

### 3. 开发

```bash
# 构建项目
cmake -B build -S tools/spz_to_glb -DBUILD_TESTS=ON
cmake --build build -j4

# 运行测试
cd build
ctest --output-on-failure

# 开发...
```

### 4. 测试

```bash
# 确保所有测试通过
ctest --output-on-failure

# 检查代码风格
# 遵循项目代码规范

# 性能测试（如果适用）
./build/benchmarks
```

### 5. 提交

```bash
# 添加更改
git add src/your_changes.cpp

# 提交（遵循提交规范）
git commit -m "feat: add new feature"

# 或
git commit -m "fix: resolve issue #123"
```

### 6. 推送

```bash
# 推送到你的 fork
git push origin feature/your-feature-name
```

### 7. 创建 Pull Request

1. 访问 GitHub 仓库
2. 点击 "Pull requests"
3. 点击 "New pull request"
4. 选择你的分支
5. 填写 PR 描述
6. 提交 PR

## 📝 代码规范

### C++ 风格

```cpp
// 类名：大驼峰
class MyClassName {
public:
    // 函数：小驼峰
    void myFunctionName();
    
    // 变量：小写，下划线分隔
    int my_variable;
};

// 命名空间：小写
namespace my_namespace {
    // 常量：大写，下划线分隔
    const int MAX_SIZE = 100;
}

// 文件命名：小写，下划线分隔
// my_class.cpp, my_class.h
```

### 注释规范

```cpp
/**
 * @brief 函数简短描述
 * 
 * @param param1 参数 1 说明
 * @param param2 参数 2 说明
 * @return 返回值说明
 * 
 * @note 可选的注释
 */
void myFunction(int param1, std::string param2);
```

### 错误处理

```cpp
// 使用异常处理错误
if (error_condition) {
    throw std::runtime_error("错误信息");
}

// 或使用 optional
std::optional<Result> tryOperation() {
    if (success) {
        return Result();
    }
    return std::nullopt;
}
```

## 📋 提交规范

### Commit Message 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码风格（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具/配置

### 示例

```bash
# 新功能
git commit -m "feat: add batch processing support"

# Bug 修复
git commit -m "fix: resolve memory leak in decompression"

# 文档
git commit -m "docs: update installation guide"

# 性能优化
git commit -m "perf: optimize MD5 calculation"

# 重构
git commit -m "refactor: simplify error handling"
```

## 🧪 测试要求

### 单元测试

```cpp
// 为新功能添加单元测试
TEST(NewFeatureTest, BasicFunctionality) {
    // 测试基本功能
    EXPECT_EQ(operation(), expected_value);
}

TEST(NewFeatureTest, EdgeCases) {
    // 测试边界情况
    EXPECT_THROW(invalid_operation(), std::invalid_argument);
}
```

### 集成测试

```cpp
// 测试模块间交互
TEST(IntegrationTest, FullPipeline) {
    // 测试完整流程
    auto result = full_pipeline(input);
    EXPECT_TRUE(result.valid);
}
```

### 性能测试

```cpp
// 性能基准测试
static void BM_NewFeature(benchmark::State& state) {
    for (auto _ : state) {
        new_feature();
    }
}
BENCHMARK(BM_NewFeature);
```

## 📚 文档要求

### 代码文档

```cpp
/**
 * @brief 类/函数说明
 * 
 * 详细说明...
 * 
 * @tparam T 模板参数说明
 * @param param 参数说明
 * @return 返回值说明
 */
```

### Wiki 文档

- 使用清晰的标题
- 提供代码示例
- 包含截图（如适用）
- 添加相关链接

### README 更新

如果添加新功能，更新 README：
- 功能列表
- 使用示例
- 相关文档链接

## 🔍 Code Review

### 审查要点

- [ ] 代码正确性
- [ ] 代码风格
- [ ] 测试覆盖
- [ ] 文档完整
- [ ] 性能影响
- [ ] 安全性

### 回应审查

- **接受建议**: 直接修改
- **讨论**: 解释原因
- **拒绝**: 说明理由（有建设性）

### 修改流程

```bash
# 根据审查意见修改
git add .
git commit -m "address review comments"

# 推送到同一分支
git push origin feature/your-feature-name

# PR 会自动更新
```

## 🎓 学习资源

### C++ 资源

- [C++ Reference](https://en.cppreference.com/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Effective Modern C++](https://www.aristeia.com/Book7/Book7.html)

### 项目相关

- [fastgltf](https://github.com/spycrab/fastgltf)
- [simdjson](https://github.com/simdjson/simdjson)
- [glTF](https://www.khronos.org/gltf/)

## 🏆 贡献者权益

### 认可

- 名字出现在贡献者列表
- Release Notes 中提及
- GitHub 贡献图记录

### 权利

- 参与项目决策讨论
- 审查他人 PR
- 管理 Issue/PR

## ❓ 常见问题

### Q: 如何开始？

A: 
1. 查看 [Good First Issues](https://github.com/spz-ecosystem/spz2glb/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
2. Fork 仓库
3. 按照指南开始

### Q: 遇到问题怎么办？

A: 
- 查看文档
- 搜索现有 Issue
- 在 Discussion 中提问
- 联系维护者

### Q: PR 多久能合并？

A: 
- 简单修复：1-3 天
- 新功能：1-2 周
- 复杂功能：视情况而定

### Q: 如何成为维护者？

A: 
- 持续贡献高质量代码
- 积极参与社区
- 获得现有维护者认可

## 🔗 相关资源

- [Building](Building) - 构建指南
- [Testing](Testing) - 测试说明
- [FAQ](FAQ) - 常见问题
- [Discussions](https://github.com/spz-ecosystem/spz2glb/discussions) - 讨论区

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1
