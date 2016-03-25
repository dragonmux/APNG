#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <system_error>
#include "internals.hxx"
#include "stream.hxx"

fileStream_t::fileStream_t(const char *const fileName, const int32_t mode) noexcept : fd(-1), eof(false)
{
	struct stat fileStat;
	fd = open(fileName, mode);
	if (fd == -1 || fstat(fd, &fileStat) != 0)
		throw std::system_error(errno, std::system_category());
	length = fileStat.st_size;
}

fileStream_t::~fileStream_t() noexcept { close(fd); }

bool fileStream_t::read(void *const value, const size_t valueLen) noexcept
{
	if (eof)
		return false;
	ssize_t ret = ::read(fd, value, valueLen);
	if (ret < 0)
		throw std::system_error(errno, std::system_category());
	eof = length == size_t(lseek(fd, 0, SEEK_CUR));
	return size_t(ret) == valueLen;
}

bool fileStream_t::atEOF() const noexcept
	{ return eof; }

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

bool memoryStream_t::atEOF() const noexcept
	{ return pos == length; }