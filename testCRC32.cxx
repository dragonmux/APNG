#include "crc32.hxx"
#include <stdio.h>

constexpr std::pair<std::array<uint8_t, 17>, uint32_t> testData1 =
{
	{
		0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x02, 0x78,
		0x00, 0x00, 0x02, 0xF2, 0x08, 0x02, 0x00, 0x00,
		0x00
	},
	0xC6DE3ED3
};

int main(int, char **)
{
	uint32_t crcCalc;

	crc32_t::crc(crcCalc = 0, testData1.first);
	printf("%08X vs %08X\n", crcCalc, testData1.second);

	return 0;
}
