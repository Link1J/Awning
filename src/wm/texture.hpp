#pragma once
#include <stdint.h>

namespace Awning
{
	struct Texture
	{
		uintptr_t size;
		uintptr_t bitsPerPixel;
		uintptr_t bytesPerLine;
		struct 
		{
			uintptr_t size   = 0;
			uintptr_t offset = 0;
		} 
		red, green, blue, alpha;

		uintptr_t width;
		uintptr_t height;

		struct
		{
			union
			{
				uint8_t* pointer = nullptr;
				uint32_t number;
			};
			bool offscreen;
		} buffer;
	};
		
	struct Damage
	{ 
		uint32_t xp, yp, xs, ys; 
	} ;
}