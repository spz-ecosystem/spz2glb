#!/usr/bin/env python3
"""Layer 1: GLB Structure & SPZ_2 Specification Validation"""

import struct
import json
import os
import sys

def validate(glb_path):
    print("=" * 60)
    print("Layer 1: GLB Structure & SPZ_2 Validation")
    print("=" * 60)
    
    if not os.path.exists(glb_path):
        print(f"[ERROR] File not found: {glb_path}")
        return False
    
    with open(glb_path, 'rb') as f:
        magic = struct.unpack('<I', f.read(4))[0]
        version = struct.unpack('<I', f.read(4))[0]
        length = struct.unpack('<I', f.read(4))[0]
        
        json_len = struct.unpack('<I', f.read(4))[0]
        json_type = struct.unpack('<I', f.read(4))[0]
        json_data = f.read(json_len).decode('utf-8').rstrip('\x00')
        
        gltf = json.loads(json_data)
    
    passed = 0
    total = 7
    errors = []
    
    # 1. Magic
    if magic == 0x46546C67:
        print("    ✓ Magic: glTF")
        passed += 1
    
    # 2. Version
    if version == 2:
        print("    ✓ Version: 2")
        passed += 1
    
    # 3. extensionsUsed
    ext_used = gltf.get('extensionsUsed', [])
    for ext in ['KHR_gaussian_splatting', 'KHR_gaussian_splatting_compression_spz_2']:
        if ext in ext_used:
            print(f"    ✓ {ext}")
            passed += 1
    
    # 4. buffers
    buffers = gltf.get('buffers', [])
    if len(buffers) == 1:
        print(f"    ✓ buffers[0].byteLength: {buffers[0]['byteLength']}")
        passed += 1
    
    # 5. mesh.primitive.attributes (compression mode = empty)
    meshes = gltf.get('meshes', [])
    if meshes and meshes[0].get('primitives'):
        attrs = meshes[0]['primitives'][0].get('attributes', {})
        if len(attrs) == 0:
            print("    ✓ attributes: empty (compression stream mode)")
            passed += 1
    
    # 6. accessors (compression mode = 0)
    accessors = gltf.get('accessors', [])
    if len(accessors) == 0:
        print("    ✓ accessors: 0 (compression stream mode)")
        passed += 1
    
    print(f"\nPassed: {passed}/{total}")
    
    if errors:
        for e in errors:
            print(f"  ERROR: {e}")
        return False
    
    if passed == total:
        print("\n[PASSED] All validation checks passed!")
        return True
    else:
        return False

if __name__ == '__main__':
    glb = sys.argv[1] if len(sys.argv) > 1 else 'build/test.glb'
    success = validate(glb)
    sys.exit(0 if success else 1)
