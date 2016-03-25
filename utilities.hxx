#ifndef UTILITIES_HXX
#define UTILITIES_HXX

#include <stdint.h>
#include "stream.hxx"

struct bitmapRegion_t final
{
private:
	const uint32_t _width, _height;
	bitmapRegion_t() = delete;
	bitmapRegion_t &operator =(const bitmapRegion_t &) = delete;
	bitmapRegion_t &operator =(bitmapRegion_t &&region) = delete;

public:
	constexpr bitmapRegion_t(const uint32_t width, const uint32_t height) noexcept : _width(width), _height(height) { }
	bitmapRegion_t(const bitmapRegion_t &region) noexcept : _width(region._width), _height(region._height) { }
	bitmapRegion_t(bitmapRegion_t &&region) noexcept : _width(region._width), _height(region._height) { }

	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
};

uint32_t swap32(const uint32_t i) noexcept
{
	return ((i >> 24) & 0xFF) | ((i >> 8) & 0xFF00) | ((i & 0xFF00) << 8) | ((i & 0xFF) << 24);
}
void swap(uint32_t &i) noexcept { i = swap32(i); }

uint16_t swap16(const uint16_t i) noexcept
{
	return ((i >> 8) & 0xFF) | ((i & 0xFF) << 8);
}
void swap(uint16_t &i) noexcept { i = swap16(i); }

template<typename T> struct pngRGB_t
{
	T r;
	T g;
	T b;
};

template<typename T> struct pngRGBA_t final : public pngRGB_t<T>
	{ T a; };

using pngRGB8_t = pngRGB_t<uint8_t>;
using pngRGB16_t = pngRGB_t<uint16_t>;
using pngRGBA8_t = pngRGBA_t<uint8_t>;
using pngRGB16_t = pngRGB_t<uint16_t>;

template<typename T> bool readRGB(stream_t &stream, T &pixel) noexcept
	{ return stream.read(pixel.r) && stream.read(pixel.g) && stream.read(pixel.b); }
template<typename T> bool readRGBA(stream_t &stream, T &pixel) noexcept
	{ return readRGB(stream, pixel) && stream.read(pixel.a); }

template<typename T, bool copyFunc(stream_t &, T &)> bool copyFrame(stream_t &stream, void *const dataPtr, bitmapRegion_t frame) noexcept
{
	T *const data = static_cast<T *const>(dataPtr);
	const uint32_t width = frame.width();
	const uint32_t height = frame.height();
	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			if (!copyFunc(stream, data[x + (y * width)]))
				return false;
		}
	}
	return true;
}

#endif /*UTILITIES_HXX*/