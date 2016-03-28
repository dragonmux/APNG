#ifndef APNG_HXX
#include "apng.hxx"
#endif

#ifndef STREAM__HXX
#define STREAM__HXX

#include <zlib.h>

constexpr uint32_t operator ""_KiB(const unsigned long long value) noexcept { return uint32_t(value) * 1024; }

struct notImplemented_t : public std::exception { };
struct zlibError_t : public std::exception { };

struct stream_t
{
protected:
	stream_t() noexcept { }

public:
	template<typename T> bool read(T &value) noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) noexcept
		{ return read(value.data(), N); }

	bool read(void *const value, const size_t valueLen)
	{
		size_t actualLen = 0;
		if (!read(value, valueLen, actualLen))
			return false;
		return valueLen == actualLen;
	}

	virtual bool read(void *const, const size_t, size_t &) { throw notImplemented_t(); }
	virtual bool write(const void *const, const size_t) { throw notImplemented_t(); }
	virtual bool atEOF() const { throw notImplemented_t(); }
};

struct fileStream_t final : public stream_t
{
private:
	int fd;
	size_t length;
	bool eof;

public:
	fileStream_t(const char *const fileName, const int32_t mode);
	~fileStream_t() noexcept;

	bool read(void *const value, const size_t valueLen, size_t &actualLen) final override;
	bool atEOF() const noexcept final override;
};

struct memoryStream_t : public stream_t
{
private:
	char *const memory;
	const size_t length;
	size_t pos;

public:
	memoryStream_t(void *const stream, const size_t streamLength) noexcept;

	bool read(void *const value, const size_t valueLen, size_t &actualLen) noexcept final override;
	bool atEOF() const noexcept final override;
};

struct zlibStream_t : public stream_t
{
public:
	enum mode_t : uint8_t { inflate, deflate };

private:
	constexpr static const uint32_t chunkLen = 8_KiB;
	stream_t &source;
	const mode_t mode;
	z_stream stream;
	uint32_t bufferUsed;
	uint32_t bufferAvail;
	uint8_t bufferIn[chunkLen];
	uint8_t bufferOut[chunkLen];
	bool eos;

public:
	zlibStream_t(stream_t &sourceStream, const mode_t streamMode);
	~zlibStream_t() noexcept;

	bool read(void *const value, const size_t valueLen, size_t &actualLen) final override;
	bool atEOF() const noexcept final override;
};

#endif /*STREAM__HXX*/