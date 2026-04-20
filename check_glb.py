import json
import struct
import sys

def read_glb(path):
    with open(path, 'rb') as f:
        # Header
        magic = f.read(4)
        if magic != b'glTF':
            print(f"Not a GLB file: magic={magic}")
            return
        version = struct.unpack('<I', f.read(4))[0]
        length = struct.unpack('<I', f.read(4))[0]
        print(f"GLB version {version}, length {length}")
        
        # JSON chunk
        chunk_length = struct.unpack('<I', f.read(4))[0]
        chunk_type = f.read(4)
        if chunk_type != b'JSON':
            print(f"First chunk not JSON: {chunk_type}")
            return
        json_data = f.read(chunk_length)
        gltf = json.loads(json_data)
        
        # Check extensions
        print("\n=== Extensions ===")
        print(f"extensionsUsed: {gltf.get('extensionsUsed', [])}")
        print(f"extensionsRequired: {gltf.get('extensionsRequired', [])}")
        
        # Check meshes
        if 'meshes' in gltf:
            print(f"\n=== Meshes ({len(gltf['meshes'])}) ===")
            for i, mesh in enumerate(gltf['meshes']):
                print(f"Mesh {i}: {mesh.get('name', 'unnamed')}")
                if 'primitives' in mesh:
                    for j, prim in enumerate(mesh['primitives']):
                        print(f"  Primitive {j}:")
                        if 'extensions' in prim:
                            print(f"    Extensions: {list(prim['extensions'].keys())}")
                            if 'KHR_gaussian_splatting' in prim['extensions']:
                                print(f"    ✓ KHR_gaussian_splatting found!")
                                ext = prim['extensions']['KHR_gaussian_splatting']
                                print(f"      Data: {ext}")
                        else:
                            print(f"    No extensions")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python check_glb.py <glb_file>")
        sys.exit(1)
    read_glb(sys.argv[1])