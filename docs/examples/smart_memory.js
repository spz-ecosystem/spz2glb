class SmartMemoryManager {
    constructor() {
        this.deviceInfo = null;
        this.memoryLimit = 512;
        this.recommendedMaxFileSize = 32;
        this.hardMaxFileSize = 48;
    }

    detectDevice() {
        const ua = navigator.userAgent;
        const deviceMemory = navigator.deviceMemory ?? null;
        const cores = navigator.hardwareConcurrency || 4;
        const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua);
        const isTablet = /iPad|Android(?!.*Mobile)|Tablet/i.test(ua);

        let type = 'desktop';
        if (isMobile && !isTablet) {
            type = 'mobile';
        } else if (isTablet) {
            type = 'tablet';
        }

        let tier = 'medium';
        if (deviceMemory !== null && deviceMemory <= 2) {
            tier = 'low';
        } else if (type === 'desktop' && cores >= 8 && (deviceMemory === null || deviceMemory >= 8)) {
            tier = 'high';
        } else if (type !== 'desktop' || cores <= 4 || (deviceMemory !== null && deviceMemory <= 4)) {
            tier = 'medium';
        }

        this.deviceInfo = { type, tier, cores, deviceMemory, isMobile, isTablet };
        return this.deviceInfo;
    }

    detectMemoryLimit() {
        if (!this.deviceInfo) {
            this.detectDevice();
        }

        let limit = 512;
        if (navigator.deviceMemory) {
            limit = Math.floor(navigator.deviceMemory * 256);
        }
        if (performance.memory?.jsHeapSizeLimit) {
            const heapLimit = Math.floor(performance.memory.jsHeapSizeLimit / (1024 * 1024));
            limit = Math.min(limit, heapLimit);
        }

        if (this.deviceInfo.tier === 'low') {
            limit = Math.min(limit, 256);
        } else if (this.deviceInfo.tier === 'medium') {
            limit = Math.min(limit, 512);
        } else {
            limit = Math.min(limit, 1024);
        }

        this.memoryLimit = Math.max(limit, 128);
        return this.memoryLimit;
    }

    calculateFileBudget() {
        if (!this.deviceInfo) {
            this.detectDevice();
        }
        if (!this.memoryLimit) {
            this.detectMemoryLimit();
        }

        const budgets = {
            low: { recommended: 16, hardMax: 24 },
            medium: { recommended: 32, hardMax: 48 },
            high: { recommended: 48, hardMax: 64 },
        };

        const budget = budgets[this.deviceInfo.tier] || budgets.medium;
        const memoryRecommended = Math.max(8, Math.floor(this.memoryLimit * 0.25));
        const memoryHardMax = Math.max(16, Math.floor(this.memoryLimit * 0.35));

        this.recommendedMaxFileSize = Math.min(budget.recommended, memoryRecommended);
        this.hardMaxFileSize = Math.max(
            this.recommendedMaxFileSize + 8,
            Math.min(budget.hardMax, memoryHardMax),
        );

        return {
            recommendedMaxFileSize: this.recommendedMaxFileSize,
            hardMaxFileSize: this.hardMaxFileSize,
        };
    }

    getRecommendation(fileSizeMB) {
        if (!this.deviceInfo) {
            this.detectDevice();
        }
        this.calculateFileBudget();

        if (fileSizeMB <= this.recommendedMaxFileSize) {
            return {
                state: 'standard',
                canHandle: true,
                allowConversion: true,
                message: '适合当前浏览器档位，可直接转换。',
                recommendedMaxSize: this.recommendedMaxFileSize,
                hardMaxSize: this.hardMaxFileSize,
                cliCommand: 'spz2glb input.spz output.glb',
            };
        }

        if (fileSizeMB <= this.hardMaxFileSize) {
            return {
                state: 'light',
                canHandle: true,
                allowConversion: true,
                message: '文件接近浏览器安全上限，建议优先使用桌面浏览器或 CLI。',
                recommendedMaxSize: this.recommendedMaxFileSize,
                hardMaxSize: this.hardMaxFileSize,
                cliCommand: 'spz2glb input.spz output.glb',
            };
        }

        return {
            state: 'reject',
            canHandle: false,
            allowConversion: false,
            message: '文件超过浏览器安全上限，请改用 CLI。',
            recommendedMaxSize: this.recommendedMaxFileSize,
            hardMaxSize: this.hardMaxFileSize,
            cliCommand: 'spz2glb input.spz output.glb',
        };
    }

    getWasmMemoryConfig() {
        if (!this.deviceInfo) {
            this.detectDevice();
        }
        if (!this.memoryLimit) {
            this.detectMemoryLimit();
        }

        const mb = 1024 * 1024;
        const limitBytes = this.memoryLimit * mb;

        let initial = 128 * mb;
        let maximum = 512 * mb;
        if (this.deviceInfo.tier === 'low') {
            initial = 64 * mb;
            maximum = 256 * mb;
        } else if (this.deviceInfo.tier === 'high') {
            initial = 256 * mb;
            maximum = 768 * mb;
        }

        maximum = Math.min(maximum, limitBytes);
        initial = Math.min(initial, maximum);

        const memory = new WebAssembly.Memory({
            initial: Math.floor(initial / 65536),
            maximum: Math.floor(maximum / 65536),
        });

        return { memory, initial, maximum };
    }

    getInfo() {
        const device = this.detectDevice();
        const memoryLimit = this.detectMemoryLimit();
        const { recommendedMaxFileSize, hardMaxFileSize } = this.calculateFileBudget();

        return {
            device,
            memoryLimit,
            recommendedMaxFileSize,
            hardMaxFileSize,
            getRecommendation: this.getRecommendation.bind(this),
            getWasmMemoryConfig: this.getWasmMemoryConfig.bind(this),
        };
    }
}

export const memoryManager = new SmartMemoryManager();
export default SmartMemoryManager;
