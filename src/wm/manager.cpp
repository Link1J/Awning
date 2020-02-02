#include "manager.hpp"

#include <list>

#include "window.hpp"
#include "client.hpp"
#include "log.hpp"

#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

static std::list<Awning::WM::Window*> windowList;
static Awning::WM::Window* hoveredOver;


namespace Awning::WM::Manager
{
	namespace Handle
	{
		namespace Input
		{
			namespace Mouse
			{
				enum WindowParts
				{
					APPLCATION,
					TOP_FRAME_RESIZE,
					BOTTOM_FRAME_RESIZE,
					LEFT_FRAME_RESIZE,
					RIGHT_FRAME_RESIZE,
					TOP_FRAME_MOVE,
					BOTTOM_FRAME_MOVE,
					LEFT_FRAME_MOVE,
					RIGHT_FRAME_MOVE,
				} frame;
				
				void Scroll(int axis, bool direction, float step)
				{
					if (hoveredOver && frame == APPLCATION)
					{
						Wayland::Pointer::Axis(
							(wl_client*)hoveredOver->Client(),
							axis, step * (direction ? 1 : -1)
						);
					}
				}

				void Moved(int x, int y)
				{
					auto curr = windowList.begin();
					while (curr != windowList.end())
					{
						if((*curr)->XPos()                    <= x 
						&& (*curr)->YPos()                    <= y
						&& (*curr)->XPos() + (*curr)->XSize() >  x
						&& (*curr)->YPos() + (*curr)->YSize() >  y
						)
						{
							frame = APPLCATION;
							break;
						}

						// Top frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                    -  1 <= x
						&& (*curr)->YPos ()                    - 10 <= y
						&& (*curr)->XPos () + (*curr)->XSize() +  1 >  x
						&& (*curr)->YPos ()                         >  y
						)
						{
							frame = TOP_FRAME_MOVE;
							break;
						}
						
						// Bottom frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                    -  1 <= x
						&& (*curr)->YPos () + (*curr)->YSize()      <= y
						&& (*curr)->XPos () + (*curr)->XSize() +  1 >  x
						&& (*curr)->YPos () + (*curr)->YSize() +  1 >  y
						)
						{
							frame = BOTTOM_FRAME_RESIZE;
							break;
						}

						// Left frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                    -  1 <= x
						&& (*curr)->YPos ()                    - 10 <= y
						&& (*curr)->XPos ()                         >  x
						&& (*curr)->YPos () + (*curr)->YSize() +  1 >  y
						)
						{
							frame = LEFT_FRAME_RESIZE;
							break;
						}

						// Right frame
						if((*curr)->Frame()
						&& (*curr)->XPos () + (*curr)->XSize()      <= x
						&& (*curr)->YPos ()                    - 10 <= y
						&& (*curr)->XPos () + (*curr)->XSize() +  1 >  x
						&& (*curr)->YPos () + (*curr)->YSize() +  1 >  y
						)
						{
							frame = RIGHT_FRAME_RESIZE;
							break;
						}

						curr++;
					}

					if (hoveredOver != *curr)
					{
						if (hoveredOver)
						{
							Wayland::Pointer::Leave(
								(wl_client  *)hoveredOver->Client(), 
								(wl_resource*)Client::Surface(hoveredOver)
							);
						}

						if (curr != windowList.end())
						{
							int localX = x - (*curr)->XPos();
							int localY = y - (*curr)->YPos();

							Wayland::Pointer::Enter(
								(wl_client  *)(*curr)->Client(), 
								(wl_resource*)Client::Surface(*curr),
								x, y
							);
							Wayland::Pointer::Frame(
								(wl_client*)(*curr)->Client()
							);
						}

						if (curr == windowList.end())
						{
							hoveredOver = nullptr;
						}
						else
						{
							hoveredOver = *curr;
						}
					}
					else if (hoveredOver && frame == APPLCATION)
					{
						int localX = x - hoveredOver->XPos();
						int localY = y - hoveredOver->YPos();

						Wayland::Pointer::Moved(
							(wl_client  *)hoveredOver->Client(),
							x, y
						);
						Wayland::Pointer::Frame(
							(wl_client*)hoveredOver->Client()
						);
					}
				}

				void Pressed(uint32_t button)
				{
					if (hoveredOver != windowList.front())
					{
						if (hoveredOver)
							Manager::Window::Raise(hoveredOver);
						return;
					}
					else if (hoveredOver && frame == APPLCATION)
					{
						Wayland::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, true
						);
					}
				}

				void Released(uint32_t button)
				{
					if (hoveredOver && frame == APPLCATION)
					{
						Wayland::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, false
						);
					}
				}
			}

			namespace Keyboard
			{
				void Pressed(uint32_t key)
				{
					auto window = windowList.front();
					if (window)
					{
						Wayland::Keyboard::Button(
							(wl_client*)window->Client(),
							key, false
						);
					}
				}

				void Released(uint32_t key)
				{
					auto window = windowList.front();
					if (window)
					{
						Wayland::Keyboard::Button(
							(wl_client*)window->Client(),
							key, false
						);
					}
				}
			}
		}
	}

	namespace Window
	{
		void Add(Awning::WM::Window* window)
		{
			windowList.emplace_back(window);
		}

		void Remove(Awning::WM::Window* window)
		{
			auto curr = windowList.begin();
			while (curr != windowList.end())
			{
				if (*curr == window)
					break;
				curr++;
			}
			windowList.erase(curr);
		}

		void Raise(Awning::WM::Window* window)
		{
			auto curr = windowList.begin();
			while (curr != windowList.end())
			{
				if (*curr == window)
					break;
				curr++;
			}

			if (curr == windowList.end())
				return;

			windowList.erase(curr);
			windowList.emplace_front(window);

			if (window->Raised)
				window->Raised(window->data);
		}

		std::list<Awning::WM::Window*> Get()
		{
			return windowList;
		}
	}
}