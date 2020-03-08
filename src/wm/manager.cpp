#include "manager.hpp"

#include <list>

#include "window.hpp"
#include "client.hpp"
#include <spdlog/spdlog.h>

#include "protocols/wl/pointer.hpp"
#include "protocols/wl/keyboard.hpp"

#include <linux/input.h>
#include "protocols/handler/xdg-shell.h"

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
				
				void Scroll(int axis, float step)
				{
					if (Window::Manager::hoveredOver && action == APPLCATION)
					{
						Protocols::WL::Pointer::Axis(
							(wl_client*)Window::Manager::hoveredOver->Client(),
							axis, step
						);
						Protocols::WL::Pointer::Frame(
							(wl_client*)Window::Manager::hoveredOver->Client()
						);
					}
				}

				void Moved(int x, int y)
				{
					auto curr = Window::Manager::windowList.begin();
					while (curr != Window::Manager::windowList.end() && input == UNLOCK)
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

					if (Window::Manager::hoveredOver != *curr && input == UNLOCK && action != APPLCATION)
					{
						if (Window::Manager::hoveredOver)
						{
							Protocols::WL::Pointer::Leave(
								(wl_client  *)Window::Manager::hoveredOver->Client(), 
								(wl_resource*)Client::Get::Surface(Window::Manager::hoveredOver)
							);
							Protocols::WL::Pointer::Frame(
								(wl_client*)Window::Manager::hoveredOver->Client()
							);

							Window::Manager::hoveredOver = nullptr;
						}
					}
					else if (Window::Manager::hoveredOver != *curr && input == UNLOCK && action == APPLCATION)
					{
						if (curr != Window::Manager::windowList.end())
						{
							int localX = x - (*curr)->XPos() - (*curr)->XOffset();
							int localY = y - (*curr)->YPos() - (*curr)->YOffset();

							Protocols::WL::Pointer::Enter(
								(wl_client  *)(*curr)->Client(), 
								(wl_resource*)Client::Get::Surface(*curr),
								localX, localY, x, y
							);
							Protocols::WL::Pointer::Frame(
								(wl_client*)(*curr)->Client()
							);

							Window::Manager::hoveredOver = *curr;
						}
					}
					else if (Window::Manager::hoveredOver && action == APPLCATION)
					{
						int localX = x - Window::Manager::hoveredOver->XPos() + Window::Manager::hoveredOver->XOffset();
						int localY = y - Window::Manager::hoveredOver->YPos() + Window::Manager::hoveredOver->YOffset();

						Protocols::WL::Pointer::Moved(
							(wl_client*)Window::Manager::hoveredOver->Client(),
							localX, localY, x, y
						);
						Protocols::WL::Pointer::Frame(
							(wl_client*)Window::Manager::hoveredOver->Client()
						);
					}
					else if (Window::Manager::hoveredOver && action == MOVE && input == LOCK)
					{
						int newX = Window::Manager::hoveredOver->XPos() + (x - preX);
						int newY = Window::Manager::hoveredOver->YPos() + (y - preY);
						Window::Manager::Move(Window::Manager::hoveredOver, newX, newY);
						Protocols::WL::Pointer::Moved(nullptr, x, y, x, y);
					}
					else if (Window::Manager::hoveredOver && action == RESIZE && input == LOCK)
					{
						int deltaX = (x - preX);
						int deltaY = (y - preY);
						int XSize = Window::Manager::hoveredOver->XSize();
						int YSize = Window::Manager::hoveredOver->YSize();
						int XPos = Window::Manager::hoveredOver->XPos();
						int YPos = Window::Manager::hoveredOver->YPos();

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

						int preX = Window::Manager::hoveredOver->XSize();
						int preY = Window::Manager::hoveredOver->YSize();

						Window::Manager::Resize(Window::Manager::hoveredOver, XSize, YSize);

						if (preX != Window::Manager::hoveredOver->XSize())
							Window::Manager::Move(Window::Manager::hoveredOver, XPos, Window::Manager::hoveredOver->YPos());
						if (preY != Window::Manager::hoveredOver->YSize())
							Window::Manager::Move(Window::Manager::hoveredOver, Window::Manager::hoveredOver->XPos(), YPos);

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

					if (Window::Manager::hoveredOver != Window::Manager::windowList.front())
					{
						if (Window::Manager::hoveredOver)
							Window::Manager::Raise(Window::Manager::hoveredOver);
						return;
					}
					else if (Window::Manager::hoveredOver && action == APPLCATION)
					{
						Protocols::WL::Pointer::Button(
							(wl_client*)Window::Manager::hoveredOver->Client(),
							button, true
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (Window::Manager::hoveredOver)
						{
							Window::Manager::Raise(Window::Manager::hoveredOver);
							Protocols::WL::Pointer::Leave(
								(wl_client  *)Window::Manager::hoveredOver->Client(), 
								(wl_resource*)Client::Get::Surface(Window::Manager::hoveredOver)
							);
						}
						input = LOCK;
					}
				}

				void Released(uint32_t button)
				{
					if (Window::Manager::hoveredOver && action == APPLCATION)
					{
						Protocols::WL::Pointer::Button(
							(wl_client*)Window::Manager::hoveredOver->Client(),
							button, false
						);
					}
					else if (action == MOVE || action == RESIZE)
					{
						if (button == lockButton)
						{
							if (Window::Manager::hoveredOver)
							{
								Protocols::WL::Pointer::Leave(
									(wl_client  *)Window::Manager::hoveredOver->Client(), 
									(wl_resource*)Client::Get::Surface(Window::Manager::hoveredOver)
								);
							}

							Window::Manager::hoveredOver = nullptr;
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
					auto window = Window::Manager::windowList.front();
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
					auto window = Window::Manager::windowList.front();
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
}