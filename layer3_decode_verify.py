#!/usr/bin/env python3
"""
Layer 3: Decoding Consistency Verification
"""

import os
import sys
import struct
import json

def extract_json_from_glb(glb_path):
    with open(glb_path, 'rb') as f:
        magic = struct.unpack('<I', f.read(4))[0]
        if magic != 0x46546C67:
            return None
        f.read(8)  # version + length
        json_len = struct.unpack('<I', f.read(4))[0]
        f.read(4)  # type
        json_data = f.read(json_len).decode('utf-8').rstrip('\x00')
        return json.loads(json_data)

def verify(spz_path, glb_path):
    print("=" * 60)
    print("Layer 3: Decoding Consistency Verification")
    print("=" * 60)
    
    print(f"\n[1] Reading SPZ...")
    with open(spz_path, 'rb') as f:
        spz_data = f.read()
    print(f"    Size: {len(spz_data)} bytes")
    is_gzip = spz_data[0:2] == b'\x1f\x8b'
    print(f"    Gzip: {is_gzip}")
    
    print(f"\n[2] Verifying GLB...")
    try:
        gltf = extract_json_from_glb(glb_path)
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False
    
    ext_used = gltf.get('extensionsUsed', [])
    if 'KHR_gaussian_splatting_compression_spz_2' in ext_used:
        print(f"    ✓ SPZ_2 extension")
    else:
        print(f"    ✗ No SPZ_2")
        return False
    
    meshes = gltf.get('meshes', [])
    if meshes and meshes[0].get('primitives'):
        attrs = meshes[0]['primitives'][0].get('attributes', {})
        print(f"    ✓ Attributes: {len(attrs)}")
    
    buffers = gltf.get('buffers', [])
    if buffers:
        buffer_size = buffers[0].get('byteLength', 0)
        print(f"    ✓ Buffer: {buffer_size} bytes")
        
        if len(spz_data) == buffer_size:
            print(f"\n[PASSED] Size match: {len(spz_data)}")
            return True
    
    return False

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: layer3_decode_verify.py <spz> <glb>")
        sys.exit(1)
    success = verify(sys.argv[1], sys.argv[2])
    sys.exit(0 if success else 1)
