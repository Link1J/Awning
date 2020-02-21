#include "manager.hpp"

#include <list>

#include "window.hpp"
#include "client.hpp"
#include "log.hpp"

#include "protocols/wl/pointer.hpp"
#include "protocols/wl/keyboard.hpp"

#include <linux/input.h>
#include "protocols/handler/xdg-shell-protocol.h"

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
						Protocols::WL::Pointer::Axis(
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
							Protocols::WL::Pointer::Leave(
								(wl_client  *)hoveredOver->Client(), 
								(wl_resource*)Client::Get::Surface(hoveredOver)
							);
							Protocols::WL::Pointer::Frame(
								(wl_client*)hoveredOver->Client()
							);
						}

						if (curr != windowList.end())
						{
							int localX = x - (*curr)->XPos() + (*curr)->XOffset();
							int localY = y - (*curr)->YPos() + (*curr)->YOffset();

							Protocols::WL::Pointer::Enter(
								(wl_client  *)(*curr)->Client(), 
								(wl_resource*)Client::Get::Surface(*curr),
								localX, localY, x, y
							);
							Protocols::WL::Pointer::Frame(
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

						Protocols::WL::Pointer::Moved(
							(wl_client*)hoveredOver->Client(),
							localX, localY, x, y
						);
						Protocols::WL::Pointer::Frame(
							(wl_client*)hoveredOver->Client()
						);
					}
					else if (hoveredOver && action == MOVE && input == LOCK)
					{
						int newX = hoveredOver->XPos() + (x - preX);
						int newY = hoveredOver->YPos() + (y - preY);
						Manager::Window::Reposition(hoveredOver, newX, newY);
						Protocols::WL::Pointer::Moved(nullptr, x, y, x, y);
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
						case TOP_LEFT:
							YPos += deltaY;
							YSize -= deltaY;
							XPos += deltaX;
							XSize -= deltaX;
							break;
						case TOP_RIGHT:
							YPos += deltaY;
							YSize -= deltaY;
							XSize += deltaX;
							break;
						case BOTTOM_LEFT:
							XPos += deltaX;
							XSize -= deltaX;
							YSize += deltaY;
							break;
						case BOTTOM_RIGHT:
							XSize += deltaX;
							YSize += deltaY;
							break;
						}

						int preX = hoveredOver->XSize();
						int preY = hoveredOver->YSize();

						Manager::Window::Resize(hoveredOver, XSize, YSize);

						if (preX != hoveredOver->XSize() || preY != hoveredOver->YSize())
							Manager::Window::Reposition(hoveredOver, XPos, YPos);

						Protocols::WL::Pointer::Moved(nullptr, XPos, YPos, x, y);
					}

					Protocols::WL::Pointer::Moved(nullptr, x, y, x, y);

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
						Protocols::WL::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, true
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (hoveredOver)
						{
							Manager::Window::Raise(hoveredOver);
							Protocols::WL::Pointer::Leave(
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
						Protocols::WL::Pointer::Button(
							(wl_client*)hoveredOver->Client(),
							button, false
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (button == lockButton)
						{
							if (hoveredOver)
							{
								Protocols::WL::Pointer::Leave(
									(wl_client  *)hoveredOver->Client(), 
									(wl_resource*)Client::Get::Surface(hoveredOver)
								);
							}

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
						Protocols::WL::Keyboard::Button(
							(wl_client*)window->Client(),
							key, true
						);
					}
				}

				void Released(uint32_t key)
				{
					auto window = windowList.front();
					if (window)
					{
						Protocols::WL::Keyboard::Button(
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
			auto star = windowList.begin();
			auto curr = windowList.begin();
			while (curr != windowList.end())
			{
				if (*curr == window)
					break;
				curr++;
			}

			if (curr == windowList.end())
				return;

			Protocols::WL::Keyboard::ChangeWindow(
				(wl_client  *)(*star)->Client(), 
				(wl_resource*)Client::Get::Surface(*star),
				(wl_client  *)(*curr)->Client(), 
				(wl_resource*)Client::Get::Surface(*curr)
			);

			windowList.erase(curr);
			windowList.emplace_front(window);

			if (window->Raised)
				window->Raised(window->data);
		}

		void Resize(Awning::WM::Window* window, int xSize, int ySize)
		{
			printf("    Size: %4d,%4d\n", xSize, ySize);
			printf("    Min : %4d,%4d\n", window->minSize.x, window->minSize.y);
			printf("    Max : %4d,%4d\n", window->maxSize.x, window->maxSize.y);

			if (xSize < window->minSize.x)
				xSize = window->minSize.x;
			if (ySize < window->minSize.y)
				ySize = window->minSize.y;
			if (xSize > window->maxSize.x)
				xSize = window->maxSize.x;
			if (ySize > window->maxSize.y)
				ySize = window->maxSize.y;
			
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