#pragma once

#include "texture.hpp"

namespace Awning::WM::Manager
{
	namespace Functions
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
}

namespace Awning::WM::Window
{
	struct Data
	{
		Manager::Functions::Window::Resized Resized;
		Manager::Functions::Window::Raised  Raised ;

		Texture::Data texture;

		int xPos, yPos, xSize, ySize;
		bool mapped, needsFrame;
		void* data;
	};

	Data* Create    (                                    );
	void  Destory   (Data*& window                       );
	void  SetTexture(Data*  window, Texture::Data texture);
	void  Map       (Data*  window, bool map             );
	void  Raise     (Data*  window                       );
}