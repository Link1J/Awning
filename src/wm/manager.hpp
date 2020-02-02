#pragma once

#include "texture.hpp"

#include <list>
#include <stdint.h>

namespace Awning::WM { class Window; }

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

namespace Awning::WM::Manager
{
	namespace Handle
	{
		namespace Input
		{
			namespace Mouse
			{
				void Scroll  (int axis, bool direction, float step);
				void Moved   (int x, int y                        );
				void Pressed (uint32_t button                     );
				void Released(uint32_t button                     );
			}

			namespace Keyboard
			{
				void Pressed (uint32_t key);
				void Released(uint32_t key);
			}
		}
	}

	namespace Window
	{
		void Add   (Awning::WM::Window* window);
		void Remove(Awning::WM::Window* window);
		void Raise (Awning::WM::Window* window);

		std::list<Awning::WM::Window*> Get();
	}
}
