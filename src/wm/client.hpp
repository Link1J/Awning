#pragma once
#include <vector>
#include <unordered_set>

namespace Awning::WM { class Window; }

namespace Awning::WM::Client
{
	void Create(void* id                 );
	void SetWM (void* id, void  * wm     );

	namespace Bind 
	{
		void Window  (void      * id    , WM::Window* window );
		void Pointer (void      * id    , void      * pointer);
		void Keyboard(void      * id    , void      * pointer);
		void Surface (WM::Window* window, void      * pointer);
	}

	namespace Unbind 
	{
		void Window  (          WM::Window* window );
		void Pointer (void* id, void      * pointer);
		void Keyboard(void* id, void      * pointer);
	}

	namespace Get 
	{
		namespace All 
		{ 
			std::vector       <void      *> Client   (        );
			std::unordered_set<void      *> Pointers (void* id);
			std::unordered_set<void      *> Keyboards(void* id);
			std::vector       <WM::Window*> Windows  (void* id);
		}

		void* Surface(Window* window);
		void* WM     (void  * id    );
	}
}