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
	std::vector<chunk_t> chunks;
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

	const chunk_t *const chunkACTL = extractFirst(chunks, isACTL);
	if (!chunkACTL || extract(chunks, isACTL).size() != 1 || chunkACTL->length() != 8)
		throw invalidPNG_t();
	controlChunk = acTL_t::reinterpret(*chunkACTL);
	controlChunk.check(chunks);

	if (!contains(chunks, isIDAT) || isAfter(chunkACTL, extractFirst(chunks, isIDAT)) || !contains(chunks, isFCTL))
		throw invalidPNG_t();
	uint32_t i = processDefaultFrame(chunks, isBefore(extractFirst(chunks, isIDAT), extractFirst(chunks, isFCTL)));
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

uint32_t apng_t::processDefaultFrame(const std::vector<chunk_t> &chunks, const bool isSequenceFrame)
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
		_frames.emplace_back(_defaultFrame);
	else
		defaultFrameStorage.reset(_defaultFrame);

	if (!processFrame(frameData, *_defaultFrame))
		throw invalidPNG_t();

	// Return what the first unread animation frame index is.
	return isSequenceFrame ? 1 : 0;
}

acTL_t::acTL_t(const uint32_t *const data) noexcept
{
	_frames = swap32(data[0]);
	_loops = swap32(data[1]);
}

void acTL_t::check(const std::vector<chunk_t> &chunks)
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

fcTL_t::fcTL_t(const uint8_t *const data) noexcept
{
	const uint32_t *const data32 = reinterpret_cast<const uint32_t *const>(data);
	const uint16_t *const data16 = reinterpret_cast<const uint16_t *const>(data);
	_sequenceNum = swap32(data32[0]);
	_width = swap32(data32[1]);
	_height = swap32(data32[2]);
	_xOffset = swap32(data32[3]);
	_yOffset = swap32(data32[4]);
	_delayN = swap16(data16[10]);
	_delayD = swap16(data16[11]);
}

fcTL_t fcTL_t::reinterpret(const chunk_t &chunk)
{
	if (chunk.length() != 26)
		throw invalidPNG_t();
	return fcTL_t(chunk.data());
}

void fcTL_t::check(const uint32_t pngWidth, const uint32_t pngHeight)
{
	if (!_width || !_height || (_xOffset + _width) > pngWidth || (_yOffset + _height) > pngHeight)
		throw invalidPNG_t();
}

invalidPNG_t::invalidPNG_t() noexcept { }

const char *invalidPNG_t::what() const noexcept
{
	return "Invalid PNG file";
}