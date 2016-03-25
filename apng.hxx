#ifndef APNG_HXX
#define APNG_HXX

#include "stream.hxx"

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

public:
	apng_t(stream_t &stream) noexcept;

private:
	void checkSig(stream_t &stream) noexcept;
};

struct invalidPNG_t : public std::exception
{
public:
	invalidPNG_t() noexcept;
	const char *what() const noexcept;
};

#endif /*APNG_HXX*/