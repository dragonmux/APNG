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

	if (!contains(chunks, isIDAT))
		throw invalidPNG_t();

	const chunk_t &end = chunks.back();
	chunks.pop_back();
	if (!isIEND(end) || end.length() != 0)
		throw invalidPNG_t();

	const chunk_t *chunkACTL = extractFirst(chunks, isACTL);
	if (!chunkACTL || extract(chunks, isACTL).size() != 1 || chunkACTL->length() != 8)
		throw invalidPNG_t();
	controlChunk = acTL_t::reinterpret(chunkACTL->data());
	controlChunk.check(chunks);

	processDefaultFrame(chunks);
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

void apng_t::processDefaultFrame(const std::vector<chunk_t> &chunks)
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
}

void acTL_t::check(const std::vector<chunk_t> &chunks)
{
	if (_frames != extract(chunks, isFCTL).size() && !_frames)
		throw invalidPNG_t();
}

acTL_t acTL_t::reinterpret(const uint8_t *const data) noexcept
{
	acTL_t chunk;
	chunk._frames = swap32(reinterpret_cast<const uint32_t *const>(data)[0]);
	chunk._loops = swap32(reinterpret_cast<const uint32_t *const>(data)[1]);
	return chunk;
}

invalidPNG_t::invalidPNG_t() noexcept { }

const char *invalidPNG_t::what() const noexcept
{
	return "Invalid PNG file";
}