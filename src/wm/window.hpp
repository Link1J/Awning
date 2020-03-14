#pragma once
#include "texture.hpp"
#include "client.hpp"
#include "functions.hpp"
#include <vector>
#include <list>

namespace Awning
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
			static std::list<Window*> layers     [(int)Layer::END];

			static void Manage  (Window*& window, Layer layer = Layer::Application);
			static void Unmanage(Window*& window                                  );

			static void Raise  (Window*& window                      );
			static void Move   (Window*& window, int xPos , int yPos );
			static void Resize (Window*& window, int xSize, int ySize);
			static void Offset (Window*& window, int xOff , int yOff );
			static void Lowered(Window*& window                      );
		};
	
	private:
		friend Manager;

		friend void  Client::Bind::Window(void* id, Awning::Window* window        );
		friend void  Client::Unbind::Window(        Awning::Window* window        );
		friend void* Client::Get::Surface(          Awning::Window* window        );
		friend void  Client::Bind::Surface(         Awning::Window* window, void* );

		Functions::Resized Resized;
		Functions::Raised  Raised ;
		Functions::Moved   Moved  ;
		Functions::Lowered Lowered;

		Awning::Texture* texture;
		Window* parent;
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
		static Window* Create (void* client   );
		static void    Destory(Window*& window);
		static bool    Valid  (Window*& window);

		Awning::Texture*     Texture       (                            );
		void                 Mapped        (bool map                    );
		bool                 Mapped        (                            );
		int                  XPos          (                            );
		int                  YPos          (                            );
		int                  XSize         (                            );
		int                  YSize         (                            );
		void                 Frame         (bool frame                  );
		bool                 Frame         (                            );
		void                 Data          (void* data                  );
		void*                Client        (                            );
		int                  XOffset       (                            );
		int                  YOffset       (                            );
		void                 Texture       (Awning::Texture* texture    );
		void                 ConfigMinSize (int xSize, int ySize        );
		void                 ConfigMaxSize (int xSize, int ySize        );
		void                 Parent        (Window* parent, bool offsets);
		void                 AddSubwindow  (Window* child               );
		std::vector<Window*> GetSubwindows (                            );
		bool                 DrawingManaged(                            );

		void SetRaised (Functions::Raised  raised );
		void SetResized(Functions::Resized resized);
		void SetMoved  (Functions::Moved   moved  );
		void SetLowered(Functions::Lowered lowered);
	};
}