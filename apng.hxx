#ifndef APNG_HXX
#define APNG_HXX

#include <stdint.h>
#include <stddef.h>
#include <exception>
#include <array>
#include <vector>
#include <memory>
#include "stream.hxx"

#ifndef _MSC_VER
	#if __GNUC__ >= 4
		#define APNG_API	__attribute__((visibility("default")))
	#else
		#define APNG_API
	#endif
#else
	#ifdef __APNG_lib__
		#define APNG_API	__declspec(dllexport)
	#else
		#define APNG_API	__declspec(dllimport)
	#endif
#endif

struct chunkType_t final
{
private:
	std::array<uint8_t, 4> _type;
	chunkType_t() = delete;

public:
	template<typename... T, char [sizeof...(T) == 4] = nullptr> constexpr chunkType_t(T... value) noexcept :
		_type{uint8_t(value)...} { }

	bool operator ==(const chunkType_t &chunk) const noexcept { return _type == chunk._type; }
	bool operator ==(const std::array<uint8_t, 4> &value) const noexcept { return _type == value; }
	bool operator ==(const uint8_t *const value) const noexcept;

	bool operator !=(const chunkType_t &chunk) const noexcept { return _type != chunk._type; }
	bool operator !=(const std::array<uint8_t, 4> &value) const noexcept { return _type != value; }
	bool operator !=(const uint8_t *const value) const noexcept;

	std::array<uint8_t, 4> &type() noexcept { return _type; }
	const std::array<uint8_t, 4> &type() const noexcept { return _type; }
};

struct chunk_t final
{
private:
	uint32_t _length;
	chunkType_t _chunkType;
	std::unique_ptr<uint8_t[]> _chunkData;

	chunk_t() noexcept : _length(0), _chunkType{0, 0, 0, 0}, _chunkData(nullptr) { }
	chunk_t(const chunk_t &) = delete;
	chunk_t &operator =(const chunk_t &) = delete;

public:
	chunk_t(chunk_t &&chunk) noexcept : _length(chunk._length), _chunkType(chunk._chunkType), _chunkData(std::move(chunk._chunkData)) { }
	chunk_t &operator =(chunk_t &&chunk) noexcept;
	~chunk_t() noexcept { }
	uint32_t length() const noexcept { return _length; }
	const chunkType_t &type() const noexcept { return _chunkType; }
	const uint8_t *data() const noexcept { return _chunkData.get(); }

	static chunk_t loadChunk(stream_t &stream) noexcept;
};

struct APNG_API bitDepth_t final
{
public:
	enum _bitDepth_t : uint8_t { bps1, bps2, bps4, bps8, bps16 };

private:
	_bitDepth_t value;

public:
	constexpr bitDepth_t() noexcept : value(bps8) { }
	bitDepth_t(const uint8_t depth);
	bitDepth_t(const bitDepth_t &depth) noexcept : value(depth.value) { }
	bitDepth_t(bitDepth_t &&depth) noexcept : value(depth.value) { }
	bitDepth_t &operator =(const bitDepth_t &depth) noexcept { value = depth.value; return *this; }
	bitDepth_t &operator =(bitDepth_t &&depth) noexcept { value = depth.value; return *this; }
	operator _bitDepth_t() const noexcept { return value; }
};

struct APNG_API colourType_t final
{
public:
	enum _colourType_t : uint8_t { greyscale, rgb, palette, greyscaleAlpha, rgba };

private:
	_colourType_t value;

public:
	constexpr colourType_t() noexcept : value(rgb) { }
	colourType_t(const uint8_t type);
	colourType_t(const colourType_t &type) noexcept : value(type.value) { }
	colourType_t(colourType_t &&type) noexcept : value(type.value) { }
	colourType_t &operator =(const colourType_t &type) noexcept { value = type.value; return *this; }
	colourType_t &operator =(colourType_t &&type) noexcept { value = type.value; return *this; }
	operator _colourType_t() const noexcept { return value; }
};

struct APNG_API interlace_t final
{
public:
	enum _interlace_t : uint8_t { none, adam7 };

private:
	_interlace_t value;

public:
	constexpr interlace_t() noexcept : value(none) { }
	interlace_t(const uint8_t type);
	interlace_t(const interlace_t &type) noexcept : value(type.value) { }
	interlace_t(interlace_t &&type) noexcept : value(type.value) { }
	interlace_t &operator =(const interlace_t &type) noexcept { value = type.value; return *this; }
	interlace_t &operator =(interlace_t &&type) noexcept { value = type.value; return *this; }
	operator _interlace_t() const noexcept { return value; }
};

struct acTL_t final
{
private:
	uint32_t _frames;
	uint32_t _loops;

	acTL_t(const acTL_t &) = delete;
	acTL_t &operator =(const acTL_t &) = delete;

public:
	acTL_t() noexcept : _frames(1), _loops(0) { }
	acTL_t(acTL_t &&acTL) noexcept : _frames(acTL._frames), _loops(acTL._loops) { }
	acTL_t &operator =(acTL_t &&acTL) noexcept
	{
		_frames = acTL._frames;
		_loops = acTL._frames;
		return *this;
	}

	static acTL_t reinterpret(const uint8_t *const data) noexcept;
	void check(const std::vector<chunk_t> &chunks);

	uint32_t frames() const noexcept { return _frames; }
	uint32_t loops() const noexcept { return _loops; }
};

struct fcTL_t final
{
};

enum class pixelFormat_t : uint8_t
{
	format8bppGrey,
	format16bppGrey,
	format24bppRGB,
	format32bppRGBA,
	format48bppRGB,
	format64bppRGBA
};

struct APNG_API bitmap_t final
{
private:
	std::unique_ptr<uint8_t[]> _data;
	const uint32_t _width, _height;
	const pixelFormat_t _format;

public:
	bitmap_t(const uint32_t width, const uint32_t height, const pixelFormat_t format);
	const uint8_t *data() const noexcept { return _data.get(); }
	uint8_t *data() noexcept { return _data.get(); }
	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
	pixelFormat_t format() const noexcept { return _format; }
};

struct APNG_API apng_t final
{
private:
	uint32_t _width;
	uint32_t _height;
	bitDepth_t _bitDepth;
	colourType_t _colourType;
	interlace_t _interlacing;
	acTL_t controlChunk;
	std::unique_ptr<bitmap_t> _defaultFrame;

public:
	apng_t(stream_t &stream);

	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
	bitDepth_t bitDepth() const noexcept { return _bitDepth; }
	colourType_t colourType() const noexcept { return _colourType; }
	interlace_t interlacing() const noexcept { return _interlacing; }
	const bitmap_t *defaultFrame() const noexcept { return _defaultFrame.get(); }

private:
	void checkSig(stream_t &stream);
	void validateHeader();
	void processDefaultFrame(const std::vector<chunk_t> &chunks);
};

struct APNG_API invalidPNG_t : public std::exception
{
public:
	invalidPNG_t() noexcept;
	const char *what() const noexcept;
};

#endif /*APNG_HXX*/