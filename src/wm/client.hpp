#pragma once
#include <vector>
#include <unordered_set>

namespace Awning { class Window; }

namespace Awning::Client
{
	void Create(void* id                 );
	void SetWM (void* id, void  * wm     );

	namespace Bind 
	{
		void Window  (void          * id    , Awning::Window* window );
		void Pointer (void          * id    , void          * pointer);
		void Keyboard(void          * id    , void          * pointer);
		void Surface (Awning::Window* window, void          * pointer);
	}

	namespace Unbind 
	{
		void Window  (          Awning::Window* window );
		void Pointer (void* id, void          * pointer);
		void Keyboard(void* id, void          * pointer);
	}

	namespace Get 
	{
		namespace All 
		{ 
			std::vector       <void   *> Client   (        );
			std::unordered_set<void   *> Pointers (void* id);
			std::unordered_set<void   *> Keyboards(void* id);
			std::vector       <Window*>  Windows  (void* id);
		}

		void* Surface(Window* window);
		void* WM     (void  * id    );
	}
}