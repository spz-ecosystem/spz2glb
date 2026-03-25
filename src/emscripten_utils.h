// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
// Emscripten 工具函数
// 用于 JavaScript 与 C++ 数据转换

#ifndef SPZ2GLB_EMSCRIPTEN_UTILS_H_
#define SPZ2GLB_EMSCRIPTEN_UTILS_H_

#ifdef __EMSCRIPTEN__

#include <emscripten/val.h>
#include <vector>
#include <cstddef>
#include <cstdint>


namespace spz2glb {

/**
 * JavaScript Uint8Array 转 C++ std::vector<uint8_t>
 *
 * @param array JavaScript Uint8Array 对象
 * @return C++ vector
 */
inline std::vector<uint8_t> vectorFromJsArray(const emscripten::val& array) {
    const size_t length = array["length"].as<size_t>();
    std::vector<uint8_t> out(length);
    for (size_t i = 0; i < length; ++i) {
        out[i] = array[i].as<unsigned char>();
    }
    return out;
}

inline emscripten::val jsUint8ArrayFromVector(const std::vector<uint8_t>& buffer) {
    emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
    emscripten::val array = Uint8Array.new_(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
        array.set(i, buffer[i]);
    }
    return array;
}

inline emscripten::val jsUint8ArrayFromBytes(const std::vector<std::byte>& buffer) {
    emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
    emscripten::val array = Uint8Array.new_(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
        array.set(i, static_cast<uint8_t>(buffer[i]));
    }
    return array;
}


}  // namespace spz2glb

#endif  // __EMSCRIPTEN__

#endif  // SPZ2GLB_EMSCRIPTEN_UTILS_H_
