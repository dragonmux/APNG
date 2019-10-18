#ifndef APNG_HXX
#define APNG_HXX

#include <stdint.h>
#include <stddef.h>
#include <exception>
#include <array>
#include <vector>
#include <memory>

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

#include "stream.hxx"

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

	static chunk_t loadChunk(stream_t &stream);
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

struct APNG_API disposeOp_t final
{
public:
	enum _disposeOp_t { none, background, previous };

private:
	_disposeOp_t value;

public:
	constexpr disposeOp_t() noexcept : value(background) { }
	disposeOp_t(const uint8_t op);
	disposeOp_t(const disposeOp_t &op) noexcept : value(op.value) { }
	disposeOp_t(disposeOp_t &&op) noexcept : value(op.value) { }
	disposeOp_t &operator =(const disposeOp_t &op) noexcept { value = op.value; return *this; }
	disposeOp_t &operator =(disposeOp_t &&op) noexcept { value = op.value; return *this; }
	operator _disposeOp_t() const noexcept { return value; }
};

struct APNG_API blendOp_t final
{
public:
	enum _blendOp_t { source, over };

private:
	_blendOp_t value;

public:
	constexpr blendOp_t() noexcept : value(source) { }
	blendOp_t(const uint8_t op);
	blendOp_t(const blendOp_t &op) noexcept : value(op.value) { }
	blendOp_t(blendOp_t &&op) noexcept : value(op.value) { }
	blendOp_t &operator =(const blendOp_t &op) noexcept { value = op.value; return *this; }
	blendOp_t &operator =(blendOp_t &&op) noexcept { value = op.value; return *this; }
	operator _blendOp_t() const noexcept { return value; }
};

struct acTL_t final
{
private:
	uint32_t _frames;
	uint32_t _loops;

	acTL_t(const uint8_t *const data) noexcept;

public:
	constexpr acTL_t() noexcept : _frames(1), _loops(0) { }
	acTL_t(acTL_t &&acTL) noexcept : _frames(acTL._frames), _loops(acTL._loops) { }
	acTL_t(const acTL_t &) = delete;
	acTL_t &operator =(const acTL_t &) = delete;
	void operator =(acTL_t &&acTL) noexcept
	{
		_frames = acTL._frames;
		_loops = acTL._loops;
	}

	static acTL_t reinterpret(const chunk_t &chunk);
	void check(const std::vector<chunk_t> &chunks) const;

	uint32_t frames() const noexcept { return _frames; }
	uint32_t loops() const noexcept { return _loops; }
};

struct fcTL_t final
{
private:
	uint32_t _frame;
	uint32_t _sequenceIndex;
	uint32_t _width;
	uint32_t _height;
	uint32_t _xOffset;
	uint32_t _yOffset;
	uint16_t _delayN;
	uint16_t _delayD;
	disposeOp_t _disposeOp;
	blendOp_t _blendOp;

	fcTL_t(const uint8_t *const data, const uint32_t frame) noexcept;

public:
	constexpr fcTL_t() noexcept : _frame(0), _sequenceIndex(0), _width(0), _height(0), _xOffset(0),
		_yOffset(0), _delayN(0), _delayD(0), _disposeOp(), _blendOp() { }
	fcTL_t(fcTL_t &&fcTL) noexcept : _frame(fcTL._frame), _sequenceIndex(fcTL._sequenceIndex),
		_width(fcTL._width), _height(fcTL._height), _xOffset(fcTL._xOffset), _yOffset(fcTL._yOffset),
		_delayN(fcTL._delayN), _delayD(fcTL._delayD), _disposeOp(fcTL._disposeOp), _blendOp(fcTL._blendOp) { }
	fcTL_t(const fcTL_t &) = delete;
	fcTL_t &operator =(const fcTL_t &) = delete;
	void operator =(fcTL_t &&fcTL) noexcept
	{
		_frame = fcTL._frame;
		_sequenceIndex = fcTL._sequenceIndex;
		_width = fcTL._width;
		_height = fcTL._height;
		_xOffset = fcTL._xOffset;
		_yOffset = fcTL._yOffset;
		_delayN = fcTL._delayN;
		_delayD = fcTL._delayD;
		_disposeOp = fcTL._disposeOp;
		_blendOp = fcTL._blendOp;
	}

	static fcTL_t reinterpret(const chunk_t &chunk, const uint32_t frame);
	void check(const uint32_t pngWidth, const uint32_t pngHeight, const bool first = false);

	uint32_t sequenceIndex() const noexcept { return _sequenceIndex; }
	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
	uint32_t xOffset() const noexcept { return _xOffset; }
	uint32_t yOffset() const noexcept { return _yOffset; }
	uint16_t delayN() const noexcept { return _delayN; }
	uint16_t delayD() const noexcept { return _delayD; }
	disposeOp_t disposeOp() const noexcept { return _disposeOp; }
	blendOp_t blendOp() const noexcept { return _blendOp; }
};

enum class pixelFormat_t : uint8_t
{
	format8bppGrey,
	format16bppGrey,
	format8bppGreyA,
	format16bppGreyA,
	format24bppRGB,
	format32bppRGBA,
	format48bppRGB,
	format64bppRGBA
};

struct APNG_API displayTime_t final
{
private:
	const uint32_t delayN, delayD;

public:
	constexpr displayTime_t(const uint32_t N, const uint32_t D) noexcept : delayN{N}, delayD{D} { }
	displayTime_t(const displayTime_t &time) noexcept : delayN{time.delayN}, delayD{time.delayD} { }
	void waitFor() const noexcept;

	displayTime_t(displayTime_t &&) = delete;
	displayTime_t &operator =(const displayTime_t &) = delete;
	displayTime_t &operator =(displayTime_t &&) = delete;
};

struct APNG_API bitmap_t final
{
private:
	std::unique_ptr<uint8_t []> _data;
	const uint32_t _width, _height;
	const pixelFormat_t _format;
	bool transValueValid;
	uint16_t transValue[3];

public:
	bitmap_t(const uint32_t width, const uint32_t height, const pixelFormat_t format);
	const uint8_t *data() const noexcept { return _data.get(); }
	uint8_t *data() noexcept { return _data.get(); }
	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
	pixelFormat_t format() const noexcept { return _format; }
	bool hasTransparency() const noexcept { return transValueValid; }
	template<typename T> T transparent() const noexcept { return T(); }
	void transparent(const uint16_t *const value) noexcept
	{
		transValue[0] = value[0];
		transValue[1] = value[1];
		transValue[2] = value[2];
		transValueValid = true;
	}
};

struct APNG_API apng_t final
{
private:
	using chunkList_t = std::vector<chunk_t>;
	using chunkIter_t = chunkList_t::const_iterator;

	uint32_t _width;
	uint32_t _height;
	bitDepth_t _bitDepth;
	colourType_t _colourType;
	interlace_t _interlacing;
	acTL_t controlChunk;
	bitmap_t *_defaultFrame;
	std::vector<std::pair<fcTL_t, std::unique_ptr<bitmap_t>>> _frames;
	std::unique_ptr<bitmap_t> defaultFrameStorage;
	bool transColourValid;
	uint16_t transColour[3];

public:
	apng_t(stream_t &stream);

	uint32_t width() const noexcept { return _width; }
	uint32_t height() const noexcept { return _height; }
	bitDepth_t bitDepth() const noexcept { return _bitDepth; }
	colourType_t colourType() const noexcept { return _colourType; }
	interlace_t interlacing() const noexcept { return _interlacing; }
	const bitmap_t *defaultFrame() const noexcept { return _defaultFrame; }
	pixelFormat_t pixelFormat() const;
	uint32_t loops() const noexcept { return controlChunk.loops(); }
	std::vector<std::pair<const displayTime_t, const bitmap_t *const>> frames() const noexcept;

private:
	void checkSig(stream_t &stream);
	void validateHeader();

	bool processFrame(stream_t &stream, bitmap_t &frame);
	uint32_t processDefaultFrame(const chunkList_t &chunks, const bool isSequenceFrame, const chunk_t &controlChunk);
	void processFrame(const chunkIter_t &chunkBegin, const chunkIter_t &chunkEnd, const uint32_t frameIndex,
		const chunk_t &controlChunk);
};

struct APNG_API invalidPNG_t : public std::exception
{
public:
	invalidPNG_t() noexcept = default;
	const char *what() const noexcept { return "Invalid PNG file"; }
};

#endif /*APNG_HXX*/
