#pragma once
#include "manager/functions.hpp"
#include "manager/manager.hpp"

namespace Awning::WM
{
	class Window
	{
		friend void Manager::Window::Raise(Awning::WM::Window* window);

		Manager::Functions::Window::Resized Resized;
		Manager::Functions::Window::Raised  Raised ;

		Texture::Data texture;

		int xPos, yPos, xSize, ySize;
		bool mapped, needsFrame;
		void* data;

	public:
		static Window* Create (               );
		static void    Destory(Window*& window);

		Texture::Data* Texture   (                    );
		void           Mapped    (bool map            );
		bool           Mapped    (                    );
		int            XPos      (                    );
		int            YPos      (                    );
		int            XSize     (                    );
		int            YSize     (                    );
		void           Frame     (bool frame          );
		bool           Frame     (                    );
		void           ConfigPos (int xPos, int yPos  );
		void           ConfigSize(int xSize, int ySize);
		void           Data      (void* data          );

		void SetRaised(Manager::Functions::Window::Raised raised);
	};
}