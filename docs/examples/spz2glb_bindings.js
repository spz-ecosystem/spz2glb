export async function loadSpz2Glb(wasmUrl, options = {}) {
    const moduleUrl = wasmUrl.replace(/\.wasm($|[?#])/, '.js$1');
    const { default: createModule } = await import(moduleUrl);

    const module = await createModule({
        ...options,
        print: options.print ?? ((text) => console.log('[WASM]', text)),
        printErr: options.printErr ?? ((text) => console.error('[WASM]', text)),
        locateFile: options.locateFile ?? ((path) => path.endsWith('.wasm') ? wasmUrl : path),
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

