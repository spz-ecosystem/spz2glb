/**
 * WebAssembly bindings for spz2glb using Emscripten JS glue code
 * Handles Emscripten internal imports correctly
 */

export async function loadSpz2Glb(wasmUrl, options = {}) {
    const loadOnce = async (url) => {
        const moduleUrl = url.replace('.wasm', '.js');
        const { default: createModule } = await import(moduleUrl);

        const module = await createModule({
            print: (text) => console.log('[WASM]', text),
            printErr: (text) => console.error('[WASM]', text),
        });

        if (!module || !module.HEAPU8 || !module.HEAPU8.buffer) {
            throw new Error('WASM runtime not initialized (HEAPU8 missing)');
        }

        const exports = module.asm || module;

        return {
            validateHeader: (buffer) => {
                const [ptr, size] = writeBuffer(module, buffer);
                const result = exports._spz2glb_validate_header(ptr, size);
                freeBuffer(module, ptr);
                return result;
            },

            convert: (spzBuffer) => {
                const [inputPtr, inputSize] = writeBuffer(module, spzBuffer);
                const outSizePtr = exports._spz2glb_alloc(8);
                const resultPtr = exports._spz2glb_convert(inputPtr, inputSize, outSizePtr);
                freeBuffer(module, inputPtr);

                if (!resultPtr) {
                    freeBuffer(module, outSizePtr);
                    return null;
                }

                const heapU32 = new Uint32Array(module.HEAPU8.buffer);
                const outSize = heapU32[outSizePtr / 4];
                freeBuffer(module, outSizePtr);

                if (!outSize) {
                    exports._spz2glb_free(resultPtr);
                    return null;
                }

                const result = readBuffer(module, resultPtr, outSize);
                exports._spz2glb_free(resultPtr);
                return result;
            },

            getVersion: () => {
                const ptr = exports._spz2glb_alloc(12);
                exports._spz2glb_get_version(ptr, ptr + 4, ptr + 8);
                const heapU32 = new Uint32Array(module.HEAPU8.buffer);
                const version = `${heapU32[ptr / 4]}.${heapU32[ptr / 4 + 1]}.${heapU32[ptr / 4 + 2]}`;
                freeBuffer(module, ptr);
                return version;
            },

            module: module
        };
    };

    try {
        return await loadOnce(wasmUrl);
    } catch (err) {
        const msg = String(err && err.message ? err.message : err);
        const shouldRetry = msg.includes('buffer') || msg.includes('HEAPU8') || msg.includes('_embind_register_function');
        if (!shouldRetry) {
            console.error('Failed to load WASM module:', err);
            throw err;
        }

        const sep = wasmUrl.includes('?') ? '&' : '?';
        const retryUrl = `${wasmUrl}${sep}v=${Date.now()}`;
        console.warn('Retrying WASM load with cache-busting URL:', retryUrl);
        return await loadOnce(retryUrl);
    }
}

function writeBuffer(module, jsBuffer) {
    const size = jsBuffer.byteLength;
    const ptr = module._malloc(size);
    if (!ptr) throw new Error('Memory allocation failed');
    module.HEAPU8.set(new Uint8Array(jsBuffer.buffer, jsBuffer.byteOffset, size), ptr);
    return [ptr, size];
}

function readBuffer(module, ptr, size) {
    return new Uint8Array(module.HEAPU8.buffer.slice(ptr, ptr + size));
}

function freeBuffer(module, ptr) {
    if (ptr) module._free(ptr);
}
