#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <crunch++.h>
#include <memory>
#include <system_error>
#include "apng.hxx"

class apngTests final : public testsuit
{
public:
	void testFileStream() noexcept
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

	void testMemoryStream() noexcept
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

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<apngTests>();
}