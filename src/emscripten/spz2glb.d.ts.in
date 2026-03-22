export interface Spz2GlbMemoryStats {
  peak_usage_bytes: number;
  current_usage_bytes: number;
  total_allocations: number;
  total_frees: number;
  failed_allocations: number;
}

export interface Spz2GlbBindings {
  convert(spzBuffer: Uint8Array): Uint8Array | null;
  validateHeader(buffer: Uint8Array): boolean;
  getVersion(): string;
  getMemoryStats(): Spz2GlbMemoryStats;
  resetMemoryStats(): void;
  exports: WebAssembly.Exports;
}

export interface SpzVerifyBindings {
  validateHeader(data: Uint8Array): boolean;
  computeMd5(data: Uint8Array): Uint8Array;
  getMemoryStats(): Spz2GlbMemoryStats;
  exports: WebAssembly.Exports;
}

export interface LoadOptions {
  memory?: WebAssembly.Memory;
}

export async function loadSpz2Glb(wasmUrl: string, options?: LoadOptions): Promise<Spz2GlbBindings>;

export async function loadSpzVerify(wasmUrl: string, options?: LoadOptions): Promise<SpzVerifyBindings>;
