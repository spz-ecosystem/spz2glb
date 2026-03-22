export interface ValidationResult {
  passed: boolean;
  layer: 1 | 2 | 3;
  message?: string;
  details?: Record<string, unknown>;
}

export interface AllLayersResult {
  overall: boolean;
  layers: ValidationResult[];
  totalTime: number;
}

export interface SpzVerifyModule {
  layer1Validate(glbData: Uint8Array): ValidationResult;
  layer2Validate(spzData: Uint8Array, glbData: Uint8Array): ValidationResult;
  layer3Validate(spzData: Uint8Array, glbData: Uint8Array): ValidationResult;
  validateAll(spzData: Uint8Array, glbData: Uint8Array): AllLayersResult;
}

export default function createSpzVerifyModule(): Promise<SpzVerifyModule>;
