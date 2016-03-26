#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <system_error>
#include "internals.hxx"
#include "stream.hxx"

fileStream_t::fileStream_t(const char *const fileName, const int32_t mode) : fd(-1), eof(false)
{
	struct stat fileStat;
	fd = open(fileName, mode);
	if (fd == -1 || fstat(fd, &fileStat) != 0)
		throw std::system_error(errno, std::system_category());
	length = fileStat.st_size;
}

fileStream_t::~fileStream_t() noexcept { close(fd); }

bool fileStream_t::read(void *const value, const size_t valueLen, size_t &actualLen)
{
	if (eof)
		return false;
	ssize_t ret = ::read(fd, value, valueLen);
	if (ret < 0)
		throw std::system_error(errno, std::system_category());
	eof = length == size_t(lseek(fd, 0, SEEK_CUR));
	actualLen = size_t(ret);
	return true;
}

bool fileStream_t::atEOF() const noexcept
	{ return eof; }

memoryStream_t::memoryStream_t(void *const stream, const size_t streamLength) noexcept :
	memory(static_cast<char *const>(stream)), length(streamLength), pos(0) { }

bool memoryStream_t::read(void *const value, const size_t valueLen, size_t &actualLen) noexcept
{
	if ((pos + valueLen) < pos)
		return false;
	actualLen = (pos + valueLen) > length ? length - pos : valueLen;
	memcpy(value, memory + pos, actualLen);
	pos += actualLen;
	return true;
}

bool memoryStream_t::atEOF() const noexcept
	{ return pos == length; }