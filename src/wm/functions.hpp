#pragma once

namespace Awning::Functions
{
	typedef void (*Resized)(void* data, int width, int height      );
	typedef void (*Raised )(void* data                             );
	typedef void (*Moved  )(void* data, int x, int y               );
	typedef void (*Button )(void* data, uint32_t button, bool state);
	typedef void (*Scroll )(void* data, uint32_t axis, int step    );
	typedef void (*Enter  )(void* data, void* object, int x, int y );
	typedef void (*Leave  )(void* data, void* object               );
	typedef void (*Frame  )(void* data                             );
	typedef void (*Lowered)(void* data                             );
};