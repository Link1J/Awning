#pragma once
#include <stdint.h>

namespace Awning::WM
{
	struct Texture
	{
		uintptr_t size;
		uintptr_t bitsPerPixel;
		uintptr_t bytesPerLine;
		struct 
		{
			uintptr_t size;
			uintptr_t offset;
		} 
		red, green, blue, alpha;

		uintptr_t width;
		uintptr_t height;

		union
		{
			uint8_t * u8 ;
			uint16_t* u16;
			uint32_t* u32;
		} buffer;
	};
		
	struct Damage
	{ 
		int xp, yp, xs, ys; 
	} ;
}