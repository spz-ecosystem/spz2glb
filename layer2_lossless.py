#!/usr/bin/env python3
"""
Layer 2: Binary Lossless Verification
SPZ → GLB → Extract raw stream → Compare hashes
"""

import struct
import hashlib
import json
import subprocess
import sys

def md5_hash(data):
    return hashlib.md5(data).hexdigest()

def extract_raw_stream_from_glb(glb_path):
    with open(glb_path, 'rb') as f:
        # GLB header (12 bytes)
        magic = struct.unpack('<I', f.read(4))[0]
        if magic != 0x46546C67:
            return None, "Invalid GLB"
        
        # Skip version and length
        f.read(8)
        
        # JSON chunk
        json_len = struct.unpack('<I', f.read(4))[0]
        json_type = struct.unpack('<I', f.read(4))[0]
        json_data = f.read(json_len).decode('utf-8').rstrip('\x00')
        
        # JSON padding
        f.read((4 - (json_len % 4)) % 4)
        
        # Parse JSON for buffer size
        gltf = json.loads(json_data)
        buffer_size = gltf['buffers'][0]['byteLength']
        
        # Skip BIN chunk header (8 bytes: 4 length + 4 type)
        bin_chunk_length = struct.unpack('<I', f.read(4))[0]
        bin_chunk_type = struct.unpack('<I', f.read(4))[0]
        
        # Read actual binary data
        bin_data = f.read(buffer_size)
        
        return bin_data, None

def verify_lossless(spz_path, glb_path, spz2glb_tool):
    print("=" * 60)
    print("Layer 2: Binary Lossless Verification")
    print("=" * 60)
    
    # Convert
    print(f"\n[1] Converting SPZ → GLB...")
    result = subprocess.run([spz2glb_tool, spz_path, glb_path], 
                         capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] {result.stderr}")
        return False
    print(f"    ✓ Done")
    
    # Read original
    print(f"\n[2] Reading original SPZ...")
    with open(spz_path, 'rb') as f:
        original = f.read()
    print(f"    Size: {len(original)} bytes")
    print(f"    MD5:  {md5_hash(original)}")
    
    # Extract from GLB
    print(f"\n[3] Extracting from GLB...")
    extracted, error = extract_raw_stream_from_glb(glb_path)
    if error:
        print(f"    ✗ {error}")
        return False
    print(f"    Size: {len(extracted)} bytes")
    print(f"    MD5:  {md5_hash(extracted)}")
    
    # Compare
    print(f"\n[4] Comparing...")
    if original == extracted:
        print(f"\n[PASSED] Binary lossless! 100% match!")
        print(f"    MD5: {md5_hash(original)}")
        return True
    else:
        print(f"\n[FAILED]")
        print(f"    Original: {len(original)} bytes")
        print(f"    Extracted: {len(extracted)} bytes")
        return False

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("Usage: layer2_lossless.py <input.spz> <output.glb> <spz2glb_tool>")
        sys.exit(1)
    
    success = verify_lossless(sys.argv[1], sys.argv[2], sys.argv[3])
    sys.exit(0 if success else 1)
