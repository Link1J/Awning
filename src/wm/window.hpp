#pragma once
#include "manager.hpp"
#include "client.hpp"

namespace Awning::WM
{
	class Window
	{
		friend void  Manager::Window::Raise (Awning::WM::Window* window        );
		friend void  Manager::Window::Resize(Awning::WM::Window* window,int,int);
		friend void  Client::Bind(void* id,  Awning::WM::Window* window        );
		friend void  Client::Unbind(         Awning::WM::Window* window        );
		friend void* Client::Surface(        Awning::WM::Window* window        );

		Manager::Functions::Window::Resized Resized;
		Manager::Functions::Window::Raised  Raised ;

		Texture::Data texture;

		int xPos, yPos, xSize, ySize;
		bool mapped, needsFrame;
		void* data,* client;

	public:
		static Window* Create (void* client   );
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
		void*          Client    (                    );

		void SetRaised (Manager::Functions::Window::Raised  raised );
		void SetResized(Manager::Functions::Window::Resized resized);
	};
}