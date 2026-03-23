/**
 * High-Performance WebAssembly bindings for spz2glb
 * Uses WebAssembly.instantiate directly with external memory
 */

function createSpz2GlbBindings(instance, memory) {
    const exports = instance.exports;

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

        const heapU32 = new Uint32Array(memory.buffer);
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
        const heap = new Uint8Array(memory.buffer);
        heap.set(new Uint8Array(jsBuffer.buffer, jsBuffer.byteOffset, size), ptr);
        return [ptr, size];
    }

    function readBuffer(ptr, size) {
        const heap = new Uint8Array(memory.buffer);
        return new Uint8Array(heap.slice(ptr, ptr + size));
    }

    function freeBuffer(ptr) {
        if (ptr) exports._spz2glb_free(ptr);
    }

    function getVersion() {
        const ptr = exports._spz2glb_alloc(12);
        exports._spz2glb_get_version(ptr, ptr + 4, ptr + 8);
        const heapU32 = new Uint32Array(memory.buffer);
        const version = `${heapU32[ptr / 4]}.${heapU32[ptr / 4 + 1]}.${heapU32[ptr / 4 + 2]}`;
        freeBuffer(ptr);
        return version;
    }

    return { validateHeader, convert, getVersion };
}

export async function loadSpz2Glb(wasmUrl, options = {}) {
    const response = await fetch(wasmUrl);
    if (!response.ok) throw new Error(`Failed to fetch WASM: ${response.status}`);
    const buffer = await response.arrayBuffer();

    // 创建内存（使用传入的配置或默认值）
    const initialPages = options.initialPages || options.memory?.initial || 1024;  // 64MB
    const maximumPages = options.maximumPages || options.memory?.maximum || 16384; // 1GB
    const memory = new WebAssembly.Memory({
        initial: initialPages,
        maximum: maximumPages
    });

    const { instance } = await WebAssembly.instantiate(buffer, {
        env: { memory }
    });

    return createSpz2GlbBindings(instance, memory);
}
