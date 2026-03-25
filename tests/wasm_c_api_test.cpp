#include "../src/spz2glb_wasm_c_api.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <zlib.h>

bool convertSpzToGlbCore(const uint8_t* spzData, size_t spzSize, std::vector<std::byte>& glbData);

namespace {

[[noreturn]] void failCheck(const char* expr, int line) {
    std::cerr << "CHECK failed at line " << line << ": " << expr << std::endl;
    std::exit(1);
}

#define CHECK(expr) do { if (!(expr)) failCheck(#expr, __LINE__); } while (0)

std::vector<uint8_t> makeMinimalSpzPayload() {
    std::vector<uint8_t> payload(16, 0);

    const uint32_t magic = 0x5053474e;
    const uint32_t version = 2;
    const uint32_t numPoints = 1;
    const uint8_t shDegree = 0;
    const uint8_t fractionalBits = 8;
    const uint8_t flags = 0;
    const uint8_t reserved = 0;

    std::memcpy(payload.data(), &magic, sizeof(magic));
    std::memcpy(payload.data() + 4, &version, sizeof(version));
    std::memcpy(payload.data() + 8, &numPoints, sizeof(numPoints));
    std::memcpy(payload.data() + 12, &shDegree, sizeof(shDegree));
    std::memcpy(payload.data() + 13, &fractionalBits, sizeof(fractionalBits));
    std::memcpy(payload.data() + 14, &flags, sizeof(flags));
    std::memcpy(payload.data() + 15, &reserved, sizeof(reserved));

    return payload;
}

std::vector<uint8_t> gzipBytes(const std::vector<uint8_t>& input) {
    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
    stream.avail_in = static_cast<uInt>(input.size());

    CHECK(deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) == Z_OK);

    std::vector<uint8_t> output(128, 0);
    int ret = Z_OK;
    while (ret == Z_OK) {
        if (stream.total_out == output.size()) {
            output.resize(output.size() * 2);
        }

        stream.next_out = reinterpret_cast<Bytef*>(output.data() + stream.total_out);
        stream.avail_out = static_cast<uInt>(output.size() - stream.total_out);
        ret = deflate(&stream, Z_FINISH);
    }

    CHECK(ret == Z_STREAM_END);
    output.resize(stream.total_out);
    deflateEnd(&stream);
    return output;
}

std::vector<uint8_t> makeMinimalSpzGzip() {
    return gzipBytes(makeMinimalSpzPayload());
}

std::vector<uint8_t> makeInvalidSpzPayload() {
    auto payload = makeMinimalSpzPayload();
    payload[0] = 0;
    return payload;
}

}  // namespace


int main() {
    spz2glb_reset_memory_stats();

    Spz2GlbMemoryStats stats{};
    spz2glb_get_memory_stats(&stats);
    CHECK(stats.peak_usage_bytes == 0);
    CHECK(stats.current_usage_bytes == 0);
    CHECK(stats.total_allocations == 0);
    CHECK(stats.total_frees == 0);
    CHECK(stats.failed_allocations == 0);

    std::vector<uint8_t> spzData = makeMinimalSpzGzip();
    std::vector<uint8_t> rawSpzData = makeMinimalSpzPayload();
    std::vector<uint8_t> invalidSpzData = makeInvalidSpzPayload();

    CHECK(spz2glb_validate_spz_header(rawSpzData.data(), rawSpzData.size()));
    CHECK(spz2glb_validate_spz_header(spzData.data(), spzData.size()));
    CHECK(!spz2glb_validate_spz_header(invalidSpzData.data(), invalidSpzData.size()));

    std::vector<std::byte> coreOutput;

    CHECK(convertSpzToGlbCore(spzData.data(), spzData.size(), coreOutput));
    CHECK(!coreOutput.empty());
    CHECK(spz2glb_validate_header(reinterpret_cast<const uint8_t*>(coreOutput.data()), coreOutput.size()));

    const size_t reserved = spz2glb_reserve_input(spzData.size());
    CHECK(reserved == spzData.size());

    uint8_t* input = spz2glb_get_input_ptr();
    CHECK(input != nullptr);
    std::memcpy(input, spzData.data(), spzData.size());

    uint8_t* output = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(1));
    size_t outSize = 123;
    CHECK(spz2glb_convert_reserved_input(0, &output, &outSize) == 0);
    CHECK(output == nullptr);
    CHECK(outSize == 0);

    CHECK(spz2glb_convert_reserved_input(spzData.size(), &output, &outSize) == 1);
    CHECK(output != nullptr);
    CHECK(outSize > 0);
    CHECK(spz2glb_validate_header(output, outSize));

    spz2glb_get_memory_stats(&stats);
    CHECK(stats.current_usage_bytes >= reserved + outSize);
    CHECK(stats.peak_usage_bytes >= stats.current_usage_bytes);

    spz2glb_reset_memory_stats();
    spz2glb_get_memory_stats(&stats);
    CHECK(stats.current_usage_bytes >= reserved + outSize);
    CHECK(stats.peak_usage_bytes >= stats.current_usage_bytes);

    spz2glb_release_output(output);

    CHECK(spz2glb_reserve_input(0) == 0);
    CHECK(spz2glb_get_input_ptr() == nullptr);

    spz2glb_get_memory_stats(&stats);
    CHECK(stats.current_usage_bytes == 0);

    uint8_t* buffer = spz2glb_alloc(32);
    CHECK(buffer != nullptr);
    std::memset(buffer, 0xCD, 32);
    spz2glb_release_output(buffer);

    return 0;
}
