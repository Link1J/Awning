#pragma once

#include "../texture.hpp"
#include "functions.hpp"

#include <list>

namespace Awning::WM { class Window; }

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
