#include "crc32.hxx"

// Iterate till we have calculated the CRC
constexpr uint32_t calcTableC(const uint32_t poly, const uint32_t c, const uint8_t k) noexcept
	{ return k ? calcTableC(poly, ((c & 1) == 1 ? poly : 0) ^ (c >> 1), k - 1) : c; }
// Generate CRC for byte 'n'
constexpr uint32_t calcTableC(const uint32_t poly, const uint8_t n) noexcept
	{ return calcTableC(poly, n, 8); }

template<uint8_t N, uint32_t poly, uint32_t... table> struct calcTable
	{ constexpr static std::array<const uint32_t, sizeof...(table) + N + 1> value = calcTable<N - 1, poly, calcTableC(poly, N), table...>::value; };
template<uint32_t poly, uint32_t... table> struct calcTable<0, poly, table...>
	{ constexpr static std::array<const uint32_t, sizeof...(table) + 1> value = { calcTableC(poly, 0), table... }; };

std::array<const uint32_t, 256> crc32_t::crcTable = calcTable<255, poly>::value;