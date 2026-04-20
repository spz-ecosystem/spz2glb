---
name: ci-cd-wasm-reproducibility
description: Use when a GitHub Actions WASM or Emscripten build fails intermittently, depends on latest toolchains, or downloads ports during CI and needs a reproducible, supply-chain-safe fix.
---

## Overview

这是从 `spz2glb` 的一次真实 CI/WASM 故障中抽出的 **skill 草案**，目标是后续迁移到 Graphify。核心原则只有一句：**先区分“代码回归”与“工具链/供应链漂移”，再动 workflow。**

## Trigger Signals

- workflow 使用 `latest` 工具链
- 失败集中在 `Emscripten`、`ports`、`zlib`、下载、404、cache
- 同一提交只在部分 matrix profile 失败
- `README`、workflow、`CMakeLists.txt` 对同一依赖的写法不一致
- 修复动作已经开始偏向“改业务代码”而不是“锁定构建环境”

## Root-Cause Workflow

1. **先找真实仓库根目录**：避免在错误目录看 `git status` / workflow。
2. **比对失败提交与最近成功提交**：确认是否真改到了构建链。
3. **识别真实故障文件**：不要修错 workflow，也不要被相邻仓库/根目录配置带偏。
4. **检查构建参数是否真生效**：重点排查“传了但 CMake/脚本根本没用”的无效开关。
5. **把 ports 依赖视为供应链输入**：只要构建期在线拉取，就要考虑 pin 版本与缓存。

## Minimal Fix Pattern

- **固定工具链版本**：把 `latest` 改成明确版本号。
- **缓存 Emscripten 系统库/ports**：统一设置 `EM_CACHE`，并在 GitHub Actions 中缓存该目录。
- **删除无效配置开关**：例如 workflow 传参、README 示例、CMake 选项三者不一致时，优先清理 no-op 参数。
- **统一 zlib 接入路径**：只保留一个经过验证的接入方式，避免 `-sUSE_ZLIB=1` 与零散 `--use-port=...` 文档并存。
- **同步文档**：`README`、troubleshooting、workflow 口径必须一致。

## Anti-Patterns

- 看到 `ports/zlib` 失败就直接改业务代码
- 没确认真实仓库边界就开始修 CI
- 在根目录 workflow 修复子仓库故障
- 把 `latest` 当成“不是版本问题”的理由继续保留
- 保留死参数，让后续排障继续被假线索误导

## Evidence Template

每次沉淀到 Graphify 前，至少保留：

- 失败 run id
- 最近成功 run id
- 失败 job / step 名称
- 是否涉及 `latest`
- 是否涉及在线 ports 下载
- 实际改动文件清单
- 哪些配置被确认是 no-op

## Suggested Graphify Nodes

- `detect-real-repo-root`
- `compare-last-successful-run`
- `classify-code-vs-toolchain-regression`
- `pin-emscripten-version`
- `cache-em-cache-directory`
- `remove-no-op-build-flags`
- `reconcile-readme-workflow-cmake`

## Notes for This Incident

本次 `spz2glb` 事件的直接经验是：**真实风险不是业务代码本身，而是 `latest` + 构建期在线取 zlib/ports + 文档/配置漂移叠加后，把排障路径带偏。** 这类问题非常适合沉淀成 Graphify 的可复用节点与 skill。
