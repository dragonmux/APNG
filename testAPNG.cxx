#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <crunch++.h>
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
		}
		catch (invalidPNG_t &error)
		{
			fail(error.what());
		}
	}

	void testMemoryStream() noexcept
	{
		const int32_t fd = open("loading_16.png", O_RDONLY | O_NOCTTY);
		assertNotEqual(fd, -1);
		// TODO: stat fd.
		close(fd);
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