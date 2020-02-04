#pragma once

#include "texture.hpp"

#include <list>
#include <stdint.h>

namespace Awning::WM { class Window; }

namespace Frame 
{
	namespace Resize 
	{
		int const top    = 2;
		int const bottom = 2;
		int const left   = 2;
		int const right  = 2;
	}
	namespace Move 
	{
		int const top    = 8;
		int const bottom = 0;
		int const left   = 0;
		int const right  = 0;
	}
	namespace Size 
	{
		int const top    = Resize::top    + Move::top   ;
		int const bottom = Resize::bottom + Move::bottom;
		int const left   = Resize::left   + Move::left  ;
		int const right  = Resize::right  + Move::right ;
	}
}

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
			enum WindowAction
			{
				APPLCATION,
				RESIZE,
				MOVE,
			};
			enum WindowSide
			{
				TOP,
				TOP_LEFT,
				LEFT,
				BOTTOM_LEFT,
				BOTTOM,
				BOTTOM_RIGHT,
				RIGHT,
				TOP_RIGHT,
			};

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

			void Lock(WindowAction action, WindowSide side = TOP);
		}
	}

	namespace Window
	{
		void Add       (Awning::WM::Window* window                      );
		void Remove    (Awning::WM::Window* window                      );
		void Raise     (Awning::WM::Window* window                      );
		void Resize    (Awning::WM::Window* window, int xSize, int ySize);
		void Reposition(Awning::WM::Window* window, int xPos , int yPos );

		std::list<Awning::WM::Window*> Get();
	}
}
