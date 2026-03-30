// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb

#ifndef SPZ2GLB_CORE_H
#define SPZ2GLB_CORE_H

#include <cstddef>
#include <cstdint>
#include <vector>

bool convertSpzToGlbCore(const uint8_t* spzData, size_t spzSize, std::vector<std::byte>& glbData);
bool validateSpzHeaderCore(const uint8_t* data, size_t size);

#endif  // SPZ2GLB_CORE_H
