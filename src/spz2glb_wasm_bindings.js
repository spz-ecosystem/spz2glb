/**
 * High-Performance WebAssembly bindings for spz2glb
 *
 * Uses WebAssembly.instantiate directly instead of Embind
 * for zero-copy JS <-> WASM memory access
 *
 * Usage:
 *   const { instance } = await WebAssembly.instantiate(wasmBuffer);
 *   const spz2glb = createSpz2GlbBindings(instance);
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

        let outSizePtr = 0;
        const resultPtr = exports.spz2glb_convert(inputPtr, inputSize, outSizePtr);

        freeBuffer(inputPtr);

        if (!resultPtr || !outSizePtr) {
            return null;
        }

        const result = readBuffer(resultPtr, outSizePtr);
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

    function getMemoryStats() {
        const statsPtr = exports.spz2glb_get_memory_stats();
        const stats = new DataView(memory.buffer, statsPtr, 32);
        return {
            peak_usage: stats.getUint32(0, true),
            current_usage: stats.getUint32(8, true),
            total_allocations: stats.getUint32(16, true),
            total_frees: stats.getUint32(24, true)
        };
    }

    function getVersion() {
        const major = { value: 0 };
        const minor = { value: 0 };
        const patch = { value: 0 };
        exports.spz2glb_get_version(major, minor, patch);
        return `${major.value}.${minor.value}.${patch.value}`;
    }

    return {
        validateHeader,
        convert,
        getMemoryStats,
        getVersion,
        exports
    };
}

async function loadSpz2GlbWasm(wasmUrl) {
    const response = await fetch(wasmUrl);
    const buffer = await response.arrayBuffer();
    const { instance } = await WebAssembly.instantiate(buffer, {
        env: {
            memory: new WebAssembly.Memory({ initial: 64, maximum: 1024 })
        }
    });
    return createSpz2GlbBindings(instance);
}

export { createSpz2GlbBindings, loadSpz2GlbWasm };
