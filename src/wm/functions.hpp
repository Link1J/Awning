#pragma once

namespace Awning::Functions
{
	typedef void (*Resized)(void* data, int width, int height);
	typedef void (*Raised )(void* data                       );
	typedef void (*Moved  )(void* data, int x, int y         );
};