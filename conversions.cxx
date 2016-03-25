#include "apng.hxx"

bitDepth_t::bitDepth_t(const uint8_t depth)
{
	if (depth == 1)
		value = bps1;
	else if (depth == 2)
		value = bps2;
	else if (depth == 4)
		value = bps4;
	else if (depth == 8)
		value = bps8;
	else if (depth == 16)
		value = bps16;
	else
		throw invalidPNG_t();
}

colourType_t::colourType_t(const uint8_t type)
{
	if (type == 0)
		value = greyscale;
	else if (type == 2)
		value = rgb;
	else if (type == 3)
		value = palette;
	else if (type == 4)
		value = greyscaleAlpha;
	else if (type == 6)
		value = rgba;
	else
		throw invalidPNG_t();
}

interlace_t::interlace_t(const uint8_t type)
{
	if (type == 0)
		value = none;
	else if (type == 1)
		value = adam7;
	else
		throw invalidPNG_t();
}