/**
 * High-Performance WebAssembly bindings for spz2glb
 * Handles Emscripten compressed import names
 */

function createSpz2GlbBindings(instance) {
    const exports = instance.exports;
    let currentMemory = exports.memory;

    function getMemoryBuffer() {
        if (exports.memory !== currentMemory) {
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
        if (!ptr) throw new Error('Memory allocation failed');
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

    return { validateHeader, convert, getVersion };
}

export async function loadSpz2Glb(wasmUrl, options = {}) {
    const response = await fetch(wasmUrl);
    if (!response.ok) throw new Error(`Failed to fetch WASM: ${response.status}`);
    const buffer = await response.arrayBuffer();

    // Build import object with compressed names
    // Emscripten may compress "env" to "a", "memory" to "b", etc.
    const importObject = {
        env: {},
        a: {}  // Handle compressed "env" -> "a"
    };

    try {
        const { instance } = await WebAssembly.instantiate(buffer, importObject);
        return createSpz2GlbBindings(instance);
    } catch (err) {
        console.error('WASM instantiation failed:', err);
        
        // If failed, try to analyze what imports are actually needed
        const wasmModule = await WebAssembly.compile(buffer);
        const imports = WebAssembly.Module.imports(wasmModule);
        console.log('WASM imports:');
        imports.forEach((imp, i) => {
            console.log(`  ${i}: ${imp.module}.${imp.name} (${imp.kind})`);
        });
        
        throw err;
    }
}
