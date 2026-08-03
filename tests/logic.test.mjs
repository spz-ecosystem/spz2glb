#!/usr/bin/env node
/**
 * 前端 JS 逻辑单测（node:test，零浏览器依赖）。
 *
 * 覆盖 docs/examples/spz2glb_bindings.js 导出的纯函数：
 *   - parseGlbJson     GLB 二进制解析（合法/非法/缺 chunk/KHR 扩展）
 *   - generateReportJson 报告字段完整性（成功/失败/conversionMs 分支）
 *   - detectSpzVersion  SPZ 头版本检测（v4 zstd / v3 gzip / 未知）
 *   - queueCounts       队列状态机统计（曾因 'pending' vs 'queued' 键名不匹配出 NaN）
 *
 * CI 用法：
 *   node --test --test-reporter=json tests/logic.test.mjs > logic-tests.json
 */

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TextEncoder, TextDecoder } from 'node:util';
import { parseGlbJson, generateReportJson, detectSpzVersion, queueCounts } from '../docs/examples/spz2glb_bindings.js';

const enc = new TextEncoder();

// ---------- GLB 构造工具 ----------
// GLB: magic(4) version(4) totalLen(4) + chunk(s): len(4) type(4) data(4对齐)

function buildGlb({ json, binBytes = new Uint8Array(0), version = 2 } = {}) {
    const jsonBytes = enc.encode(JSON.stringify(json));
    const pad4 = (n) => (4 - (n % 4)) % 4;

    // JSON chunk 4 字节对齐 padding 用空格（0x20），符合 glTF 规范——用 \0 填充会让 JSON.parse 失败
    const jsonChunkData = new Uint8Array(jsonBytes.length + pad4(jsonBytes.length)).fill(0x20);
    jsonChunkData.set(jsonBytes);
    const binChunkData = new Uint8Array(binBytes.length + pad4(binBytes.length));
    binChunkData.set(binBytes);

    const chunks = [
        [jsonChunkData, 0x4E4F534A], // "JSON"
        [binChunkData, 0x004E4942],  // "BIN\0"
    ];

    let totalLen = 12;
    for (const [data] of chunks) totalLen += 8 + data.length;

    const buf = new Uint8Array(totalLen);
    const view = new DataView(buf.buffer);
    view.setUint32(0, 0x46546C67, true); // "glTF"
    view.setUint32(4, version, true);
    view.setUint32(8, totalLen, true);

    let offset = 12;
    for (const [data, type] of chunks) {
        view.setUint32(offset, data.length, true);
        view.setUint32(offset + 4, type, true);
        buf.set(data, offset + 8);
        offset += 8 + data.length;
    }
    return buf;
}

const FULL_JSON = {
    asset: { version: '2.0' },
    extensionsUsed: ['KHR_gaussian_splatting', 'KHR_gaussian_splatting_compression_spz_2'],
    extensionsRequired: ['KHR_gaussian_splatting'],
    meshes: [{
        primitives: [{
            extensions: {
                KHR_gaussian_splatting: {
                    kernel: 'GAUSSIAN_KERNEL_1_1',
                    colorSpace: 'LINEAR_SRGB',
                    sortingMethod: 'GLOBAL_SORT',
                    projection: 'ORTHOGRAPHIC',
                    // 压缩扩展嵌套在 KHR_gaussian_splatting.extensions（spz2glb 实际输出结构）
                    extensions: {
                        KHR_gaussian_splatting_compression_spz_2: {
                            bufferView: 0,
                            spzVersion: 2,
                            compression: 'zstd',
                            coordinateSystem: 1,
                        },
                    },
                },
            },
        }],
    }],
};

// ---------- parseGlbJson ----------

test('parseGlbJson: 合法 GLB 提取 JSON/BIN 大小与 KHR 扩展', () => {
    const glb = buildGlb({ json: FULL_JSON, binBytes: new Uint8Array(1024) });
    const info = parseGlbJson(glb);
    assert.ok(info, '应返回解析结果');
    assert.equal(info.totalSizeBytes, glb.length);
    assert.ok(info.jsonChunkSize > 0, 'jsonChunkSize 应 > 0');
    assert.equal(info.binChunkSize, 1024);
    assert.deepEqual(info.extensionsUsed, ['KHR_gaussian_splatting', 'KHR_gaussian_splatting_compression_spz_2']);
    assert.deepEqual(info.extensionsRequired, ['KHR_gaussian_splatting']);
    assert.equal(info.KHR_gaussian_splatting.kernel, 'GAUSSIAN_KERNEL_1_1');
    assert.equal(info.KHR_gaussian_splatting_compression_spz_2.compression, 'zstd');
    assert.equal(info.KHR_gaussian_splatting_compression_spz_2.coordinateSystem, 1);
});

test('parseGlbJson: 无扩展 GLB 返回空数组/null 扩展', () => {
    const glb = buildGlb({ json: { asset: { version: '2.0' }, meshes: [{ primitives: [{}] }] } });
    const info = parseGlbJson(glb);
    assert.ok(info);
    assert.deepEqual(info.extensionsUsed, []);
    assert.deepEqual(info.extensionsRequired, []);
    assert.equal(info.KHR_gaussian_splatting, null);
    assert.equal(info.KHR_gaussian_splatting_compression_spz_2, null);
});

test('parseGlbJson: 过短输入 (<12B) 返回 null', () => {
    assert.equal(parseGlbJson(new Uint8Array(8)), null);
    assert.equal(parseGlbJson(new Uint8Array(0)), null);
});

test('parseGlbJson: 非 glTF magic 返回 null', () => {
    const bad = new Uint8Array(12);
    assert.equal(parseGlbJson(bad), null);
});

test('parseGlbJson: 非 v2 版本返回 null', () => {
    const glb = buildGlb({ json: { asset: {} }, version: 1 });
    assert.equal(parseGlbJson(glb), null);
});

test('parseGlbJson: 无 JSON chunk 返回 null', () => {
    // 只有 BIN chunk（chunkType=0x004E4942），无 JSON → jsonStr 为空 → null
    const binData = new Uint8Array([1, 2, 3, 4]);
    const binPadded = new Uint8Array(8);
    binPadded.set(binData);
    const buf = new Uint8Array(12 + 8 + binPadded.length);
    const view = new DataView(buf.buffer);
    view.setUint32(0, 0x46546C67, true);
    view.setUint32(4, 2, true);
    view.setUint32(8, buf.length, true);
    view.setUint32(12, binPadded.length, true);
    view.setUint32(16, 0x004E4942, true);
    buf.set(binPadded, 20);
    assert.equal(parseGlbJson(buf), null);
});

test('parseGlbJson: JSON chunk 内容非法返回 null', () => {
    // 手工构造：JSON chunk 数据为非法 JSON 文本
    const badJson = enc.encode('{not valid json');
    const jsonChunkData = new Uint8Array(badJson.length + ((4 - (badJson.length % 4)) % 4));
    jsonChunkData.set(badJson);
    const buf = new Uint8Array(12 + 8 + jsonChunkData.length);
    const view = new DataView(buf.buffer);
    view.setUint32(0, 0x46546C67, true);
    view.setUint32(4, 2, true);
    view.setUint32(8, buf.length, true);
    view.setUint32(12, jsonChunkData.length, true);
    view.setUint32(16, 0x4E4F534A, true);
    buf.set(jsonChunkData, 20);
    assert.equal(parseGlbJson(buf), null);
});

test('parseGlbJson: 接受 WASM 视图 subarray（getBytesView 输出形态）', () => {
    // getBytesView() 返回 HEAPU8.subarray(ptr, ptr+size)，offset 可能非 0
    const glb = buildGlb({ json: FULL_JSON });
    const wrapper = new Uint8Array(glb.length + 16); // 前 16B 垃圾，验证 byteOffset 正确处理
    wrapper.set(glb, 16);
    const view = wrapper.subarray(16, 16 + glb.length);
    const info = parseGlbJson(view);
    assert.ok(info, 'subarray 视图应正确解析');
    assert.equal(info.totalSizeBytes, glb.length);
});

// ---------- generateReportJson ----------

const REPORT_OPTS = {
    fileName: 'test.spz',
    spzSizeBytes: 12345,
    spzVersion: 4,
    compression: 'zstd',
    glbSizeBytes: 5678,
    glbInfo: {
        jsonChunkSize: 100,
        binChunkSize: 5578,
        totalSizeBytes: 5678,
        extensionsUsed: ['KHR_gaussian_splatting'],
        extensionsRequired: [],
        KHR_gaussian_splatting: { kernel: 'GAUSSIAN_KERNEL_1_1', colorSpace: 'LINEAR_SRGB', sortingMethod: 'GLOBAL_SORT', projection: 'ORTHOGRAPHIC' },
        KHR_gaussian_splatting_compression_spz_2: { bufferView: 0, spzVersion: 2, compression: 'zstd', coordinateSystem: 1 },
    },
    success: true,
    timingMs: 250.6,
    conversionMs: 200.4,
};

test('generateReportJson: 成功报告字段完整 + 单位展示', () => {
    const report = JSON.parse(generateReportJson(REPORT_OPTS));
    assert.equal(report.result, 'success');
    assert.equal(report.file, 'test.spz');
    assert.equal(report.sizeBytes, 12345);
    assert.equal(report.spz.version, 4);
    assert.equal(report.spz.compression, 'zstd');
    assert.equal(report.glb.jsonChunkSize, 100);
    assert.equal(report.glb.binChunkSize, 5578);
    assert.equal(report.glb.totalSizeBytes, 5678);
    assert.equal(report.glb.outputSizeBytes, 5678);
    assert.equal(report.timingMs, 250.6);
    assert.equal(report.timingMsDisplay, '251 ms');
    assert.equal(report.conversionMs, 200.4);
    assert.equal(report.conversionMsDisplay, '200 ms');
    assert.equal(report.coordinateSystem.found, true);
    assert.equal(report.coordinateSystem.value, 1);
    assert.equal(report.generator.name, 'spz2glb');
    assert.equal(report.generator.version, '2.0.5');
    assert.equal(report.extensionsUsed[0], 'KHR_gaussian_splatting');
    assert.equal(report.KHR_gaussian_splatting.kernel, 'GAUSSIAN_KERNEL_1_1');
    assert.equal(report.KHR_gaussian_splatting_compression_spz_2.compression, 'zstd');
    assert.ok(report.timestamp, '应包含 ISO 时间戳');
});

test('generateReportJson: 失败报告含 error + conversionMs 为 null', () => {
    const report = JSON.parse(generateReportJson({
        ...REPORT_OPTS,
        success: false,
        errorMsg: 'boom',
        glbSizeBytes: 0,
        glbInfo: null,
        conversionMs: undefined,
    }));
    assert.equal(report.result, 'failed');
    assert.equal(report.error, 'boom');
    assert.equal(report.conversionMs, null);
    assert.equal(report.conversionMsDisplay, null);
    assert.equal(report.glb.outputSizeBytes, 0);
});

test('generateReportJson: conversionMs 未传不显示单位', () => {
    const report = JSON.parse(generateReportJson({ ...REPORT_OPTS, conversionMs: undefined }));
    assert.equal(report.conversionMs, null);
    assert.equal(report.conversionMsDisplay, null);
});

test('generateReportJson: 无 glbInfo 时扩展字段安全回退', () => {
    const report = JSON.parse(generateReportJson({ ...REPORT_OPTS, glbInfo: null, success: false, errorMsg: 'x' }));
    assert.equal(report.glb.jsonChunkSize, 0);
    assert.deepEqual(report.extensionsUsed, []);
    // glbInfo 为 null → KHR 对象为 {}（空对象），coordinateSystem.value 走 ?? -1 回退
    assert.equal(report.KHR_gaussian_splatting_compression_spz_2.coordinateSystem, undefined);
    assert.equal(report.coordinateSystem.value, -1);
    assert.equal(report.coordinateSystem.found, false);
});

// ---------- detectSpzVersion ----------

test('detectSpzVersion: v4 NGSP 头 → zstd', () => {
    const buf = new Uint8Array([0x4E, 0x47, 0x53, 0x50, 4, 0, 0, 0]);
    assert.deepEqual(detectSpzVersion(buf), { version: 4, compression: 'zstd' });
});

test('detectSpzVersion: v3 gzip 头 → gzip', () => {
    const buf = new Uint8Array([0x1F, 0x8B, 8, 0]);
    assert.deepEqual(detectSpzVersion(buf), { version: 3, compression: 'gzip' });
});

test('detectSpzVersion: 未知/过短 → unknown/0', () => {
    assert.deepEqual(detectSpzVersion(new Uint8Array([0x00, 0x00])), { version: 0, compression: 'unknown' });
    assert.deepEqual(detectSpzVersion(new Uint8Array(0)), { version: 0, compression: 'unknown' });
});

// ---------- queueCounts ----------

test('queueCounts: 空队列全 0', () => {
    assert.deepEqual(queueCounts([]), { queued: 0, processing: 0, done: 0, failed: 0 });
});

test('queueCounts: 混合状态正确计数', () => {
    const queue = [
        { status: 'queued' }, { status: 'queued' },
        { status: 'processing' },
        { status: 'done' }, { status: 'done' }, { status: 'done' },
        { status: 'failed' },
    ];
    assert.deepEqual(queueCounts(queue), { queued: 2, processing: 1, done: 3, failed: 1 });
});

test('queueCounts: 未知状态被忽略（不污染四个槽位）', () => {
    // 历史 bug：状态名 'pending' 曾导致 qPending 显示 NaN——未知状态不得改变标准计数
    const queue = [{ status: 'pending' }, { status: 'queued' }];
    assert.deepEqual(queueCounts(queue), { queued: 1, processing: 0, done: 0, failed: 0 });
});

test('queueCounts: 不修改原队列', () => {
    const queue = [{ status: 'queued' }];
    queueCounts(queue);
    assert.deepEqual(queue, [{ status: 'queued' }]);
});
