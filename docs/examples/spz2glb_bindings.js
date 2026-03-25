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
            const chunkSize = options.chunkSize ?? 1024 * 1024;
            const inputPtr = reserveInput(module, file.size);
            try {
                await writeFileToReservedInput(module, file, inputPtr, chunkSize);
                return convertReservedInput(module, file.size);
            } finally {
                releaseReservedInput(module);
            }
        },

        getVersion() {
            const ptr = module._malloc(12);
            try {
                module._spz2glb_get_version(ptr, ptr + 4, ptr + 8);
                const major = module.getValue(ptr, 'i32');
                const minor = module.getValue(ptr + 4, 'i32');
                const patch = module.getValue(ptr + 8, 'i32');
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
    const outPtrPtr = module._malloc(4);
    const outSizePtr = module._malloc(4);

    try {
        const ok = module._spz2glb_convert_reserved_input(size, outPtrPtr, outSizePtr);
        if (!ok) {
            return null;
        }

        const resultPtr = module.getValue(outPtrPtr, 'i32') >>> 0;
        const outSize = module.getValue(outSizePtr, 'i32') >>> 0;
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
        get bytes() {
            if (released) {
                throw new Error('WASM 输出已释放');
            }
            return getHeap(module).subarray(ptr, ptr + size);
        },
        release() {
            if (!released) {
                module._spz2glb_release_output(ptr);
                released = true;
            }
        },
    };
}

async function writeFileToReservedInput(module, file, inputPtr, chunkSize) {
    let offset = 0;
    while (offset < file.size) {
        const end = Math.min(offset + chunkSize, file.size);
        const chunk = new Uint8Array(await file.slice(offset, end).arrayBuffer());
        getHeap(module).set(chunk, inputPtr + offset);
        offset = end;
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

