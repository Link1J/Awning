#pragma once
#include "manager.hpp"
#include "client.hpp"

#include <vector>

namespace Awning::WM
{
	class Window
	{
		friend void  Manager::Window::Raise (       Awning::WM::Window* window        );
		friend void  Manager::Window::Resize(       Awning::WM::Window* window,int,int);
		friend void  Client::Bind::Window(void* id, Awning::WM::Window* window        );
		friend void  Client::Unbind::Window(        Awning::WM::Window* window        );
		friend void* Client::Get::Surface(          Awning::WM::Window* window        );
		friend void  Client::Bind::Surface(         Awning::WM::Window* window, void* );
		friend void  Manager::Window::Reposition(   Awning::WM::Window* window,int,int);

		Manager::Functions::Window::Resized Resized;
		Manager::Functions::Window::Raised  Raised ;
		Manager::Functions::Window::Moved   Moved  ;

		WM::Texture* texture;
		WM::Window * parent;
		bool parentOffsets;

		struct {
			int x, y;
		} minSize, maxSize, size, pos, offset;

		bool mapped, needsFrame;
		void* data,* client;

		std::vector<Window*> subwindows;
		bool drawingManaged = false;

	public:
		static Window* Create        (void* client   );
		static Window* CreateUnmanged(void* client   );
		static void    Destory       (Window*& window);

		WM::Texture*         Texture       (                                       );
		void                 Mapped        (bool map                               );
		bool                 Mapped        (                                       );
		int                  XPos          (                                       );
		int                  YPos          (                                       );
		int                  XSize         (                                       );
		int                  YSize         (                                       );
		void                 Frame         (bool frame                             );
		bool                 Frame         (                                       );
		void                 ConfigPos     (int xPos, int yPos, bool offset = false);
		void                 ConfigSize    (int xSize, int ySize                   );
		void                 Data          (void* data                             );
		void*                Client        (                                       );
		int                  XOffset       (                                       );
		int                  YOffset       (                                       );
		void                 Texture       (WM::Texture* texture                   );
		void                 ConfigMinSize (int xSize, int ySize                   );
		void                 ConfigMaxSize (int xSize, int ySize                   );
		void                 Parent        (WM::Window* parent, bool offsets       );
		void                 AddSubwindow  (WM::Window* child                      );
		std::vector<Window*> GetSubwindows (                                       );
		bool                 DrawingManaged(                                       );

		void SetRaised (Manager::Functions::Window::Raised  raised );
		void SetResized(Manager::Functions::Window::Resized resized);
		void SetMoved  (Manager::Functions::Window::Moved   moved  );
	};
}