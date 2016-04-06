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

chunk_t chunk_t::loadChunk(stream_t &stream) noexcept
{
	chunk_t chunk;
	if (!stream.read(chunk._length) ||
		!stream.read(chunk._chunkType.type()))
		throw invalidPNG_t();
	swap(chunk._length);
	chunk._chunkData.reset(new uint8_t[chunk._length]);
	uint32_t crcRead, crcCalc;
	if (!stream.read(chunk._chunkData.get(), chunk._length) ||
		!stream.read(crcRead))
		throw invalidPNG_t();
	swap(crcRead);
	crc32_t::crc(crcCalc = 0, chunk._chunkType.type());
	crc32_t::crc(crcCalc, chunk._chunkData.get(), chunk._length);
	if (crcCalc != crcRead)
		throw invalidPNG_t();
	return chunk;
}

constexpr static const std::array<uint8_t, 8> pngSig =
	{ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

constexpr static const chunkType_t typeIHDR{'I', 'H', 'D', 'R'};
constexpr static const chunkType_t typePLTE{'P', 'L', 'T', 'E'};
constexpr static const chunkType_t typeACTL{'a', 'c', 'T', 'L'};
constexpr static const chunkType_t typeIDAT{'I', 'D', 'A', 'T'};
constexpr static const chunkType_t typeFCTL{'f', 'c', 'T', 'L'};
constexpr static const chunkType_t typeFDAT{'f', 'd', 'A', 'T'};
constexpr static const chunkType_t typeIEND{'I', 'E', 'N', 'D'};

bool isIHDR(const chunk_t &chunk) noexcept { return chunk.type() == typeIHDR; }
bool isPLTE(const chunk_t &chunk) noexcept { return chunk.type() == typePLTE; }
bool isACTL(const chunk_t &chunk) noexcept { return chunk.type() == typeACTL; }
bool isIDAT(const chunk_t &chunk) noexcept { return chunk.type() == typeIDAT; }
bool isFCTL(const chunk_t &chunk) noexcept { return chunk.type() == typeFCTL; }
bool isIEND(const chunk_t &chunk) noexcept { return chunk.type() == typeIEND; }
bool isFDAT(const chunk_t &chunk) noexcept { return chunk.type() == typeFDAT; }

bitmap_t::bitmap_t(const uint32_t width, const uint32_t height, const pixelFormat_t format) :
	_width(width), _height(height), _format(format)
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
		throw invalidPNG_t();
	const size_t length = width * height * bytes;
	_data.reset(new uint8_t[length]);
	memset(_data.get(), 0, length);
}

apng_t::apng_t(stream_t &stream)
{
	chunkList_t chunks;
	checkSig(stream);

	chunk_t header = chunk_t::loadChunk(stream);
	if (!isIHDR(header) || header.length() != 13)
		throw invalidPNG_t();
	const uint8_t *const headerData = header.data();
	_width = swap32(reinterpret_cast<const uint32_t *const>(headerData)[0]);
	_height = swap32(reinterpret_cast<const uint32_t *const>(headerData)[1]);
	_bitDepth = bitDepth_t(headerData[8]);
	_colourType = colourType_t(headerData[9]);
	if (headerData[10] || headerData[11])
		throw invalidPNG_t();
	_interlacing = interlace_t(headerData[12]);
	validateHeader();

	while (!stream.atEOF())
		chunks.emplace_back(chunk_t::loadChunk(stream));

	if (_colourType == colourType_t::palette || _colourType == colourType_t::rgb || _colourType == colourType_t::rgba)
	{
		auto palettes = extract(chunks, isPLTE);
		if ((_colourType == colourType_t::palette && palettes.size() != 1) || palettes.size() > 1)
			throw invalidPNG_t();
		if (palettes.size())
		{
			const chunk_t *palette = palettes[0];
			if ((palette->length() % 3) != 0)
				throw invalidPNG_t();
			// process palette.
		}
	}
	else if (contains(chunks, isPLTE))
		throw invalidPNG_t();

	const chunk_t &end = chunks.back();
	chunks.pop_back();
	if (!isIEND(end) || end.length() != 0)
		throw invalidPNG_t();

	const chunk_t *const acTL = extractFirst(chunks, isACTL);
	if (!acTL || extract(chunks, isACTL).size() != 1 || acTL->length() != 8)
		throw invalidPNG_t();
	controlChunk = acTL_t::reinterpret(*acTL);
	controlChunk.check(chunks);

	if (!contains(chunks, isIDAT) || isAfter(acTL, extractFirst(chunks, isIDAT)) || !contains(chunks, isFCTL))
		throw invalidPNG_t();
	const auto fcTLChunks = extractIters(chunks, isFCTL);
	const chunk_t &fcTL = *fcTLChunks[0];
	uint32_t i = processDefaultFrame(chunks, isBefore(&fcTL, extractFirst(chunks, isIDAT)), fcTL);
	const uint32_t lastFrame = controlChunk.frames() - 1;
	for (; i < controlChunk.frames(); ++i)
		processFrame(fcTLChunks[i], (i == lastFrame ? chunks.end() : fcTLChunks[i + 1]), i, *fcTLChunks[i]);
}

void apng_t::checkSig(stream_t &stream)
{
	std::array<uint8_t, 8> sig;
	stream.read(sig);
	if (sig != pngSig)
		throw invalidPNG_t();
}

void apng_t::validateHeader()
{
	if (!_width || !_height || (_width >> 31) || (_height >> 31))
		throw invalidPNG_t();
	if (_colourType == colourType_t::rgb || _colourType == colourType_t::greyscaleAlpha || _colourType == colourType_t::rgba)
	{
		if (_bitDepth != bitDepth_t::bps8 && _bitDepth != bitDepth_t::bps16)
			throw invalidPNG_t();
	}
	else if (_colourType == colourType_t::palette && _bitDepth == bitDepth_t::bps16)
		throw invalidPNG_t();
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
	else if (_colourType == colourType_t::greyscale || _colourType == colourType_t::greyscaleAlpha)
	{
		if (_bitDepth == bitDepth_t::bps8 || _bitDepth == bitDepth_t::bps4 || _bitDepth == bitDepth_t::bps2 || _bitDepth == bitDepth_t::bps1)
			return pixelFormat_t::format8bppGrey;
		else if (_bitDepth == bitDepth_t::bps16)
			return pixelFormat_t::format16bppGrey;
	}
	throw invalidPNG_t();
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
		/*else*/ if (_bitDepth == bitDepth_t::bps8)
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
	auto dataChunks = extract(chunks, isIDAT);
	size_t offs = 0, dataLength = 0;
	for (const chunk_t *chunk : dataChunks)
		dataLength += chunk->length();
	std::unique_ptr<uint8_t[]> data(new uint8_t[dataLength]);
	for (const chunk_t *chunk : dataChunks)
	{
		memcpy(data.get() + offs, chunk->data(), chunk->length());
		offs += chunk->length();
	}

	memoryStream_t memoryStream(data.get(), dataLength);
	zlibStream_t frameData(memoryStream, zlibStream_t::inflate);
	_defaultFrame = new bitmap_t(_width, _height, pixelFormat());
	if (isSequenceFrame)
	{
		std::unique_ptr<bitmap_t> frame(_defaultFrame);
		fcTL_t fcTL = fcTL_t::reinterpret(controlChunk, 0);
		fcTL.check(_width, _height, true);
		_frames.emplace_back(std::make_pair(std::move(fcTL), std::move(frame)));
	}
	else
		defaultFrameStorage.reset(_defaultFrame);

	if (!processFrame(frameData, *_defaultFrame))
		throw invalidPNG_t();

	// Return what the first unread animation frame index is.
	return isSequenceFrame ? 1 : 0;
}

template<blendOp_t::_blendOp_t op> void compositFrame(const bitmap_t &source, bitmap_t &destination, pixelFormat_t pixelFormat) noexcept
{
	if (pixelFormat == pixelFormat_t::format24bppRGB)
		compFrame(compRGB<pngRGB8_t, op>, source, destination, 0, 0);
	else if (pixelFormat == pixelFormat_t::format48bppRGB)
		compFrame(compRGB<pngRGB16_t, op>, source, destination, 0, 0);
	/*else if (pixelFormat == pixelFormat_t::format32bppRGBA)
		compFrame(compRGBA<pngRGBA8_t, op>, source, destination, 0, 0);
	else if (pixelFormat == pixelFormat_t::format64bppRGBA)
		compFrame(compRGBA<pngRGBA16_t, op>, source, destination, 0, 0);
	else if (pixelFormat == pixelFormat_t::format8bppGrey)
		compFrame<pngGrey8_t, compGrey<pngGrey8_t, compFunc>>(source, destination, 0, 0);
	else if (pixelFormat == pixelFormat_t::format16bppGrey)
		compFrame<pngGrey16_t, compGrey<pngGrey16_t, compFunc>>(source, destination, 0, 0);*/
	/*else if (pixelFormat == pixelFormat_t::bps8)
		compFrame<pngGreyA8_t, readGreyA>(source, destination, 0, 0);
	else if (pixelFormat == pixelFormat_t::bps16)
		compFrame<pngGreyA16_t, readGreyA>(source, destination, 0, 0);*/
}

void apng_t::processFrame(const chunkIter_t &chunkBegin, const chunkIter_t &chunkEnd, const uint32_t frameIndex,
	const chunk_t &controlChunk)
{
	const pixelFormat_t format = pixelFormat();
	fcTL_t fcTL = fcTL_t::reinterpret(controlChunk, frameIndex);
	fcTL.check(_width, _height, frameIndex == 0);

	auto dataChunks = extract(chunkBegin, chunkEnd, isFDAT);
	size_t offs = 0, dataLength = 0, sequenceIndex = fcTL.sequenceIndex();
	for (const chunk_t *chunk : dataChunks)
		dataLength += chunk->length() - 4;
	std::unique_ptr<uint8_t[]> data(new uint8_t[dataLength]);
	for (const chunk_t *chunk : dataChunks)
	{
		const uint8_t *const chunkData = chunk->data();
		const uint32_t sequenceNum = *reinterpret_cast<const uint32_t *>(chunkData);
		if (swap32(sequenceNum) != ++sequenceIndex)
			throw invalidPNG_t();
		memcpy(data.get() + offs, chunkData + 4, chunk->length() - 4);
		offs += chunk->length();
	}

	memoryStream_t memoryStream(data.get(), dataLength);
	zlibStream_t frameData(memoryStream, zlibStream_t::inflate);
	std::unique_ptr<bitmap_t> frame(new bitmap_t(_width, _height, format));
	if (!processFrame(frameData, *frame))
		throw invalidPNG_t();

	/*// This constructs a disposeOp_t::background initialised bitmap anyway.
	std::unique_ptr<bitmap_t> frame(new bitmap_t(_width, _height, format));
	if (fcTL.disposeOp() == disposeOp_t::none && frameIndex != 0)
		compositFrame<blendOp_t::source>(*_frames.back().second, *frame, format);
	else if (fcTL.disposeOp() == disposeOp_t::previous)
	{
		// TODO: make me loop back over all previous frames till disposeOp != previous.
		// at this point, then copy the contents of that frame in blendOp_t::source mode.
	}

	if (fcTL.blendOp() == blendOp_t::source && fcTL.disposeOp() != disposeOp_t::background)
		compositFrame<blendOp_t::source>(partialFrame, *frame, format);
	else
		compositFrame<blendOp_t::over>(partialFrame, *frame, format);*/
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

acTL_t::acTL_t(const uint32_t *const data) noexcept
{
	_frames = swap32(data[0]);
	_loops = swap32(data[1]);
}

void acTL_t::check(const std::vector<chunk_t> &chunks) const
{
	if (_frames != extract(chunks, isFCTL).size() && !_frames)
		throw invalidPNG_t();
}

acTL_t acTL_t::reinterpret(const chunk_t &chunk)
{
	if (chunk.length() != 8)
		throw invalidPNG_t();
	return acTL_t(reinterpret_cast<const uint32_t *const>(chunk.data()));
}

fcTL_t::fcTL_t(const uint8_t *const data, const uint32_t frame) noexcept : _frame(frame)
{
	const uint32_t *const data32 = reinterpret_cast<const uint32_t *const>(data);
	const uint16_t *const data16 = reinterpret_cast<const uint16_t *const>(data);
	_sequenceIndex = swap32(data32[0]);
	_width = swap32(data32[1]);
	_height = swap32(data32[2]);
	_xOffset = swap32(data32[3]);
	_yOffset = swap32(data32[4]);
	_delayN = swap16(data16[10]);
	_delayD = swap16(data16[11]);
	_disposeOp = disposeOp_t(data[24]);
	_blendOp = blendOp_t(data[25]);
}

fcTL_t fcTL_t::reinterpret(const chunk_t &chunk, const uint32_t frame)
{
	if (chunk.length() != 26)
		throw invalidPNG_t();
	return fcTL_t(chunk.data(), frame);
}

void fcTL_t::check(const uint32_t pngWidth, const uint32_t pngHeight, const bool first)
{
	if (!_width || !_height || (_xOffset + _width) > pngWidth || (_yOffset + _height) > pngHeight)
		throw invalidPNG_t();
	if (first)
	{
		if (_width != pngWidth || _height != pngHeight || _xOffset || _yOffset || _sequenceIndex || _frame)
			throw invalidPNG_t();
		if (_disposeOp = disposeOp_t::previous)
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

invalidPNG_t::invalidPNG_t() noexcept { }

const char *invalidPNG_t::what() const noexcept
{
	return "Invalid PNG file";
}