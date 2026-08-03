#!/usr/bin/env node
/**
 * Synthetic SPZ fixture generator for CI testing.
 *
 * Generates minimal deterministic SPZ v3/v4 files with known content,
 * for use in CI pipelines to replace externally-mounted fixed samples.
 *
 * Outputs:
 *   --out <dir>   Output directory for generated .spz files (default: ./dist/fixtures)
 *   --manifest    Also write SHA-256 manifest to stdout (default: true)
 *
 * Output files:
 *   minimal_v3.spz     - 0 points, v3, gzip
 *   single_point_v3.spz - 1 point,  v3, gzip
 *   ten_points_v3.spz   - 10 points, v3, gzip
 *   v4_header_only.spz  - 0 points, v4, no TOC (32B header only)
 */

import { mkdirSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { gzipSync } from 'node:zlib';
import { join, resolve } from 'node:path';

// ---------- SPZ v3 16-byte header ----------
function buildV3Header(numPoints, shDegree = 0, fractionalBits = 8, flags = 0) {
  const buf = Buffer.alloc(16);
  buf.writeUInt32LE(0x5053474e, 0);  // magic "NGSP"
  buf.writeUInt32LE(3, 4);            // version = 3
  buf.writeUInt32LE(numPoints, 8);    // numPoints
  buf.writeUInt8(shDegree, 12);
  buf.writeUInt8(fractionalBits, 13);
  buf.writeUInt8(flags, 14);
  buf.writeUInt8(0, 15);              // reserved (padding)
  return buf;
}

// ---------- SPZ v4 32-byte header ----------
function buildV4Header(numPoints, numStreams = 0, shDegree = 0, fractionalBits = 12, flags = 0) {
  const buf = Buffer.alloc(32);
  buf.writeUInt32LE(0x5053474e, 0);  // magic "NGSP"
  buf.writeUInt32LE(4, 4);            // version = 4
  buf.writeUInt32LE(numPoints, 8);    // numPoints
  buf.writeUInt8(shDegree, 12);
  buf.writeUInt8(fractionalBits, 13);
  buf.writeUInt8(flags, 14);
  buf.writeUInt8(numStreams, 15);     // numStreams
  buf.writeUInt32LE(0, 16);           // toc_byte_offset = 0 (no TOC for 0 streams)
  // reserved[12]: all zeros (bytes 20-31)
  return buf;
}

// ---------- Minimal Gaussian point data ----------
// v3+ per-point layout (from SPZ spec):
//   positions:  12 bytes (3 × float32 LE)
//   scale:      1 byte  (log scale, quantized)
//   color:      3 bytes (SH DC RGB, normalized)
//   rotation:   4 bytes (smallest three quat)
//   total:     20 bytes per point
const POINT_STRIDE = 20;

function buildGaussianPoint(seed) {
  const buf = Buffer.alloc(POINT_STRIDE);
  // Use deterministic pseudo-random values based on seed
  const rng = (offset) => {
    const val = ((seed * 2654435761 + offset * 1013904243) >>> 0) % 256;
    return val / 255;
  };

  // positions: 3 × float32 LE
  buf.writeFloatLE(rng(0) * 10 - 5, 0);    // x: [-5, 5)
  buf.writeFloatLE(rng(1) * 10 - 5, 4);    // y
  buf.writeFloatLE(rng(2) * 10 - 5, 8);    // z

  // scale: log scale, 1 byte
  buf.writeUInt8(128 + (seed % 128), 12);

  // color: SH DC RGB, 3 bytes normalized
  buf.writeUInt8(180 + (seed % 76), 13);     // R: ~0.7-1.0
  buf.writeUInt8(100 + (seed % 100), 14);    // G: ~0.4-0.8
  buf.writeUInt8(50 + (seed % 80), 15);      // B: ~0.2-0.5

  // rotation: smallest three quat, 4 bytes
  buf.writeUInt32LE(0x3F800000 + seed, 16);  // pseudo-quat

  return buf;
}

function buildGaussianData(numPoints) {
  const buf = Buffer.alloc(numPoints * POINT_STRIDE);
  for (let i = 0; i < numPoints; i++) {
    buildGaussianPoint(i + 1).copy(buf, i * POINT_STRIDE);
  }
  return buf;
}

// ---------- Build SPZ v3 file (gzip compressed) ----------
function buildSpzV3(numPoints, opts = {}) {
  const { shDegree = 0, fractionalBits = 8, flags = 0 } = opts;

  const header = buildV3Header(numPoints, shDegree, fractionalBits, flags);
  const pointData = buildGaussianData(numPoints);
  const uncompressed = Buffer.concat([header, pointData]);

  return gzipSync(uncompressed);
}

// ---------- Build SPZ v4 file (raw header, optional ZSTD streams) ----------
function buildSpzV4(numPoints) {
  // v4 header is NOT compressed — stored raw
  return buildV4Header(numPoints);
}

// ---------- Main ----------
function main() {
  const args = process.argv.slice(2);
  const outDirIdx = args.indexOf('--out');
  const outDir = outDirIdx >= 0 ? resolve(args[outDirIdx + 1]) : join(process.cwd(), 'dist', 'fixtures');
  const showManifest = !args.includes('--no-manifest');

  mkdirSync(outDir, { recursive: true });

  const fixtures = [
    { name: 'minimal_v3.spz',      data: buildSpzV3(0)  },
    { name: 'single_point_v3.spz', data: buildSpzV3(1)  },
    { name: 'ten_points_v3.spz',   data: buildSpzV3(10)  },
    { name: 'v4_header_only.spz',  data: buildSpzV4(0)  },
  ];

  const manifest = [];
  for (const { name, data } of fixtures) {
    const filePath = join(outDir, name);
    writeFileSync(filePath, data);
    const hash = createHash('sha256').update(data).digest('hex');
    manifest.push({ file: name, sha256: hash, size: data.length });
  }

  if (showManifest) {
    const manifestJson = JSON.stringify(manifest, null, 2);
    if (outDirIdx >= 0) {
      // Also write manifest file alongside fixtures
      writeFileSync(join(outDir, 'fixture-manifest.json'), manifestJson + '\n');
    }
    console.log(manifestJson);
  }

  // Exit with fixture count
  console.error(`Generated ${fixtures.length} synthetic SPZ fixtures -> ${outDir}`);
}

main();
