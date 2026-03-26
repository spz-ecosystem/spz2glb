import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { gzipSync } from 'node:zlib';

import { chromium } from 'playwright';

function createMinimalSpzBuffer() {
    const buffer = Buffer.alloc(16);
    buffer.writeUInt32LE(0x5053474e, 0);
    buffer.writeUInt32LE(2, 4);
    buffer.writeUInt32LE(1, 8);
    buffer.writeUInt8(0, 12);
    buffer.writeUInt8(8, 13);
    buffer.writeUInt8(0, 14);
    buffer.writeUInt8(0, 15);
    return buffer;
}

function createInvalidSpzBuffer() {
    const buffer = createMinimalSpzBuffer();
    buffer[0] = 0;
    return buffer;
}

function createOversizedSpzBuffer(sizeBytes) {
    const header = createMinimalSpzBuffer();
    const buffer = Buffer.alloc(sizeBytes, 0);
    header.copy(buffer, 0, 0, header.length);
    return buffer;
}

function parseRenderedSize(text) {
    const normalized = String(text ?? '').trim();
    const match = normalized.match(/^(\d+(?:\.\d+)?)\s*(B|KB|MB)$/i);
    if (!match) {
        throw new Error(`unexpected size format: ${normalized}`);
    }
    const value = Number(match[1]);
    const unit = match[2].toUpperCase();
    if (!Number.isFinite(value) || value < 0) {
        throw new Error(`invalid size value: ${normalized}`);
    }
    if (unit === 'MB') return value * 1024 * 1024;
    if (unit === 'KB') return value * 1024;
    return value;
}

async function main() {
    const baseUrl = new URL(process.env.SPZ2GLB_SMOKE_URL ?? 'http://127.0.0.1:8000/index.html');
    const chunkSize = Number(process.env.SPZ2GLB_SMOKE_CHUNK_SIZE ?? 8);
    baseUrl.searchParams.set('chunkSize', String(chunkSize));

    const tempDir = mkdtempSync(join(tmpdir(), 'spz2glb-smoke-'));
    const validPath = join(tempDir, 'valid.spz');
    const invalidPath = join(tempDir, 'invalid.spz');
    const oversizedPath = join(tempDir, 'oversized.spz');
    writeFileSync(validPath, gzipSync(createMinimalSpzBuffer()));
    writeFileSync(invalidPath, gzipSync(createInvalidSpzBuffer()));
    writeFileSync(oversizedPath, createOversizedSpzBuffer(72 * 1024 * 1024));


    const browser = await chromium.launch({ headless: true });
    const page = await browser.newPage();
    const pageErrors = [];
    page.on('pageerror', (error) => pageErrors.push(error.message));

    try {
        await page.goto(baseUrl.toString(), { waitUntil: 'networkidle', timeout: 30000 });
        await page.waitForFunction(
            () => document.getElementById('statusBox')?.textContent?.includes('WASM 模块加载成功'),
            null,
            { timeout: 30000 },
        );

        await page.locator('#fileInput').setInputFiles(invalidPath);
        await page.waitForFunction(
            () => document.getElementById('fileInfo')?.textContent?.includes('SPZ 头部预检失败'),
            null,
            { timeout: 10000 },
        );
        const invalidDisabled = await page.locator('#convertBtn').isDisabled();
        if (!invalidDisabled) {
            throw new Error('invalid SPZ should keep convert button disabled');
        }

        await page.locator('#fileInput').setInputFiles(oversizedPath);
        await page.waitForFunction(
            () => document.getElementById('fileInfo')?.textContent?.includes('文件超过浏览器安全上限'),
            null,
            { timeout: 10000 },
        );
        const oversizedDisabled = await page.locator('#convertBtn').isDisabled();
        if (!oversizedDisabled) {
            throw new Error('oversized SPZ should be rejected and keep convert button disabled');
        }

        await page.locator('#fileInput').setInputFiles(validPath);

        await page.waitForFunction(
            () => !document.getElementById('convertBtn')?.disabled,
            null,
            { timeout: 10000 },
        );

        const downloadPromise = page.waitForEvent('download', { timeout: 30000 });
        await page.locator('#convertBtn').click();
        const download = await downloadPromise;
        const downloadPath = await download.path();
        if (!downloadPath) {
            throw new Error('download path is unavailable');
        }

        await page.waitForFunction(
            () => document.getElementById('statusBox')?.textContent?.includes('转换成功'),
            null,
            { timeout: 10000 },
        );
        await page.waitForFunction(
            () => document.getElementById('statsBox')?.style.display === 'grid',
            null,
            { timeout: 10000 },
        );

        const state = await page.evaluate(() => window.__spz2glbTestState);
        if (!state || !Array.isArray(state.chunkWrites) || state.chunkWrites.length < 2) {
            throw new Error(`expected multi-chunk writes, got ${JSON.stringify(state)}`);
        }
        if (state.chunkWrites.some((chunk) => chunk.size > chunkSize)) {
            throw new Error(`chunk size exceeded limit: ${JSON.stringify(state.chunkWrites)}`);
        }
        let expectedOffset = 0;
        for (const chunk of state.chunkWrites) {
            if (chunk.offset !== expectedOffset || chunk.end <= chunk.offset) {
                throw new Error(`chunk writes are not contiguous: ${JSON.stringify(state.chunkWrites)}`);
            }
            expectedOffset = chunk.end;
        }
        if (expectedOffset !== state.inputSize) {
            throw new Error(`chunk writes did not cover full input: ${expectedOffset} vs ${state.inputSize}`);
        }

        if (state.chunkSize !== chunkSize) {
            throw new Error(`unexpected chunk size in state: ${JSON.stringify(state)}`);
        }

        const operationNames = Array.isArray(state.operations) ? state.operations.map((entry) => entry.name) : [];
        const statsIndex = operationNames.indexOf('memory-stats-read');
        const viewIndex = operationNames.indexOf('output-view-read');
        const blobIndex = operationNames.indexOf('output-blob-created');
        const releaseIndex = operationNames.indexOf('output-released');
        const postReleaseErrorIndex = operationNames.indexOf('post-release-view-error');
        if (statsIndex < 0 || viewIndex <= statsIndex || blobIndex <= viewIndex) {
            throw new Error(`expected stats -> view -> blob order: ${JSON.stringify(state.operations)}`);
        }
        if (releaseIndex <= blobIndex || postReleaseErrorIndex <= releaseIndex) {
            throw new Error(`expected release lifecycle markers after blob creation: ${JSON.stringify(state.operations)}`);
        }
        const postReleaseError = state.operations[postReleaseErrorIndex];
        if (!postReleaseError?.message?.includes('WASM 输出已释放')) {
            throw new Error(`missing post-release view error: ${JSON.stringify(state.operations)}`);
        }


        const glb = readFileSync(downloadPath);
        if (glb.length < 12 || glb.readUInt32LE(0) !== 0x46546c67 || glb.readUInt32LE(4) !== 2) {
            throw new Error('downloaded file is not a valid GLB header');
        }
        if (state.blobSize !== glb.length || state.outputSize !== glb.length) {
            throw new Error(`download size mismatch: state=${JSON.stringify(state)}, glb=${glb.length}`);
        }

        const statValues = await page.evaluate(() => ({
            current: document.getElementById('statCurrent')?.textContent,
            peak: document.getElementById('statPeak')?.textContent,
            workPeak: document.getElementById('statWorkPeak')?.textContent,
            hotAvailable: document.getElementById('statHotAvailable')?.textContent,
        }));
        if (Object.values(statValues).some((value) => !value || value === '-')) {
            throw new Error(`memory stats were not rendered: ${JSON.stringify(statValues)}`);
        }
        const currentBytes = parseRenderedSize(statValues.current);
        const peakBytes = parseRenderedSize(statValues.peak);
        const workPeakBytes = parseRenderedSize(statValues.workPeak);
        const hotAvailable = Number(statValues.hotAvailable);
        if (peakBytes < currentBytes) {
            throw new Error(`peak memory must be >= current memory: ${JSON.stringify(statValues)}`);
        }
        if (!Number.isFinite(workPeakBytes) || workPeakBytes <= 0) {
            throw new Error(`work peak must be positive: ${JSON.stringify(statValues)}`);
        }
        if (!Number.isFinite(hotAvailable) || hotAvailable < 0) {
            throw new Error(`hot pool availability is invalid: ${JSON.stringify(statValues)}`);
        }
        if (pageErrors.length > 0) {
            throw new Error(`page errors: ${pageErrors.join(' | ')}`);
        }

    } finally {
        await page.close();
        await browser.close();
        rmSync(tempDir, { recursive: true, force: true });
    }
}


await main();
