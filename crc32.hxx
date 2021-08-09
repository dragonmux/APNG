#ifndef CRC32_HXX
#define CRC32_HXX

#include "internals.hxx"

constexpr uint32_t calcPolynomial(const uint8_t bit) noexcept { return 1U << (31U - bit); }
template<typename T, typename... U> constexpr typename std::enable_if<sizeof...(U) != 0, uint32_t>::type
	calcPolynomial(T bit, U... bits) noexcept { return calcPolynomial(bit) | calcPolynomial(bits...); }

struct crc32_t final
{
private:
	constexpr static uint32_t poly = calcPolynomial(0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26);
	static_assert(poly == 0xEDB88320u, "Polynomial calculation failure");
	static std::array<const uint32_t, 256> crcTable;

public:
	template<size_t N> static void crc(uint32_t &crc, const std::array<uint8_t, N> &data) noexcept
		{ crc32_t::crc(crc, data.data(), data.size()); }
	static void crc(uint32_t &crc, const uint8_t *data, size_t dataLen) noexcept
	{
		crc ^= UINT32_C(0xFFFFFFFF);
		while (dataLen--)
			crc = crcTable[uint8_t(crc ^ *data++)] ^ (crc >> 8U);
		crc ^= UINT32_C(0xFFFFFFFF);
	}

	crc32_t() = delete;
};

#endif /*CRC32_HXX*/
