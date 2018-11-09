#include <stdio.h>
#include <fcntl.h>
#include <system_error>
#include "apng.hxx"

int main(int argc, char **argv) noexcept try
{
	if (argc != 2)
		return 1;

	fileStream_t pngFile(argv[1], O_RDONLY | O_NOCTTY);
	apng_t image(pngFile);
	printf("Sucessfully read %s (%ux%u)\n", argv[1], image.width(), image.height());

	return 0;
}
catch (std::system_error &error)
{
	puts(error.what());
	return 2;
}
catch (invalidPNG_t &error)
{
	puts(error.what());
	return 1;
}
