#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <system_error>
#include "internals.hxx"
#include "stream.hxx"

fileStream_t::fileStream_t(const char *const fileName, const int32_t mode) noexcept : fd(-1)
{
	fd = open(fileName, mode);
	if (fd == -1)
		throw std::system_error(errno, std::system_category());
}

fileStream_t::~fileStream_t() noexcept { close(fd); }

bool fileStream_t::read(void *const value, const size_t length) noexcept
{
	ssize_t ret = ::read(fd, value, length);
	if (ret < 0)
		throw std::system_error(errno, std::system_category());
	return size_t(ret) == length;
}

memoryStream_t::memoryStream_t(void *const file, const size_t fileLength) noexcept :
	memory(file), length(fileLength) { }

bool memoryStream_t::read(void *const value, const size_t valueLen) noexcept
{
	if ((pos + valueLen) < pos || (pos + valueLen) > length)
		return false;
	memcpy(value, memory, valueLen);
	pos += valueLen;
	return true;
}