#include <chrono>
#include <thread>
#include <memory.h>
#include "crc32.hxx"
#include "utilities.hxx"
#include "apng.hxx"

bool chunkType_t::operator ==(const uint8_t *const value) const noexcept { return memcmp(value, _type.data(), _type.size()) == 0; }
bool chunkType_t::operator !=(const uint8_t *const value) const noexcept { return memcmp(value, _type.data(), _type.size()) != 0; }

chunk_t &chunk_t::operator =(chunk_t &&chunk) noexcept
{
	_length = chunk._length;
	_chunkType = chunk._chunkType;
	_chunkData.swap(chunk._chunkData);
	return *this;
}

chunk_t chunk_t::loadChunk(stream_t &stream)
{
	chunk_t chunk;
	if (!stream.read(chunk._length) ||
		!stream.read(chunk._chunkType.type()))
		throw invalidPNG_t{};
	swap(chunk._length);
	chunk._chunkData = makeUnique<uint8_t []>(chunk._length);
	uint32_t crcRead, crcCalc;
	if (!stream.read(chunk._chunkData.get(), chunk._length) ||
		!stream.read(crcRead))
		throw invalidPNG_t{};
	swap(crcRead);
	crc32_t::crc(crcCalc = 0, chunk._chunkType.type());
	crc32_t::crc(crcCalc, chunk._chunkData.get(), chunk._length);
	if (crcCalc != crcRead)
		throw invalidPNG_t{};
	return chunk;
}

struct chunkStream_t final : public stream_t
{
public:
	using chunkList_t = std::vector<const chunk_t *>;

private:
	const chunkList_t _chunks;
	size_t chunk, pos;
	const bool isSequence;
	size_t sequenceIndex;

	size_t length() const noexcept { return _chunks[chunk]->length() - (isSequence ? 4 : 0); }
	const uint8_t *data() const noexcept { return _chunks[chunk]->data() + (isSequence ? 4 : 0); }

public:
	explicit chunkStream_t(chunkList_t &&chunks, const bool sequence = false, const size_t seqIndex = 0) noexcept :
		_chunks{std::move(chunks)}, chunk{}, pos{}, isSequence{sequence}, sequenceIndex{seqIndex} { }

	bool read(void *const value, const size_t valueLen, size_t &actualLen) final override
	{
		if (atEOF())
			return false;

		auto buffer = static_cast<uint8_t *>(value);
		actualLen = 0;
		while (actualLen < valueLen && !atEOF())
		{
			const size_t valueDelta = valueLen - actualLen;
			const size_t chunkDelta = length() - pos;
			const size_t amount = valueDelta > chunkDelta ? chunkDelta : valueDelta;
			if (isSequence && pos == 0)
			{
				const uint32_t sequenceNum = read32(_chunks[chunk]->data());
				if (sequenceNum != ++sequenceIndex)
					throw invalidPNG_t{};
			}

			memcpy(buffer + actualLen, data() + pos, amount);
			if (amount == chunkDelta)
			{
				++chunk;
				pos = 0;
			}
			else
				pos += amount;
			actualLen += amount;
		}
		return true;
	}

	bool atEOF() const noexcept final override { return chunk == _chunks.size(); }
};

constexpr static const std::array<uint8_t, 8> pngSig =
	{ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

constexpr static const chunkType_t typeIHDR{'I', 'H', 'D', 'R'};
constexpr static const chunkType_t typePLTE{'P', 'L', 'T', 'E'};
constexpr static const chunkType_t typeTRNS{'t', 'R', 'N', 'S'};
constexpr static const chunkType_t typeACTL{'a', 'c', 'T', 'L'};
constexpr static const chunkType_t typeIDAT{'I', 'D', 'A', 'T'};
constexpr static const chunkType_t typeFCTL{'f', 'c', 'T', 'L'};
constexpr static const chunkType_t typeFDAT{'f', 'd', 'A', 'T'};
constexpr static const chunkType_t typeIEND{'I', 'E', 'N', 'D'};

bool isIHDR(const chunk_t &chunk) noexcept { return chunk.type() == typeIHDR; }
bool isPLTE(const chunk_t &chunk) noexcept { return chunk.type() == typePLTE; }
bool isTRNS(const chunk_t &chunk) noexcept { return chunk.type() == typeTRNS; }
bool isACTL(const chunk_t &chunk) noexcept { return chunk.type() == typeACTL; }
bool isIDAT(const chunk_t &chunk) noexcept { return chunk.type() == typeIDAT; }
bool isFCTL(const chunk_t &chunk) noexcept { return chunk.type() == typeFCTL; }
bool isIEND(const chunk_t &chunk) noexcept { return chunk.type() == typeIEND; }
bool isFDAT(const chunk_t &chunk) noexcept { return chunk.type() == typeFDAT; }

constexpr static uint64_t uint64Max = std::numeric_limits<uint64_t>::max();

inline uint64_t safeMul(const uint64_t a, const uint64_t b) noexcept
{
	if (a == uint64Max || b == uint64Max)
		return uint64Max;
	// This uses the first step in the Karatsuba decomposition method of multiplication
	// to determine if the multiplication would set any bits above the max for uint64_t
	const uint64_t c = (a >> 32U) * (b >> 32U);
	if (c)
		return uint64Max;
	return a * b;
}

template<typename ...values_t> uint64_t safeMul(const uint64_t a, const uint64_t b, values_t &&...values) noexcept
	{ return safeMul(safeMul(a, b), values...); }

bitmap_t::bitmap_t(const uint32_t width, const uint32_t height, const pixelFormat_t format) :
	_data{}, _width(width), _height(height), _format(format), transValueValid(false), transValue{}
{
	uint8_t bytes;
	if (_format == pixelFormat_t::format8bppGrey)
		bytes = 1;
	else if (_format == pixelFormat_t::format16bppGrey)
		bytes = 2;
	else if (_format == pixelFormat_t::format24bppRGB)
		bytes = 3;
	else if (_format == pixelFormat_t::format32bppRGBA)
		bytes = 4;
	else if (_format == pixelFormat_t::format48bppRGB)
		bytes = 6;
	else if (_format == pixelFormat_t::format64bppRGBA)
		bytes = 8;
	else
		throw invalidPNG_t{};
	const uint64_t length = safeMul(width, height, bytes);
	if (length == uint64Max)
		throw std::bad_alloc{};
	_data = makeUnique<uint8_t []>(length);
	memset(_data.get(), 0, length);
}

apng_t::apng_t(stream_t &stream) : _defaultFrame{}, transColourValid(false), transColour{}
{
	chunkList_t chunks;
	checkSig(stream);

	chunk_t header = chunk_t::loadChunk(stream);
	if (!isIHDR(header) || header.length() != 13)
		throw invalidPNG_t{};
	const auto headerData = header.data();
	_width = read32(&headerData[0]);
	_height = read32(&headerData[4]);
	_bitDepth = {headerData[8]};
	_colourType = {headerData[9]};
	if (headerData[10] || headerData[11])
		throw invalidPNG_t{};
	_interlacing = {headerData[12]};
	validateHeader();

	while (!stream.atEOF())
		chunks.emplace_back(chunk_t::loadChunk(stream));

	if (_colourType == colourType_t::palette || _colourType == colourType_t::rgb || _colourType == colourType_t::rgba)
	{
		auto palettes = extract(chunks, isPLTE);
		if ((_colourType == colourType_t::palette && palettes.size() != 1) || palettes.size() > 1)
			throw invalidPNG_t{};
		if (!palettes.empty())
		{
			const chunk_t *palette = palettes[0];
			if ((palette->length() % 3) != 0)
				throw invalidPNG_t{};
			// process palette.
		}
	}
	else if (contains(chunks, isPLTE))
		throw invalidPNG_t{};

	if (_colourType == colourType_t::rgb || _colourType == colourType_t::greyscale)
	{
		auto transChunks = extract(chunks, isTRNS);
		if (transChunks.size() > 1)
			throw invalidPNG_t{};
		if (!transChunks.empty())
		{
			const chunk_t *trans = transChunks[0];
			if ((_colourType == colourType_t::rgb && trans->length() != 6) ||
				(_colourType == colourType_t::greyscale && trans->length() != 2))
				throw invalidPNG_t{};
			const auto colour = trans->data();
			if (_colourType == colourType_t::rgb)
			{
				transColour[0] = read16(&colour[0]);
				transColour[1] = read16(&colour[2]);
				transColour[2] = read16(&colour[4]);
			}
			else
				transColour[0] = read16(colour);
			transColourValid = true;
		}
	}

	const chunk_t &end = chunks.back();
	chunks.pop_back();
	if (!isIEND(end) || end.length() != 0)
		throw invalidPNG_t{};

	const chunk_t *const acTL = extractFirst(chunks, isACTL);
	if (!acTL || extract(chunks, isACTL).size() != 1 || acTL->length() != 8)
		throw invalidPNG_t{};
	controlChunk = acTL_t::reinterpret(*acTL);
	controlChunk.check(chunks);

	if (!contains(chunks, isIDAT) || isAfter(acTL, extractFirst(chunks, isIDAT)) || !contains(chunks, isFCTL))
		throw invalidPNG_t{};
	const auto fcTLChunks = extractIters(chunks, isFCTL);
	const chunk_t &fcTL = *fcTLChunks[0];
	uint32_t i = processDefaultFrame(chunks, isBefore(&fcTL, extractFirst(chunks, isIDAT)), fcTL);
	const uint32_t lastFrame = controlChunk.frames() - 1;
	for (; i < controlChunk.frames(); ++i)
		processFrame(fcTLChunks[i], (i == lastFrame ? chunks.end() : fcTLChunks[i + 1]), i, *fcTLChunks[i]);
}

void apng_t::checkSig(stream_t &stream)
{
	std::array<uint8_t, 8> sig{};
	stream.read(sig);
	if (sig != pngSig)
		throw invalidPNG_t{};
}

void apng_t::validateHeader()
{
	if (!_width || !_height || (_width >> 31U) || (_height >> 31U))
		throw invalidPNG_t{};
	if (_colourType == colourType_t::rgb || _colourType == colourType_t::greyscaleAlpha || _colourType == colourType_t::rgba)
	{
		if (_bitDepth != bitDepth_t::bps8 && _bitDepth != bitDepth_t::bps16)
			throw invalidPNG_t{};
	}
	else if (_colourType == colourType_t::palette && _bitDepth == bitDepth_t::bps16)
		throw invalidPNG_t{};
}

pixelFormat_t apng_t::pixelFormat() const
{
	if (_colourType == colourType_t::rgb)
	{
		if (_bitDepth == bitDepth_t::bps8)
			return pixelFormat_t::format24bppRGB;
		else if (_bitDepth == bitDepth_t::bps16)
			return pixelFormat_t::format48bppRGB;
	}
	else if (_colourType == colourType_t::rgba)
	{
		if (_bitDepth == bitDepth_t::bps8)
			return pixelFormat_t::format32bppRGBA;
		else if (_bitDepth == bitDepth_t::bps16)
			return pixelFormat_t::format64bppRGBA;
	}
	else if (_colourType == colourType_t::palette)
		return pixelFormat_t::format24bppRGB;
	else if (_colourType == colourType_t::greyscale)
	{
		if (_bitDepth == bitDepth_t::bps8 || _bitDepth == bitDepth_t::bps4 || _bitDepth == bitDepth_t::bps2 || _bitDepth == bitDepth_t::bps1)
			return pixelFormat_t::format8bppGrey;
		else if (_bitDepth == bitDepth_t::bps16)
			return pixelFormat_t::format16bppGrey;
	}
	else if (_colourType == colourType_t::greyscaleAlpha)
	{
		if (_bitDepth == bitDepth_t::bps8 || _bitDepth == bitDepth_t::bps4 || _bitDepth == bitDepth_t::bps2 || _bitDepth == bitDepth_t::bps1)
			return pixelFormat_t::format8bppGreyA;
		else if (_bitDepth == bitDepth_t::bps16)
			return pixelFormat_t::format16bppGreyA;
	}
	throw invalidPNG_t{};
}

bool apng_t::processFrame(stream_t &stream, bitmap_t &frame)
{
	void *const data = frame.data();
	const bitmapRegion_t region(frame.width(), frame.height());
	if (_colourType == colourType_t::rgb)
	{
		if (_bitDepth == bitDepth_t::bps8)
			return copyFrame<pngRGB8_t, readRGB>(stream, data, region);
		else if (_bitDepth == bitDepth_t::bps16)
			return copyFrame<pngRGB16_t, readRGB>(stream, data, region);
	}
	else if (_colourType == colourType_t::rgba)
	{
		if (_bitDepth == bitDepth_t::bps8)
			return copyFrame<pngRGBA8_t, readRGBA>(stream, data, region);
		else if (_bitDepth == bitDepth_t::bps16)
			return copyFrame<pngRGBA16_t, readRGBA>(stream, data, region);
	}
	else if (_colourType == colourType_t::greyscale)
	{
		// 1, 2, 4 here..
		/*else*/
		if (_bitDepth == bitDepth_t::bps8)
			return copyFrame<pngGrey8_t, readGrey>(stream, data, region);
		else if (_bitDepth == bitDepth_t::bps16)
			return copyFrame<pngGrey16_t, readGrey>(stream, data, region);
	}
	else if (_colourType == colourType_t::greyscaleAlpha)
	{
		if (_bitDepth == bitDepth_t::bps8)
			return copyFrame<pngGreyA8_t, readGreyA>(stream, data, region);
		else if (_bitDepth == bitDepth_t::bps16)
			return copyFrame<pngGreyA16_t, readGreyA>(stream, data, region);
	}
	return false;
}

uint32_t apng_t::processDefaultFrame(const chunkList_t &chunks, const bool isSequenceFrame, const chunk_t &controlChunk)
{
	chunkStream_t chunkStream(extract(chunks, isIDAT));
	zlibStream_t frameData{chunkStream, zlibStream_t::inflate};
	auto frame = makeUnique<bitmap_t>(_width, _height, pixelFormat());
	_defaultFrame = frame.get();
	if (isSequenceFrame)
	{
		fcTL_t fcTL = fcTL_t::reinterpret(controlChunk, 0);
		fcTL.check(_width, _height, true);
		_frames.emplace_back(std::make_pair(std::move(fcTL), std::move(frame)));
	}
	else
		defaultFrameStorage = std::move(frame);

	if (!processFrame(frameData, *_defaultFrame))
		throw invalidPNG_t{};

	// Return what the first unread animation frame index is.
	return isSequenceFrame ? 1 : 0;
}

template<blendOp_t::_blendOp_t op> void compositFrame(const bitmap_t &source, bitmap_t &destination, const pixelFormat_t pixelFormat, const fcTL_t &fcTL) noexcept
{
	const uint32_t xOffset = fcTL.xOffset();
	const uint32_t yOffset = fcTL.yOffset();
	if (pixelFormat == pixelFormat_t::format24bppRGB)
		compFrame(compRGB<pngRGB8_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format48bppRGB)
		compFrame(compRGB<pngRGB16_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format32bppRGBA)
		compFrame(compRGBA<pngRGBA8_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format64bppRGBA)
		compFrame(compRGBA<pngRGBA16_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format8bppGrey)
		compFrame(compGrey<pngGrey8_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format16bppGrey)
		compFrame(compGrey<pngGrey16_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format8bppGreyA)
		compFrame(compGreyA<pngGreyA8_t, op>, source, destination, xOffset, yOffset);
	else if (pixelFormat == pixelFormat_t::format16bppGreyA)
		compFrame(compGreyA<pngGreyA16_t, op>, source, destination, xOffset, yOffset);
}

void apng_t::processFrame(const chunkIter_t &chunkBegin, const chunkIter_t &chunkEnd, const uint32_t frameIndex,
	const chunk_t &controlChunk)
{
	const pixelFormat_t format = pixelFormat();
	fcTL_t fcTL = fcTL_t::reinterpret(controlChunk, frameIndex);
	fcTL.check(_width, _height, frameIndex == 0);

	chunkStream_t chunkStream(extract(chunkBegin, chunkEnd, isFDAT), true, fcTL.sequenceIndex());
	zlibStream_t frameData(chunkStream, zlibStream_t::inflate);
	bitmap_t partialFrame(fcTL.width(), fcTL.height(), format);
	if (!processFrame(frameData, partialFrame))
		throw invalidPNG_t{};
	if (transColourValid)
		partialFrame.transparent(transColour);

	// This constructs a disposeOp_t::background initialised bitmap anyway.
	std::unique_ptr<bitmap_t> frame(new bitmap_t(_width, _height, format));
	if (fcTL.disposeOp() == disposeOp_t::none && frameIndex != 0)
		compositFrame<blendOp_t::source>(*_frames.back().second, *frame, format, fcTL_t());
	else if (fcTL.disposeOp() == disposeOp_t::previous)
	{
		auto source = _frames.end();
		// Only if there are frames to backtrack over.
		if (source != _frames.begin())
		{
			// Find the first frame that doesn't have disposeOp_t::previous, or is simply the first frame.
			while (--source != _frames.begin() && source->first.disposeOp() == disposeOp_t::previous)
				continue;
			// And composit it in as the basis of this frame.
			compositFrame<blendOp_t::source>(*source->second, *frame, format, fcTL_t());
		}
	}

	if (fcTL.blendOp() == blendOp_t::source || fcTL.disposeOp() == disposeOp_t::background)
		compositFrame<blendOp_t::source>(partialFrame, *frame, format, fcTL);
	else
		compositFrame<blendOp_t::over>(partialFrame, *frame, format, fcTL);
	_frames.emplace_back(std::make_pair(std::move(fcTL), std::move(frame)));
}

std::vector<std::pair<const displayTime_t, const bitmap_t *const>> apng_t::frames() const noexcept
{
	std::vector<std::pair<const displayTime_t, const bitmap_t *const>> frameArray;
	for (const auto &frame : _frames)
	{
		const fcTL_t &fcTL = frame.first;
		frameArray.emplace_back(std::make_pair(displayTime_t(fcTL.delayN(), fcTL.delayD()), frame.second.get()));
	}
	return frameArray;
}

acTL_t::acTL_t(const uint8_t *const data) noexcept : _frames{read32(&data[0])}, _loops{read32(&data[4])} { }

void acTL_t::check(const std::vector<chunk_t> &chunks) const
{
	if (_frames != extract(chunks, isFCTL).size() && !_frames)
		throw invalidPNG_t{};
}

acTL_t acTL_t::reinterpret(const chunk_t &chunk)
{
	if (chunk.length() != 8)
		throw invalidPNG_t{};
	return {chunk.data()};
}

fcTL_t::fcTL_t(const uint8_t *const data, const uint32_t frame) noexcept : _frame(frame), _sequenceIndex{read32(&data[0])},
	_width{read32(&data[4])}, _height{read32(&data[8])}, _xOffset{read32(&data[12])}, _yOffset{read32(&data[16])},
	_delayN{read16(&data[20])}, _delayD{read16(&data[22])}, _disposeOp{data[24]}, _blendOp{data[25]} { }

fcTL_t fcTL_t::reinterpret(const chunk_t &chunk, const uint32_t frame)
{
	if (chunk.length() != 26)
		throw invalidPNG_t{};
	return {chunk.data(), frame};
}

void fcTL_t::check(const uint32_t pngWidth, const uint32_t pngHeight, const bool first)
{
	if (!_width || !_height || (_xOffset + _width) > pngWidth || (_yOffset + _height) > pngHeight)
		throw invalidPNG_t{};
	if (first)
	{
		if (_width != pngWidth || _height != pngHeight || _xOffset || _yOffset || _sequenceIndex || _frame)
			throw invalidPNG_t{};
		if (_disposeOp == disposeOp_t::previous)
			_disposeOp = disposeOp_t::background;
	}
	if (!_delayN)
	{
		_delayN = 1;
		_delayD = 100;
	}
	else if (!_delayD)
		_delayD = 100;
}

void displayTime_t::waitFor() const noexcept
{
	const std::chrono::seconds num(delayN);
	const std::chrono::nanoseconds N(num);
	std::this_thread::sleep_for(N / delayD);
}
