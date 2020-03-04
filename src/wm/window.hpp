#pragma once
#include "manager.hpp"
#include "client.hpp"

#include <vector>
#include <list>

namespace Awning::WM
{
	class Window
	{
	public:
		class Manager
		{
		public:
			enum class Layer {
				Background,
				Bottom,
				Top,
				Overlay,
				Application,
				END,
			};

			static std::list<Window*> windowList                  ;
			static Window*            hoveredOver                 ;
			static std::list<Window*> layers     [(int)Layer::END];

			struct Functions
			{
				typedef void (*Resized)(void* data, int width, int height);
				typedef void (*Raised )(void* data                       );
				typedef void (*Moved  )(void* data, int x, int y         );
			};

			static void Manage  (Window*& window, Layer layer = Layer::Application);
			static void Unmanage(Window*& window                                  );

			static void Raise (Window*& window                      );
			static void Move  (Window*& window, int xPos , int yPos );
			static void Resize(Window*& window, int xSize, int ySize);
			static void Offset(Window*& window, int xOff , int yOff );
		};
	
	private:
		friend Manager;

		friend void  Client::Bind::Window(void* id, Awning::WM::Window* window        );
		friend void  Client::Unbind::Window(        Awning::WM::Window* window        );
		friend void* Client::Get::Surface(          Awning::WM::Window* window        );
		friend void  Client::Bind::Surface(         Awning::WM::Window* window, void* );

		Manager::Functions::Resized Resized;
		Manager::Functions::Raised  Raised ;
		Manager::Functions::Moved   Moved  ;

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

		Manager::Layer layer;

	public:		
		static Window* Create        (void* client   );
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

		void SetRaised (Manager::Functions::Raised  raised );
		void SetResized(Manager::Functions::Resized resized);
		void SetMoved  (Manager::Functions::Moved   moved  );
	};
}