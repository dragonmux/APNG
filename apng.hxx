#ifndef APNG_HXX
#define APNG_HXX

#include <stdint.h>
#include <stddef.h>
#include <exception>
#include <array>
#include "stream.hxx"

struct bitDepth_t final
{
public:
	enum _bitDepth_t { bps1, bps2, bps4, bps8, bps16 };

private:
	_bitDepth_t value;
	bitDepth_t(bitDepth_t &&) = delete;
	bitDepth_t &operator =(const bitDepth_t &) = delete;

public:
	constexpr bitDepth_t() noexcept : value(bps8) { }
	bitDepth_t(const uint8_t depth);
	bitDepth_t(const bitDepth_t &depth) noexcept : value(depth.value) { }
	bitDepth_t &operator =(bitDepth_t &&depth) noexcept { value = depth.value; return *this; }
	operator _bitDepth_t() const noexcept { return value; }
};

struct colourType_t final
{
public:
	enum _colourType_t { greyscale, rgb, palette, greyscaleAlpha, rgba };

private:
	_colourType_t value;
	colourType_t(colourType_t &&) = delete;
	colourType_t &operator =(const colourType_t &) = delete;

public:
	constexpr colourType_t() noexcept : value(rgb) { }
	colourType_t(const uint8_t type);
	colourType_t(const colourType_t &type) noexcept : value(type.value) { }
	colourType_t &operator =(colourType_t &&type) noexcept { value = type.value; return *this; }
	operator _colourType_t() const noexcept { return value; }
};

struct interlace_t final
{
public:
	enum _interlace_t { none, adam7 };

private:
	_interlace_t value;
	interlace_t(interlace_t &&) = delete;
	interlace_t &operator =(const interlace_t &) = delete;

public:
	constexpr interlace_t() noexcept : value(none) { }
	interlace_t(const uint8_t type);
	interlace_t(const interlace_t &type) noexcept : value(type.value) { }
	interlace_t &operator =(interlace_t &&type) noexcept { value = type.value; return *this; }
	operator _interlace_t() const noexcept { return value; }
};

struct acTL_t final
{
};

struct fcTL_t final
{
};

struct apng_t final
{
private:
	uint32_t _width;
	uint32_t _height;
	bitDepth_t _bitDepth;
	colourType_t _colourType;
	interlace_t _interlacing;

public:
	apng_t(stream_t &stream);

private:
	void checkSig(stream_t &stream);
};

struct invalidPNG_t : public std::exception
{
public:
	invalidPNG_t() noexcept;
	const char *what() const noexcept;
};

#endif /*APNG_HXX*/