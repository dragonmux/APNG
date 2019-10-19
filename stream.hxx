#ifndef APNG_HXX
#include "apng.hxx"
#endif

#ifndef STREAM__HXX
#define STREAM__HXX

#include <zlib.h>

constexpr uint32_t operator ""_KiB(const unsigned long long value) noexcept { return uint32_t(value) * 1024; }

struct APNG_API notImplemented_t : public std::exception { };
struct APNG_API zlibError_t : public std::exception { };

struct stream_t
{
protected:
	stream_t() noexcept = default;
	stream_t(stream_t &&) = default;
	stream_t &operator =(stream_t &&) = default;

public:
	virtual ~stream_t() = default;

	template<typename T> bool read(T &value)
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value)
		{ return read(value.data(), N); }

	bool read(void *const value, const size_t valueLen)
	{
		size_t countRead = 0;
		if (!read(value, valueLen, countRead))
			return false;
		return valueLen == countRead;
	}

	virtual bool read(void *const, const size_t, size_t &) { throw notImplemented_t(); }
	virtual bool write(const void *const, const size_t) { throw notImplemented_t(); }
	virtual bool atEOF() const { throw notImplemented_t(); }

	stream_t(const stream_t &) = delete;
	stream_t &operator =(const stream_t &) = delete;
};

struct APNG_API fileStream_t final : public stream_t
{
private:
	int fd;
	size_t length;
	bool eof;

	fileStream_t() noexcept : stream_t{}, fd{-1}, length{}, eof{true} { }

public:
	fileStream_t(const char *const fileName, const int32_t mode);
	fileStream_t(fileStream_t &&stream) noexcept : fileStream_t{} { swap(stream); }
	~fileStream_t() noexcept final override;
	void operator =(fileStream_t &&stream) noexcept { swap(stream); }

	bool read(void *const value, const size_t valueLen, size_t &countRead) final override;
	bool atEOF() const noexcept final override { return eof; }

	void swap(fileStream_t &stream) noexcept;
	fileStream_t(const fileStream_t &) = delete;
	fileStream_t &operator =(const fileStream_t &) = delete;
};

inline void swap(fileStream_t &a, fileStream_t &b) noexcept { a.swap(b); }

struct APNG_API memoryStream_t : public stream_t
{
private:
	char *memory;
	size_t length;
	size_t pos;

	memoryStream_t() noexcept : stream_t{}, memory{}, length{}, pos{} { }

public:
	memoryStream_t(void *const stream, const size_t streamLength) noexcept;
	memoryStream_t(memoryStream_t &&stream) noexcept : memoryStream_t{} { swap(stream); }
	~memoryStream_t() noexcept override = default;
	void operator =(memoryStream_t &&stream) noexcept { swap(stream); }

	bool read(void *const value, const size_t valueLen, size_t &countRead) noexcept final override;
	bool atEOF() const noexcept final override { return pos == length; }

	void swap(memoryStream_t &stream) noexcept;
	memoryStream_t(const memoryStream_t &) = delete;
	memoryStream_t &operator =(const memoryStream_t &) = delete;
};

inline void swap(memoryStream_t &a, memoryStream_t &b) noexcept { a.swap(b); }

struct APNG_API zlibStream_t : public stream_t
{
public:
	enum mode_t : uint8_t { inflate, deflate };

private:
	constexpr static const uint32_t chunkLen = 8_KiB;
	stream_t *source;
	mode_t mode;
	z_stream stream;
	uint32_t bufferUsed;
	uint32_t bufferAvail;
	uint8_t bufferIn[chunkLen];
	uint8_t bufferOut[chunkLen];
	bool eos;

	zlibStream_t() noexcept : source{}, mode{mode_t::inflate}, stream{}, bufferUsed{}, bufferAvail{},
		bufferIn{}, bufferOut{}, eos{true} { }

public:
	zlibStream_t(stream_t &sourceStream, const mode_t streamMode);
	zlibStream_t(const zlibStream_t &stream) noexcept : zlibStream_t{} { clone(stream); }
	~zlibStream_t() noexcept override;
	void operator =(const zlibStream_t &stream) noexcept { clone(stream); }

	bool read(void *const value, const size_t valueLen, size_t &countRead) final override;
	bool atEOF() const noexcept final override { return eos; }

	void clone(const zlibStream_t &stream) noexcept;
	zlibStream_t(zlibStream_t &&) = delete;
	zlibStream_t &operator =(zlibStream_t &&) = delete;
};

#endif /*STREAM__HXX*/
