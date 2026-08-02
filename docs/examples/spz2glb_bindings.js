export async function loadSpz2Glb(wasmUrl, options = {}) {
    // 缓存破坏：glue JS 与 wasm 二进制都带时间戳 query，避免浏览器 HTTP 缓存命中旧版本。
    // 与门卫 spz_gatekeeper.js 的 maybeLoadRuntime 同构——否则部署新 wasm 后强刷仍加载旧二进制。
    const cacheBust = '?v=' + Date.now();
    const moduleUrl = wasmUrl.replace(/\.wasm($|[?#])/, '.js$1');
    const { default: createModule } = await import(moduleUrl + cacheBust);

    const module = await createModule({
        ...options,
        print: options.print ?? ((text) => console.log('[WASM]', text)),
        printErr: options.printErr ?? ((text) => console.error('[WASM]', text)),
        locateFile: options.locateFile ?? ((path) => path.endsWith('.wasm') ? wasmUrl + cacheBust : path),
    });

    return {
        validateHeader(buffer) {
            return this.validateGlbHeader(buffer);
        },

        validateGlbHeader(buffer) {
            const [ptr, size] = writeBuffer(module, buffer);
            try {
                return module._spz2glb_validate_glb_header(ptr, size);
            } finally {
                freeBuffer(module, ptr);
            }
        },

        validateSpzHeader(buffer) {
            const [ptr, size] = writeBuffer(module, buffer);
            try {
                return module._spz2glb_validate_spz_header(ptr, size);
            } finally {
                freeBuffer(module, ptr);
            }
        },

        convert(spzBuffer) {
            const size = spzBuffer.byteLength;
            const inputPtr = reserveInput(module, size);
            try {
                getHeap(module).set(new Uint8Array(spzBuffer.buffer, spzBuffer.byteOffset, size), inputPtr);
                return convertReservedInput(module, size);
            } finally {
                releaseReservedInput(module);
            }
        },

        async convertFile(file, options = {}) {
            const chunkSize = normalizeChunkSize(options.chunkSize ?? 1024 * 1024);
            const onChunk = typeof options.onChunk === 'function' ? options.onChunk : null;
            const inputPtr = reserveInput(module, file.size);
            try {
                await writeFileToReservedInput(module, file, inputPtr, chunkSize, onChunk);
                return convertReservedInput(module, file.size);
            } finally {
                releaseReservedInput(module);
            }
        },

        getMemoryStats() {
            return readMemoryStats(module);
        },

        resetMemoryStats() {
            module._spz2glb_reset_memory_stats();
        },

        getVersion() {
            const { sizeTBytes } = getAbi(module);
            const ptr = module._malloc(sizeTBytes * 3);
            try {
                module._spz2glb_get_version(ptr, ptr + sizeTBytes, ptr + sizeTBytes * 2);
                const major = module.getValue(ptr, 'i32');
                const minor = module.getValue(ptr + sizeTBytes, 'i32');
                const patch = module.getValue(ptr + sizeTBytes * 2, 'i32');
                return `${major}.${minor}.${patch}`;
            } finally {
                freeBuffer(module, ptr);
            }
        },

        module,
    };
}

function getHeap(module) {
    return module.HEAPU8;
}

function getAbi(module) {
    if (module.__spz2glbAbi) {
        return module.__spz2glbAbi;
    }

    const sizeTBytes = module._spz2glb_sizeof_size_t();
    const memoryStatsBytes = module._spz2glb_sizeof_memory_stats();
    if (sizeTBytes !== 4 || memoryStatsBytes !== sizeTBytes * 9) {
        throw new Error(`Unsupported spz2glb ABI: size_t=${sizeTBytes}, memoryStats=${memoryStatsBytes}`);
    }

    const abi = { sizeTBytes, pointerBytes: sizeTBytes, memoryStatsBytes };
    module.__spz2glbAbi = abi;
    return abi;
}

function readSizeT(module, ptr) {
    return module.getValue(ptr, 'i32') >>> 0;
}

function reserveInput(module, size) {
    const reserved = module._spz2glb_reserve_input(size);
    if (reserved !== size) {
        throw new Error('WASM 输入缓冲预留失败');
    }

    const inputPtr = module._spz2glb_get_input_ptr();
    if (!inputPtr) {
        throw new Error('WASM 输入缓冲不可用');
    }
    return inputPtr;
}

function releaseReservedInput(module) {
    module._spz2glb_reserve_input(0);
}

function convertReservedInput(module, size) {
    const { pointerBytes, sizeTBytes } = getAbi(module);
    const outPtrPtr = module._malloc(pointerBytes);
    const outSizePtr = module._malloc(sizeTBytes);

    try {
        const ok = module._spz2glb_convert_reserved_input(size, outPtrPtr, outSizePtr);
        if (!ok) {
            return null;
        }

        const resultPtr = readSizeT(module, outPtrPtr);
        const outSize = readSizeT(module, outSizePtr);
        if (!resultPtr || !outSize) {
            if (resultPtr) {
                module._spz2glb_release_output(resultPtr);
            }
            return null;
        }

        return createOutputHandle(module, resultPtr, outSize);
    } finally {
        freeBuffer(module, outPtrPtr);
        freeBuffer(module, outSizePtr);
    }
}

function createOutputHandle(module, ptr, size) {
    let released = false;

    return {
        size,
        getBytesView() {
            if (released) {
                throw new Error('WASM 输出已释放');
            }
            return getHeap(module).subarray(ptr, ptr + size);
        },
        toBlob(type = 'application/octet-stream') {
            return new Blob([this.getBytesView()], { type });
        },
        get bytes() {
            return this.getBytesView();
        },
        release() {
            if (!released) {
                module._spz2glb_release_output(ptr);
                released = true;
            }
        },
    };
}

function normalizeChunkSize(chunkSize) {
    const normalized = Math.trunc(chunkSize);
    if (!Number.isFinite(normalized) || normalized <= 0) {
        throw new Error('chunkSize must be a positive integer');
    }
    return normalized;
}

async function writeFileToReservedInput(module, file, inputPtr, chunkSize, onChunk = null) {
    let offset = 0;
    while (offset < file.size) {
        const end = Math.min(offset + chunkSize, file.size);
        const chunk = new Uint8Array(await file.slice(offset, end).arrayBuffer());
        getHeap(module).set(chunk, inputPtr + offset);
        onChunk?.({ offset, end, size: chunk.byteLength });
        offset = end;
    }
}

function readMemoryStats(module) {
    const { sizeTBytes, memoryStatsBytes } = getAbi(module);
    const ptr = module._malloc(memoryStatsBytes);
    if (!ptr) {
        throw new Error('Memory allocation failed');
    }

    try {
        module._spz2glb_get_memory_stats(ptr);
        return {
            peakUsageBytes: readSizeT(module, ptr),
            currentUsageBytes: readSizeT(module, ptr + sizeTBytes),
            totalAllocations: readSizeT(module, ptr + sizeTBytes * 2),
            totalFrees: readSizeT(module, ptr + sizeTBytes * 3),
            failedAllocations: readSizeT(module, ptr + sizeTBytes * 4),
            hotAvailable: readSizeT(module, ptr + sizeTBytes * 5),
            workUsed: readSizeT(module, ptr + sizeTBytes * 6),
            workCapacity: readSizeT(module, ptr + sizeTBytes * 7),
            workPeak: readSizeT(module, ptr + sizeTBytes * 8),
        };
    } finally {
        freeBuffer(module, ptr);
    }
}

function writeBuffer(module, jsBuffer) {
    const size = jsBuffer.byteLength;
    const ptr = module._malloc(size);
    if (!ptr) {
        throw new Error('Memory allocation failed');
    }

    getHeap(module).set(new Uint8Array(jsBuffer.buffer, jsBuffer.byteOffset, size), ptr);
    return [ptr, size];
}

function freeBuffer(module, ptr) {
    if (ptr) {
        module._free(ptr);
    }
}

/**
 * GLB JSON 解析：从 GLB 二进制提取 JSON 区块并解析
 * @param {Uint8Array} glbBuffer - GLB 文件完整数据
 * @returns {object|null} 解析结果或 null
 */
export function parseGlbJson(glbBuffer) {
    const view = new DataView(glbBuffer.buffer, glbBuffer.byteOffset, glbBuffer.byteLength);
    if (glbBuffer.length < 12) return null;

    const magic = view.getUint32(0, true);
    if (magic !== 0x46546C67) return null;

    const version = view.getUint32(4, true);
    if (version !== 2) return null;

    const totalLen = view.getUint32(8, true);

    let offset = 12;
    let jsonStr = '';
    let jsonChunkSize = 0;
    let binChunkSize = 0;

    while (offset + 8 <= glbBuffer.length) {
        const chunkLen = view.getUint32(offset, true);
        const chunkType = view.getUint32(offset + 4, true);

        if (chunkType === 0x4E4F534A) {
            jsonChunkSize = chunkLen;
            const decoder = new TextDecoder('utf-8');
            jsonStr = decoder.decode(glbBuffer.slice(offset + 8, offset + 8 + Math.min(chunkLen, glbBuffer.length - offset - 8)));
        } else if (chunkType === 0x004E4942) {
            binChunkSize = chunkLen;
        }
        offset += 8 + ((chunkLen + 3) & ~3);
    }

    if (!jsonStr) return null;

    try {
        const json = JSON.parse(jsonStr);
        return {
            jsonChunkSize,
            binChunkSize,
            totalSizeBytes: totalLen,
            extensionsUsed: json.extensionsUsed || [],
            extensionsRequired: json.extensionsRequired || [],
            // KHR 扩展在 primitive 级别：meshes[0].primitives[0].extensions
            KHR_gaussian_splatting: json.meshes?.[0]?.primitives?.[0]?.extensions?.KHR_gaussian_splatting || null,
            KHR_gaussian_splatting_compression_spz_2: json.meshes?.[0]?.primitives?.[0]?.extensions?.KHR_gaussian_splatting?.extensions?.KHR_gaussian_splatting_compression_spz_2 || null,
        };
    } catch {
        return null;
    }
}

/**
 * 生成 JSON 报告字符串（镜像 CLI 端 ConversionResult::toJson）
 * @param {object} opts - 报告参数
 * @param {string} opts.fileName - SPZ 文件名
 * @param {number} opts.spzSizeBytes - SPZ 大小
 * @param {number} opts.spzVersion - SPZ 版本
 * @param {string} opts.compression - 压缩格式
 * @param {number} opts.glbSizeBytes - GLB 大小
 * @param {object} opts.glbInfo - GLB 解析结果（parseGlbJson 返回值）
 * @param {boolean} opts.success - 是否成功
 * @param {string} opts.errorMsg - 错误信息
 * @param {number} opts.timingMs - 耗时
 * @returns {string} JSON 字符串
 */
export function generateReportJson(opts) {
    const now = new Date();
    const pad2 = (n) => String(n).padStart(2, '0');
    const tz = -now.getTimezoneOffset();
    const tzSign = tz >= 0 ? '+' : '-';
    const tzHours = pad2(Math.abs(Math.trunc(tz / 60)));
    const tzMins = pad2(Math.abs(tz % 60));
    const timestamp = `${now.getFullYear()}-${pad2(now.getMonth() + 1)}-${pad2(now.getDate())}T${pad2(now.getHours())}:${pad2(now.getMinutes())}:${pad2(now.getSeconds())}${tzSign}${tzHours}${tzMins}`;

    const report = {
        file: opts.fileName,
        sizeBytes: opts.spzSizeBytes,
        spz: {
            version: opts.spzVersion || 0,
            compression: opts.compression || 'unknown',
        },
        glb: {
            magic: '0x46546C67',
            version: 2,
            jsonChunkSize: opts.glbInfo?.jsonChunkSize || 0,
            binChunkSize: opts.glbInfo?.binChunkSize || 0,
            totalSizeBytes: opts.glbInfo?.totalSizeBytes || 0,
            outputSizeBytes: opts.glbSizeBytes,
        },
        extensionsUsed: opts.glbInfo?.extensionsUsed || [],
        extensionsRequired: opts.glbInfo?.extensionsRequired || [],
        KHR_gaussian_splatting: opts.glbInfo?.KHR_gaussian_splatting ? {
            kernel: opts.glbInfo.KHR_gaussian_splatting.kernel || '',
            colorSpace: opts.glbInfo.KHR_gaussian_splatting.colorSpace || '',
            sortingMethod: opts.glbInfo.KHR_gaussian_splatting.sortingMethod || '',
            projection: opts.glbInfo.KHR_gaussian_splatting.projection || '',
        } : {},
        KHR_gaussian_splatting_compression_spz_2: opts.glbInfo?.KHR_gaussian_splatting_compression_spz_2 ? {
            bufferView: opts.glbInfo.KHR_gaussian_splatting_compression_spz_2.bufferView ?? 0,
            spzVersion: opts.glbInfo.KHR_gaussian_splatting_compression_spz_2.spzVersion ?? 0,
            compression: opts.glbInfo.KHR_gaussian_splatting_compression_spz_2.compression || '',
            coordinateSystem: opts.glbInfo.KHR_gaussian_splatting_compression_spz_2.coordinateSystem ?? -1,
        } : {},
        coordinateSystem: {
            found: (opts.glbInfo?.KHR_gaussian_splatting_compression_spz_2?.coordinateSystem ?? -1) >= 0,
            extensionId: '0xADBE0003',
            value: opts.glbInfo?.KHR_gaussian_splatting_compression_spz_2?.coordinateSystem ?? -1,
        },
        timestamp,
        generator: {
            name: 'spz2glb',
            version: '2.0.4',
            license: 'MIT',
            url: 'https://github.com/spz-ecosystem/spz2glb',
        },
        result: opts.success ? 'success' : 'failed',
        timingMs: opts.timingMs,
        timingMsDisplay: `${Math.round(opts.timingMs)} ms`,
        conversionMs: opts.conversionMs ?? null,
        conversionMsDisplay: opts.conversionMs != null ? `${Math.round(opts.conversionMs)} ms` : null,
    };

    // 失败时添加错误信息
    if (!opts.success && opts.errorMsg) {
        report.error = opts.errorMsg;
    }

    return JSON.stringify(report, null, 2);
}

