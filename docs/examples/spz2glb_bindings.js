/**
 * WebAssembly bindings for spz2glb using Emscripten JS glue code
 * Handles Emscripten internal imports correctly
 */

export async function loadSpz2Glb(wasmUrl, options = {}) {
    // Import the Emscripten-generated JS glue code
    const moduleUrl = wasmUrl.replace('.wasm', '.js');
    
    try {
        // Dynamic import of the Emscripten module
        const { default: createModule } = await import(moduleUrl);
        
        // Create module instance
        const module = await createModule({
            print: (text) => console.log('[WASM]', text),
            printErr: (text) => console.error('[WASM]', text),
        });
        
        // Get exported functions
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
            
            // Expose module for advanced usage
            module: module
        };
    } catch (err) {
        console.error('Failed to load WASM module:', err);
        throw err;
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
