#pragma once

#include <stdint.h>
#include <functional>

namespace Awning::WM::Manager::Functions
{
	namespace Input
	{
		namespace Mouse
		{
			typedef void (*Enter   )(void* data, int x, int y                        );
			typedef void (*Exit    )(void* data                                      );
			typedef void (*Scroll  )(void* data, int axis, bool direction, float step);
			typedef void (*Moved   )(void* data, int x, int y                        );
			typedef void (*Pressed )(void* data, uint32_t button                     );
			typedef void (*Released)(void* data, uint32_t button                     );
		}

		namespace Keyboard
		{
			typedef void (*Pressed )(void* data, uint32_t key);
			typedef void (*Released)(void* data, uint32_t key);
		}
	}

	namespace Window
	{
		typedef void (*Resized)(void* data, int width, int height);
		typedef void (*Raised )(void* data                       );
	}
}