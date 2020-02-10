#include "manager.hpp"

#include <list>

#include "window.hpp"
#include "client.hpp"
#include "log.hpp"

#include "wayland/pointer.hpp"
#include "wayland/keyboard.hpp"

#include <linux/input.h>
#include "protocols/xdg-shell-protocol.h"

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
						&& (*curr)->XPos ()                                       <= x
						&& (*curr)->YPos ()                    - Frame::Move::top <= y
						&& (*curr)->XPos () + (*curr)->XSize()                    >  x
						&& (*curr)->YPos ()                                       >  y
						)
						{
							action = MOVE;
							break;
						}

						if((*curr)->Frame()
						&& (*curr)->XPos ()                                                            <= x
						&& (*curr)->YPos ()                    - Frame::Move::top - Frame::Resize::top <= y
						&& (*curr)->XPos () + (*curr)->XSize()                                         >  x
						&& (*curr)->YPos ()                    - Frame::Move::top                      >  y
						)
						{
							action = RESIZE;
							side = TOP;
							break;
						}
						
						// Bottom frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                                          <= x
						&& (*curr)->YPos () + (*curr)->YSize()                       <= y
						&& (*curr)->XPos () + (*curr)->XSize()                       >  x
						&& (*curr)->YPos () + (*curr)->YSize() + Frame::Move::bottom >  y
						)
						{
							action = MOVE;
							break;
						}

						if((*curr)->Frame()
						&& (*curr)->XPos ()                                                                  <= x
						&& (*curr)->YPos () + (*curr)->YSize() + Frame::Move::bottom                         <= y
						&& (*curr)->XPos () + (*curr)->XSize()                                               >  x
						&& (*curr)->YPos () + (*curr)->YSize() + Frame::Move::bottom + Frame::Resize::bottom >  y
						)
						{
							action = RESIZE;
							side = BOTTOM;
							break;
						}

						// Left frame
						if((*curr)->Frame()
						&& (*curr)->XPos ()                    - Frame::Move::left <= x
						&& (*curr)->YPos ()                                        <= y
						&& (*curr)->XPos ()                                        >  x
						&& (*curr)->YPos () + (*curr)->YSize()                     >  y
						)
						{
							action = MOVE;
							break;
						}

						if((*curr)->Frame()
						&& (*curr)->XPos ()                    - Frame::Move::left - Frame::Resize::left <= x
						&& (*curr)->YPos ()                                                              <= y
						&& (*curr)->XPos ()                    - Frame::Move::left                       >  x
						&& (*curr)->YPos () + (*curr)->YSize()                                           >  y
						)
						{
							action = RESIZE;
							side = LEFT;
							break;
						}

						// Right frame
						if((*curr)->Frame()
						&& (*curr)->XPos () + (*curr)->XSize()                      <= x
						&& (*curr)->YPos ()                                         <= y
						&& (*curr)->XPos () + (*curr)->XSize() + Frame::Move::right >  x
						&& (*curr)->YPos () + (*curr)->YSize()                      >  y
						)
						{
							action = MOVE;
							break;
						}

						if((*curr)->Frame()
						&& (*curr)->XPos () + (*curr)->XSize() + Frame::Move::right                        <= x
						&& (*curr)->YPos ()                                                                <= y
						&& (*curr)->XPos () + (*curr)->XSize() + Frame::Move::right + Frame::Resize::right >  x
						&& (*curr)->YPos () + (*curr)->YSize()                                             >  y
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
								(wl_resource*)Client::Get::Surface(hoveredOver)
							);
							Wayland::Pointer::Frame(
								(wl_client*)hoveredOver->Client()
							);
						}

						if (curr != windowList.end())
						{
							int localX = x - (*curr)->XPos() + (*curr)->XOffset();
							int localY = y - (*curr)->YPos() + (*curr)->YOffset();

							Wayland::Pointer::Enter(
								(wl_client  *)(*curr)->Client(), 
								(wl_resource*)Client::Get::Surface(*curr),
								localX, localY, x, y
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
						int localX = x - hoveredOver->XPos() + hoveredOver->XOffset();
						int localY = y - hoveredOver->YPos() + hoveredOver->YOffset();

						Wayland::Pointer::Moved(
							(wl_client*)hoveredOver->Client(),
							localX, localY, x, y
						);
						Wayland::Pointer::Frame(
							(wl_client*)hoveredOver->Client()
						);
					}
					else if (hoveredOver && action == MOVE && input == LOCK)
					{
						int newX = hoveredOver->XPos() + (x - preX);
						int newY = hoveredOver->YPos() + (y - preY);
						Manager::Window::Reposition(hoveredOver, newX, newY);
						Wayland::Pointer::Moved(nullptr, newX, newY, x, y);
					}
					else if (hoveredOver && action == RESIZE && input == LOCK)
					{
						int deltaX = (x - preX);
						int deltaY = (y - preY);
						int XSize = hoveredOver->XSize();
						int YSize = hoveredOver->YSize();
						int XPos = hoveredOver->XPos();
						int YPos = hoveredOver->YPos();

						switch (side)
						{
						case TOP:
							YPos += deltaY;
							YSize -= deltaY;
							break;
						case BOTTOM:
							YSize += deltaY;
							break;
						case LEFT:
							XPos += deltaX;
							XSize -= deltaX;
							break;	
						case RIGHT:
							XSize += deltaX;
							break;
						}

						Manager::Window::Reposition(hoveredOver, XPos , YPos );
						Manager::Window::Resize    (hoveredOver, XSize, YSize);

						Wayland::Pointer::Moved(nullptr, XPos, YPos, x, y);
					}

					preX = x;
					preY = y;

					//if (hoveredOver)
					//{
					//	void* id = hoveredOver->Client();
					//	wl_resource* wm =  (wl_resource*)Client::WM(id);
					//	xdg_wm_base_send_ping(wm, 1);
					//}
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
								(wl_resource*)Client::Get::Surface(hoveredOver)
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
							Wayland::Pointer::Leave(
								(wl_client  *)hoveredOver->Client(), 
								(wl_resource*)Client::Get::Surface(hoveredOver)
							);

							hoveredOver = nullptr;
							input = UNLOCK;
							action = APPLCATION;
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

		void Resize(Awning::WM::Window* window, int xSize, int ySize)
		{
			if (xSize < window->minSize.x)
				xSize = window->minSize.x;
			if (ySize < window->minSize.x)
				ySize = window->minSize.x;
			if (xSize > window->maxSize.x)
				xSize = window->maxSize.x;
			if (ySize > window->maxSize.x)
				ySize = window->maxSize.x;
			
			if (window->Resized)
				window->Resized(window->data, xSize, ySize);

			window->ConfigSize(xSize, ySize);
		}

		void Reposition(Awning::WM::Window* window, int xPos, int yPos)
		{
			window->ConfigPos(xPos, yPos);
		}

		std::list<Awning::WM::Window*> Get()
		{
			return windowList;
		}
	}
}