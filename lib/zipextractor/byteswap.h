#include <stdint.h>
#include <SDL_endian.h>

namespace extractor
{
    static uint16_t byteswap(uint16_t num) 		{ return SDL_Swap16(num); }
    static int16_t byteswap(int16_t num) 		{ return SDL_Swap16(num); }
    static uint32_t byteswap(uint32_t num) 		{ return SDL_Swap32(num); }
    static int32_t byteswap(int32_t num) 		{ return SDL_Swap32(num); }
    static uint64_t byteswap(uint64_t num) 		{ return SDL_Swap64(num); }
    static int64_t byteswap(int64_t num) 		{ return SDL_Swap64(num); }
    static float byteswap(float num) 			{ return SDL_SwapFloat(num); }
};
