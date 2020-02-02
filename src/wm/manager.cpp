#include "manager.hpp"

#include <list>

#include "window.hpp"
#include "client.hpp"
#include "log.hpp"

#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

#include <linux/input.h>

static std::list<Awning::WM::Window*> windowList;
static Awning::WM::Window* hoveredOver;

namespace Awning::WM::Manager
{
	namespace Handle
	{
		namespace Input
		{
			enum InputLock
			{
				UNLOCK,
				LOCK
			};

			namespace Mouse
			{
				WindowAction action;	
				InputLock input;
				int lockButton;
				WindowSide side;

				int preX = 0, preY = 0;
				
				void Scroll(int axis, bool direction, float step)
				{
					if (hoveredOver && action == APPLCATION)
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
					while (curr != windowList.end() && input == UNLOCK)
					{
						if((*curr)->XPos()                    <= x 
						&& (*curr)->YPos()                    <= y
						&& (*curr)->XPos() + (*curr)->XSize() >  x
						&& (*curr)->YPos() + (*curr)->YSize() >  y
						)
						{
							action = APPLCATION;
							break;
						}

						// Top frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                    -  1 <= x
						&& (*curr)->YPos ()                    -  9 <= y
						&& (*curr)->XPos () + (*curr)->XSize() +  1 >  x
						&& (*curr)->YPos ()                         >  y
						)
						{
							action = MOVE;
							break;
						}


						if((*curr)->Frame()
						&& (*curr)->XPos ()                    -  1 <= x
						&& (*curr)->YPos ()                    - 10 <= y
						&& (*curr)->XPos () + (*curr)->XSize() +  1 >  x
						&& (*curr)->YPos ()                    -  9 >  y
						)
						{
							action = MOVE;
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
							action = RESIZE;
							side = BOTTOM;
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
							action = RESIZE;
							side = LEFT;
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
							action = RESIZE;
							side = RIGHT;
							break;
						}

						curr++;
					}

					if (hoveredOver != *curr && input == UNLOCK)
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
					else if (hoveredOver && action == APPLCATION)
					{
						int localX = x - hoveredOver->XPos();
						int localY = y - hoveredOver->YPos();

						Wayland::Pointer::Moved(
							(wl_client*)hoveredOver->Client(),
							x, y
						);
						Wayland::Pointer::Frame(
							(wl_client*)hoveredOver->Client()
						);
					}
					else if (hoveredOver && action == MOVE && input == LOCK)
					{
						int newX = hoveredOver->XPos() + (x - preX);
						int newY = hoveredOver->YPos() + (y - preY);
						hoveredOver->ConfigPos(newX, newY);
					}
					else if (hoveredOver && action == RESIZE && input == LOCK)
					{
						int deltaX = (x - preX);
						int deltaY = (y - preY);
						int newX = hoveredOver->XPos();
						int newY = hoveredOver->YPos();

						switch (side)
						{
						case TOP:
						case BOTTOM:
							newY += deltaY;
							break;
						case LEFT:
						case RIGHT:
							newX += deltaX;
							break;	
						default:
							newX += deltaX;
							newY += deltaY;
							break;
						}

						hoveredOver->ConfigSize(newX, newY);
					}

					preX = x;
					preY = y;
				}

				void Pressed(uint32_t button)
				{
					if (input == UNLOCK)
					{
						lockButton = button;
					}

					if (hoveredOver != windowList.front())
					{
						if (hoveredOver)
							Manager::Window::Raise(hoveredOver);
						return;
					}
					else if (hoveredOver && action == APPLCATION)
					{
						Wayland::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, true
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (hoveredOver)
						{
							Manager::Window::Raise(hoveredOver);
							Wayland::Pointer::Leave(
								(wl_client  *)hoveredOver->Client(), 
								(wl_resource*)Client::Surface(hoveredOver)
							);
						}
						input = LOCK;
					}
				}

				void Released(uint32_t button)
				{
					if (hoveredOver && action == APPLCATION)
					{
						Wayland::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, false
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (button == lockButton)
						{
							hoveredOver = nullptr;
							input = UNLOCK;
						}
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

			void Lock(WindowAction action, WindowSide side)
			{
				if (action == APPLCATION)
					return;
				
				Mouse::action = action;
				Mouse::input  = LOCK  ;
				Mouse::side   = side  ;
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

		void Resize(Awning::WM::Window* window)
		{
			int newX = window->XPos();
			int newY = window->YPos();
			if (window->Resized)
				window->Resized(window->data, newX, newY);
		}

		std::list<Awning::WM::Window*> Get()
		{
			return windowList;
		}
	}
}