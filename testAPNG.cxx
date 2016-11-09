#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <crunch++.h>
#include <memory>
#include <system_error>
#include "apng.hxx"
#include "crc32.hxx"

class apngTests final : public testsuit
{
public:
	void testFileStream()
	{
		try
		{
			fileStream_t pngFile("loading_16.png", O_RDONLY | O_NOCTTY);
			apng_t image(pngFile);
			assertEqual(image.width(), 16);
			assertEqual(image.height(), 16);
		}
		catch (std::system_error &error)
		{
			fail(error.what());
		}
		catch (invalidPNG_t &error)
		{
			fail(error.what());
		}
	}

	void testMemoryStream()
	{
		struct stat fileStat;
		const int32_t fd = open("loading_16.png", O_RDONLY | O_NOCTTY);
		assertNotEqual(fd, -1);
		assertEqual(fstat(fd, &fileStat), 0);
		std::unique_ptr<uint8_t []> file(new uint8_t[fileStat.st_size]);
		assertEqual(read(fd, file.get(), fileStat.st_size), ssize_t(fileStat.st_size));
		close(fd);

		memoryStream_t pngFile(file.get(), fileStat.st_size);
		try
		{
			apng_t image(pngFile);
			assertEqual(image.width(), 16);
			assertEqual(image.height(), 16);
		}
		catch (invalidPNG_t &error)
		{
			fail(error.what());
		}
	}

	void registerTests() final override
	{
		CXX_TEST(testFileStream)
		CXX_TEST(testMemoryStream)
	}
};

class crc32Tests final :  public testsuit
{
private:
	static const std::pair<std::array<uint8_t, 17>, uint32_t> testData1;

public:
	void testCRC32()
	{
		uint32_t crcCalc;
		crc32_t::crc(crcCalc = 0, testData1.first);
		assertEqual(int32_t(crcCalc), int32_t(testData1.second));
	}

	void registerTests() final override
	{
		CXX_TEST(testCRC32)
	}
};

const std::pair<std::array<uint8_t, 17>, uint32_t> crc32Tests::testData1
{
	{
		0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x02, 0x78,
		0x00, 0x00, 0x02, 0xF2, 0x08, 0x02, 0x00, 0x00,
		0x00
	},
	0xC6DE3ED3
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<apngTests, crc32Tests>();
}
