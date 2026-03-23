/**
 * Smart Memory Manager for WebAssembly
 * 
 * Features:
 * - Detects device capabilities (mobile/PC, memory limits)
 * - Auto-adjusts WASM memory configuration
 * - Provides file size recommendations
 * - Redirects large files to CLI version
 */

class SmartMemoryManager {
    constructor() {
        this.deviceInfo = null;
        this.memoryLimit = 512;
        this.recommendedMaxFileSize = 100;
    }

    detectDevice() {
        const ua = navigator.userAgent;
        const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua);
        const isTablet = /iPad|Android(?!.*Mobile)|Tablet/i.test(ua);
        const cores = navigator.hardwareConcurrency || 4;
        const isLowEnd = cores <= 4 || isMobile;

        let tier, type;
        if (isMobile && !isTablet) {
            tier = isLowEnd ? 'low' : 'medium';
            type = 'mobile';
        } else if (isTablet) {
            tier = 'medium';
            type = 'tablet';
        } else {
            tier = isLowEnd ? 'medium' : 'high';
            type = 'desktop';
        }

        this.deviceInfo = { type, tier, cores, isMobile, isLowEnd };
        return this.deviceInfo;
    }

    detectMemoryLimit() {
        let limit = 512;

        if (performance.memory) {
            const heapLimit = performance.memory.jsHeapSizeLimit;
            limit = Math.floor(heapLimit / (1024 * 1024));
        }

        if (this.deviceInfo) {
            switch (this.deviceInfo.tier) {
                case 'low':
                    limit = Math.min(limit, 256);
                    break;
                case 'medium':
                    limit = Math.min(limit, 512);
                    break;
                case 'high':
                    limit = Math.min(limit, 1024);
                    break;
            }
        }

        this.memoryLimit = limit;
        return limit;
    }

    calculateSafeFileSize() {
        if (!this.deviceInfo) this.detectDevice();
        if (!this.memoryLimit) this.detectMemoryLimit();

        const maxSafeSize = Math.floor(this.memoryLimit * 0.4);

        switch (this.deviceInfo.tier) {
            case 'low':
                this.recommendedMaxFileSize = Math.min(maxSafeSize, 50);
                break;
            case 'medium':
                this.recommendedMaxFileSize = Math.min(maxSafeSize, 100);
                break;
            case 'high':
                this.recommendedMaxFileSize = Math.min(maxSafeSize, 200);
                break;
        }

        return this.recommendedMaxFileSize;
    }

    canHandleFile(fileSizeMB) {
        if (!this.recommendedMaxFileSize) this.calculateSafeFileSize();
        return fileSizeMB <= this.recommendedMaxFileSize;
    }

    getRecommendation(fileSizeMB) {
        if (!this.deviceInfo) this.detectDevice();
        if (!this.recommendedMaxFileSize) this.calculateSafeFileSize();

        if (fileSizeMB <= this.recommendedMaxFileSize) {
            return { canHandle: true };
        }

        const isLarge = fileSizeMB > this.recommendedMaxFileSize * 2;

        return {
            canHandle: false,
            message: isLarge
                ? '文件过大，建议使用命令行版本 spz2glb 处理'
                : '此文件较大，建议使用桌面版浏览器处理',
            cliCommand: `spz2glb input.spz output.glb`,
            maxSize: this.recommendedMaxFileSize
        };
    }

    getWasmMemoryConfig() {
        if (!this.deviceInfo) this.detectDevice();
        if (!this.memoryLimit) this.detectMemoryLimit();

        const mb = 1024 * 1024;
        const limitBytes = this.memoryLimit * mb;

        let initial, maximum;

        switch (this.deviceInfo.tier) {
            case 'low':
                initial = 64 * mb;
                maximum = Math.min(256 * mb, limitBytes);
                break;
            case 'medium':
                initial = 128 * mb;
                maximum = Math.min(512 * mb, limitBytes);
                break;
            case 'high':
            default:
                initial = 256 * mb;
                maximum = Math.min(1024 * mb, limitBytes);
                break;
        }

        const memory = new WebAssembly.Memory({
            initial: Math.floor(initial / 65536),
            maximum: Math.floor(maximum / 65536)
        });

        return { memory, initial, maximum };
    }

    getInfo() {
        const device = this.detectDevice();
        const limit = this.detectMemoryLimit();
        const safe = this.calculateSafeFileSize();

        return {
            device,
            memoryLimit: limit,
            recommendedMaxFileSize: safe,
            canHandle: this.canHandleFile.bind(this),
            getRecommendation: this.getRecommendation.bind(this),
            getWasmMemoryConfig: this.getWasmMemoryConfig.bind(this)
        };
    }
}

export const memoryManager = new SmartMemoryManager();
export default SmartMemoryManager;
