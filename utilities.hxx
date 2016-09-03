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

template<typename T> bool contains(const std::vector<T> &list,
	bool condition(const T &)) noexcept
{
	for (const T &item : list)
	{
		if (condition(item))
			return true;
	}
	return false;
}

template<typename T> std::vector<const T *> extract(const typename std::vector<T>::const_iterator &begin,
	const typename std::vector<T>::const_iterator &end, bool condition(const T &)) noexcept
{
	std::vector<const T *> result;
	for (auto iter = begin; iter != end; ++iter)
	{
		const T &item = *iter;
		if (condition(item))
			result.emplace_back(&item);
	}
	return result;
}

template<typename T> std::vector<const T *> extract(const std::vector<T> &list,
	bool condition(const T &)) noexcept
	{ return extract(list.begin(), list.end(), condition); }

template<typename T> std::vector<typename std::vector<T>::const_iterator> extractIters(const std::vector<T> &list,
	bool condition(const T &)) noexcept
{
	using listIter_t = typename std::vector<T>::const_iterator;
	std::vector<listIter_t> result;
	for (listIter_t iter = list.begin(); iter != list.end(); ++iter)
	{
		const T &item = *iter;
		if (condition(item))
			result.emplace_back(iter);
	}
	return result;
}

template<typename T> const T *extractFirst(const std::vector<T> &list,
	bool condition(const T &)) noexcept
{
	for (const T &item : list)
	{
		if (condition(item))
			return &item;
	}
	return nullptr;
}

template<typename T> bool isBefore(const T *a, const T *b) noexcept
	{ return a < b; }

template<typename T> bool isAfter(const T *a, const T *b) noexcept
	{ return a > b; }

template<typename T> struct pngRGB_t
{
public:
	T r;
	T g;
	T b;

	pngRGB_t<T> operator +(const pngRGB_t<T> &pixel) const noexcept
		{ return {T(r + pixel.r), T(g + pixel.g), T(b + pixel.b)}; }
	pngRGB_t<T> operator >>(const uint8_t amount) const noexcept
		{ return {T(r >> amount), T(g >> amount), T(b >> amount)}; }
	pngRGB_t<T> operator &(const T mask) const noexcept
		{ return {T(r & mask), T(g & mask), T(b & mask)}; }
	pngRGB_t<T> operator &(const pngRGB_t<T> &mask) const noexcept
		{ return {T(r & mask.r), T(g & mask.g), T(b & mask.b)}; }
};

template<typename T> struct pngRGBA_t final : public pngRGB_t<T>
{
public:
	T a;

	using pngRGBn_t = pngRGB_t<T>;
	pngRGBA_t() = default;
	pngRGBA_t(const pngRGBn_t rgb, const T _a) : pngRGBn_t(rgb), a(_a) { }

	pngRGBA_t<T> operator +(const pngRGBA_t<T> &pixel) const noexcept
		{ return {pngRGBn_t(*this) + pngRGBn_t(pixel), T(a + pixel.a)}; }
	pngRGBA_t<T> operator >>(const uint8_t amount) const noexcept
		{ return {pngRGBn_t(*this) >> amount, T(a >> amount)}; }
	pngRGBA_t<T> operator &(const T mask) const noexcept
		{ return {pngRGBn_t(*this) & mask, T(a & mask)}; }
	pngRGBA_t<T> operator &(const pngRGBA_t<T> &mask) const noexcept
		{ return {pngRGBn_t(*this) & pngRGBn_t(mask), T(a & mask.a)}; }
};

template<typename T> struct pngGrey_t
{
public:
	T v;

	pngGrey_t<T> operator +(const pngGrey_t<T> &pixel) const noexcept
		{ return {T(v + pixel.v)}; }
	pngGrey_t<T> operator >>(const uint8_t amount) const noexcept
		{ return {T(v >> amount)}; }
	pngGrey_t<T> operator &(const T mask) const noexcept
		{ return {T(v & mask)}; }
	pngGrey_t<T> operator &(const pngGrey_t<T> &mask) const noexcept
		{ return {T(v & mask.v)}; }
};

template<typename T> struct pngGreyA_t final : public pngGrey_t<T>
{
public:
	T a;

	using pngGreyN_t = pngGrey_t<T>;
	pngGreyA_t() = default;
	pngGreyA_t(const pngGreyN_t grey, const T _a) : pngGreyN_t(grey), a(_a) { }

	pngGreyA_t<T> operator +(const pngGreyA_t<T> &pixel) const noexcept
		{ return {pngGreyN_t(*this) + pngGreyN_t(pixel), T(a + pixel.a)}; }
	pngGreyA_t<T> operator >>(const uint8_t amount) const noexcept
		{ return {pngGreyN_t(*this) >> amount, T(a >> amount)}; }
	pngGreyA_t<T> operator &(const T mask) const noexcept
		{ return {pngGreyN_t(*this) & mask, T(a & mask)}; }
	pngGreyA_t<T> operator &(const pngGreyA_t<T> &mask) const noexcept
		{ return {pngGreyN_t(*this) & pngGreyN_t(mask), T(a & mask.a)}; }
};

using pngRGB8_t = pngRGB_t<uint8_t>;
using pngRGB16_t = pngRGB_t<uint16_t>;
using pngRGBA8_t = pngRGBA_t<uint8_t>;
using pngRGBA16_t = pngRGBA_t<uint16_t>;
using pngGrey8_t = pngGrey_t<uint8_t>;
using pngGrey16_t = pngGrey_t<uint16_t>;
using pngGreyA8_t = pngGreyA_t<uint8_t>;
using pngGreyA16_t = pngGreyA_t<uint16_t>;

template<typename T> bool readRGB(stream_t &stream, T &pixel) noexcept
	{ return stream.read(pixel.r) && stream.read(pixel.g) && stream.read(pixel.b); }
template<typename T> bool readRGBA(stream_t &stream, T &pixel) noexcept
	{ return readRGB(stream, pixel) && stream.read(pixel.a); }
template<typename T> bool readGrey(stream_t &stream, T &pixel) noexcept
	{ return stream.read(pixel.v); }
template<typename T> bool readGreyA(stream_t &stream, T &pixel) noexcept
	{ return readGrey(stream, pixel) && stream.read(pixel.a); }

template<typename T, bool copyFunc(stream_t &, T &)> bool copyFrame(stream_t &stream, void *const dataPtr, const bitmapRegion_t frame) noexcept
{
	T *const data = static_cast<T *const>(dataPtr);
	const uint32_t width = frame.width();
	const uint32_t height = frame.height();
	for (uint32_t y = 0; y < height; ++y)
	{
		uint8_t filter;
		if (!stream.read(filter))
			return false;

		for (uint32_t x = 0; x < width; ++x)
		{
			if (!copyFunc(stream, data[x + (y * width)]))
				return false;
			// TODO: Make me properly handle the scanline filtering issue.
			// filterFunc(data[x + (y * width)])
		}
	}
	return true;
}

template<typename T> T compSource(T, T b) noexcept
	{ return b; }
template<typename T> T compOver(T a, T b) noexcept
	{ return (a >> 1) + (b >> 1) + ((a & b) & 0x01); }

template<blendOp_t::_blendOp_t op, typename T> T compOp(T a, T b) noexcept
{
	if (op == blendOp_t::source)
		return compSource(a, b);
	else
		return compOver(a, b);
}

template<typename T, blendOp_t::_blendOp_t op, typename U = T> U compRGB(const T pixelA, const T pixelB) noexcept
	{ return {compOp<op>(pixelA.r, pixelB.r), compOp<op>(pixelA.g, pixelB.g), compOp<op>(pixelA.b, pixelB.b)}; }
template<typename T, blendOp_t::_blendOp_t op> T compRGBA(const T pixelA, const T pixelB) noexcept
	{ return {compRGB<T, op, typename T::pngRGBn_t>(pixelA, pixelB), compOp<op>(pixelA.a, pixelB.a)}; }
template<typename T, blendOp_t::_blendOp_t op, typename U = T> U compGrey(const T pixelA, const T pixelB) noexcept
	{ return {compOp<op>(pixelA.v, pixelB.v)}; }

template<typename T> void compFrame(T compFunc(const T, const T), const bitmap_t &source, bitmap_t &destination,
	const uint32_t xOffset, const uint32_t yOffset) noexcept
{
	if ((source.width() + xOffset) > destination.width() || (source.height() + yOffset) > destination.height())
		return;
	const T *const srcData = static_cast<const T *const>(static_cast<const void *const>(source.data()));
	T *const dstData = static_cast<T *const>(static_cast<void *const>(destination.data()));
	const uint32_t width = source.width();
	const uint32_t height = source.height();

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			const uint32_t offsetSrc = x + (y * width);
			const uint32_t offsetDst = (x + xOffset) + ((y + yOffset) * destination.width());
			dstData[offsetDst] = compFunc(dstData[offsetDst], srcData[offsetSrc]);
		}
	}
}

#endif /*UTILITIES_HXX*/