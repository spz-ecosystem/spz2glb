/**
 * High-Performance WebAssembly bindings for spz2glb
 * Uses native WebAssembly.instantiate with WASM-managed memory
 * Supports memory growth detection and reallocation
 */

function createSpz2GlbBindings(instance) {
    const exports = instance.exports;
    let currentMemory = exports.memory;

    // Helper to get current memory buffer (handles growth)
    function getMemoryBuffer() {
        // Check if memory was grown
        if (exports.memory !== currentMemory) {
            console.log('[WASM] Memory grew, updating reference');
            currentMemory = exports.memory;
        }
        return currentMemory.buffer;
    }

    function validateHeader(buffer) {
        const [ptr, size] = writeBuffer(buffer);
        const result = exports._spz2glb_validate_header(ptr, size);
        freeBuffer(ptr);
        return result;
    }

    function convert(spzBuffer) {
        const [inputPtr, inputSize] = writeBuffer(spzBuffer);
        const outSizePtr = exports._spz2glb_alloc(8);
        const resultPtr = exports._spz2glb_convert(inputPtr, inputSize, outSizePtr);
        freeBuffer(inputPtr);

        if (!resultPtr) {
            freeBuffer(outSizePtr);
            return null;
        }

        const heapU32 = new Uint32Array(getMemoryBuffer());
        const outSize = heapU32[outSizePtr / 4];
        freeBuffer(outSizePtr);

        if (!outSize) {
            exports._spz2glb_free(resultPtr);
            return null;
        }

        const result = readBuffer(resultPtr, outSize);
        exports._spz2glb_free(resultPtr);
        return result;
    }

    function writeBuffer(jsBuffer) {
        const size = jsBuffer.byteLength;
        const ptr = exports._spz2glb_alloc(size);
        if (!ptr) throw new Error('Memory allocation failed in WASM');
        const heap = new Uint8Array(getMemoryBuffer());
        heap.set(new Uint8Array(jsBuffer.buffer, jsBuffer.byteOffset, size), ptr);
        return [ptr, size];
    }

    function readBuffer(ptr, size) {
        const heap = new Uint8Array(getMemoryBuffer());
        return new Uint8Array(heap.slice(ptr, ptr + size));
    }

    function freeBuffer(ptr) {
        if (ptr) exports._spz2glb_free(ptr);
    }

    function getVersion() {
        const ptr = exports._spz2glb_alloc(12);
        exports._spz2glb_get_version(ptr, ptr + 4, ptr + 8);
        const heapU32 = new Uint32Array(getMemoryBuffer());
        const version = `${heapU32[ptr / 4]}.${heapU32[ptr / 4 + 1]}.${heapU32[ptr / 4 + 2]}`;
        freeBuffer(ptr);
        return version;
    }

    // Expose memory for smart memory manager
    function getMemory() {
        return exports.memory;
    }

    return { validateHeader, convert, getVersion, getMemory };
}

export async function loadSpz2Glb(wasmUrl, options = {}) {
    const response = await fetch(wasmUrl);
    if (!response.ok) throw new Error(`Failed to fetch WASM: ${response.status}`);
    const buffer = await response.arrayBuffer();

    // WASM 模块自己管理内存，我们只需要提供必要的导入
    const importObject = {
        env: {
            // 如果 WASM 需要内存增长回调，可以在这里添加
            emscripten_notify_memory_growth: function(index) {
                console.log('[WASM] Memory growth notification, index:', index);
            }
        }
    };

    const { instance } = await WebAssembly.instantiate(buffer, importObject);

    return createSpz2GlbBindings(instance);
}
