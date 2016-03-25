#include <memory.h>
#include "crc32.hxx"
#include "utilities.hxx"
#include "apng.hxx"

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
	bool operator ==(const uint8_t *const value) const noexcept { return memcmp(value, _type.data(), _type.size()) == 0; }

	bool operator !=(const chunkType_t &chunk) const noexcept { return _type != chunk._type; }
	bool operator !=(const std::array<uint8_t, 4> &value) const noexcept { return _type != value; }
	bool operator !=(const uint8_t *const value) const noexcept { return memcmp(value, _type.data(), _type.size()) != 0; }

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

public:
	chunk_t(chunk_t &&chunk) noexcept : _length(chunk._length), _chunkType(chunk._chunkType), _chunkData(std::move(chunk._chunkData)) { }
	uint32_t length() const noexcept { return _length; }
	const chunkType_t &type() const noexcept { return _chunkType; }
	const uint8_t *data() const noexcept { return _chunkData.get(); }

	static chunk_t loadChunk(stream_t &stream) noexcept
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
};

constexpr static const std::array<uint8_t, 8> pngSig =
	{ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

constexpr static const chunkType_t typeIHDR{'I', 'H', 'D', 'R'};
constexpr static const chunkType_t typeACTL{'a', 'c', 'T', 'L'};
constexpr static const chunkType_t typeIDAT{'I', 'D', 'A', 'T'};
constexpr static const chunkType_t typeFCTL{'f', 'c', 'T', 'L'};
constexpr static const chunkType_t typeFDAT{'f', 'd', 'A', 'T'};
constexpr static const chunkType_t typeIEND{'I', 'E', 'N', 'D'};

bool isIHDR(const chunk_t &chunk) noexcept { return chunk.type() == typeIHDR; }
bool isACTL(const chunk_t &chunk) noexcept { return chunk.type() == typeACTL; }
bool isIDAT(const chunk_t &chunk) noexcept { return chunk.type() == typeIDAT; }
bool isFCTL(const chunk_t &chunk) noexcept { return chunk.type() == typeFCTL; }
bool isIEND(const chunk_t &chunk) noexcept { return chunk.type() == typeIEND; }

void checkACTL(const acTL_t &self, const std::vector<chunk_t> &chunks);

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

	while (!stream.atEOF())
		chunks.emplace_back(chunk_t::loadChunk(stream));

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
	checkACTL(controlChunk, chunks);
}

void apng_t::checkSig(stream_t &stream)
{
	std::array<uint8_t, 8> sig;
	stream.read(sig);
	if (sig != pngSig)
		throw invalidPNG_t();
}

void checkACTL(const acTL_t &self, const std::vector<chunk_t> &chunks)
{
	if (self.frames() != extract(chunks, isFCTL).size())
		throw invalidPNG_t();
}

acTL_t acTL_t::reinterpret(const uint8_t *const data) noexcept
{
	acTL_t chunk;
	chunk._frames = reinterpret_cast<const uint32_t *const>(data)[0];
	chunk._loops = reinterpret_cast<const uint32_t *const>(data)[1];
	return chunk;
}

invalidPNG_t::invalidPNG_t() noexcept { }

const char *invalidPNG_t::what() const noexcept
{
	return "Invalid PNG file";
}