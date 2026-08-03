#!/usr/bin/env node
/**
 * node:test 自定义 JSON reporter —— 生成结构化测试报告（CI 下载即看）。
 *
 * 用法：
 *   node --test --test-reporter=./tests/json_reporter.mjs tests/logic.test.mjs
 *
 * 输出：logic-tests.json（totals + 逐用例 status/durationMs）
 */
import { Transform } from 'node:stream';

const report = {
    generatedAt: new Date().toISOString(),
    suite: 'spz2glb frontend logic (parseGlbJson/report/detect/queue)',
    totals: { pass: 0, fail: 0, skip: 0, todo: 0 },
    cases: [],
};

export default new Transform({
    // node >=21: reporter 流接收反序列化的 TestEvent 对象（objectMode）
    writableObjectMode: true,
    transform(evt, _encoding, callback) {
        if (evt && typeof evt === 'object' && typeof evt.type === 'string') {
            const type = evt.type;
            if (type === 'test:pass' || type === 'test:fail' || type === 'test:skip' || type === 'test:todo') {
                const d = evt.data || {};
                const status = type.slice(5); // pass/fail/skip/todo
                report.totals[status] = (report.totals[status] || 0) + 1;
                report.cases.push({
                    name: d.name,
                    status,
                    durationMs: d.details?.duration_ms ?? null,
                });
            }
        }
        callback();
    },
    flush(callback) {
        this.push(JSON.stringify(report, null, 2) + '\n');
        callback();
    },
});
