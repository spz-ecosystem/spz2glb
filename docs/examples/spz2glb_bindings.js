/**
 * High-Performance WebAssembly bindings for spz2glb
 *
 * Uses WebAssembly.instantiate directly instead of Embind
 * for direct JS <-> WASM memory access
 *
 * Usage:
 *   import { loadSpz2Glb } from './spz2glb_bindings.js';
 *   const spz2glb = await loadSpz2Glb('./spz2glb.wasm');
 *   const glbBuffer = spz2glb.convert(spzBuffer);
 */

function createSpz2GlbBindings(wasmInstance) {
    const exports = wasmInstance.exports;
    const memory = exports.memory;

    function validateHeader(buffer) {
        const [ptr, size] = writeBuffer(buffer);
        const result = exports.spz2glb_validate_header(ptr, size);
        freeBuffer(ptr);
        return result;
    }

    function convert(spzBuffer) {
        const [inputPtr, inputSize] = writeBuffer(spzBuffer);

        const outSizePtr = exports.spz2glb_alloc(8);
        const resultPtr = exports.spz2glb_convert(inputPtr, inputSize, outSizePtr);

        freeBuffer(inputPtr);

        if (!resultPtr) {
            freeBuffer(outSizePtr);
            return null;
        }

        const heapU32 = new Uint32Array(memory.buffer);
        const outSize = heapU32[outSizePtr / 4];

        freeBuffer(outSizePtr);

        if (!outSize) {
            exports.spz2glb_free(resultPtr);
            return null;
        }

        const result = readBuffer(resultPtr, outSize);
        exports.spz2glb_free(resultPtr);

        return result;
    }

    function writeBuffer(jsBuffer) {
        const size = jsBuffer.byteLength;
        const ptr = exports.spz2glb_alloc(size);

        const heap = new Uint8Array(memory.buffer);
        heap.set(new Uint8Array(jsBuffer.buffer, jsBuffer.byteOffset, size), ptr);

        return [ptr, size];
    }

    function readBuffer(ptr, size) {
        const heap = new Uint8Array(memory.buffer);
        return new Uint8Array(heap.slice(ptr, ptr + size));
    }

    function freeBuffer(ptr) {
        if (ptr) {
            exports.spz2glb_free(ptr);
        }
    }

    function getVersion() {
        const versionPtr = exports.spz2glb_alloc(12);
        exports.spz2glb_get_version(versionPtr, versionPtr + 4, versionPtr + 8);

        const heapU32 = new Uint32Array(memory.buffer);
        const major = heapU32[versionPtr / 4];
        const minor = heapU32[versionPtr / 4 + 1];
        const patch = heapU32[versionPtr / 4 + 2];

        freeBuffer(versionPtr);
        return `${major}.${minor}.${patch}`;
    }

    function getMemoryStats() {
        const statsPtr = exports.spz2glb_alloc(40);
        exports.spz2glb_get_memory_stats(statsPtr);

        const heapU32 = new Uint32Array(memory.buffer);
        const stats = {
            peak_usage_bytes: Number(heapU32[statsPtr / 4]) + (Number(heapU32[statsPtr / 4 + 1]) << 32),
            current_usage_bytes: Number(heapU32[statsPtr / 4 + 2]) + (Number(heapU32[statsPtr / 4 + 3]) << 32),
            total_allocations: heapU32[statsPtr / 4 + 4],
            total_frees: heapU32[statsPtr / 4 + 5],
            failed_allocations: heapU32[statsPtr / 4 + 6]
        };

        freeBuffer(statsPtr);
        return stats;
    }

    function resetMemoryStats() {
        exports.spz2glb_reset_memory_stats();
    }

    return {
        validateHeader,
        convert,
        getVersion,
        getMemoryStats,
        resetMemoryStats,
        exports
    };
}

export async function loadSpz2Glb(wasmUrl, options = {}) {
    const response = await fetch(wasmUrl);
    if (!response.ok) {
        throw new Error(`Failed to fetch WASM: ${response.status}`);
    }
    const buffer = await response.arrayBuffer();

    const memory = options.memory || new WebAssembly.Memory({
        initial: 64,
        maximum: 1024
    });

    const { instance } = await WebAssembly.instantiate(buffer, {
        env: {
            memory
        }
    });

    return createSpz2GlbBindings(instance);
}
