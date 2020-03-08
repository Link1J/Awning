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
					if (input == UNLOCK)
					{
						auto curr = Window::Manager::windowList.begin();
						bool found = false;

						side = NONE;

						while (curr != Window::Manager::windowList.end() && !found)
						{
							if ((*curr)->XPos()                    < x 
							&&  (*curr)->YPos()                    < y
							&&  (*curr)->XPos() + (*curr)->XSize() > x
							&&  (*curr)->YPos() + (*curr)->YSize() > y
							)
							{
								action = APPLCATION;
								found = true;
							}

							if ((*curr)->Frame())
							{
								if (x > (*curr)->XPos()                    - Frame::Move::left  
								&&  y > (*curr)->YPos()                    - Frame::Move::top   
								&&  x < (*curr)->XPos() + (*curr)->XSize() + Frame::Move::right 
								&&  y < (*curr)->YPos() + (*curr)->YSize() + Frame::Move::bottom
								&& !found)
								{
									action = MOVE;
									found = true;
								}

								if (x > (*curr)->XPos()                    - Frame::Move::left   - Frame::Resize::left  
								&&  y > (*curr)->YPos()                    - Frame::Move::top    - Frame::Resize::top   
								&&  x < (*curr)->XPos() + (*curr)->XSize() + Frame::Move::right  + Frame::Resize::right 
								&&  y < (*curr)->YPos() + (*curr)->YSize() + Frame::Move::bottom + Frame::Resize::bottom
								&& !found)
								{
									action = RESIZE;
									found = true;
								}

								if (action == RESIZE && found)
								{
									if ((*curr)->XPos()                    > x) side = (WindowSide)(side | LEFT  );
									if ((*curr)->XPos() + (*curr)->XSize() < x) side = (WindowSide)(side | RIGHT );
									if ((*curr)->YPos()                    > y) side = (WindowSide)(side | TOP   );
									if ((*curr)->YPos() + (*curr)->YSize() < y) side = (WindowSide)(side | BOTTOM);
								}
							}

							if (!found)
								curr++;
						}

						if (Window::Manager::hoveredOver != *curr)
						{
							if (action != APPLCATION)
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
								}
							}
							else if (action == APPLCATION && found)
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
					}
					else if (Window::Manager::hoveredOver)
					{
						if (action == MOVE)
						{
							int newX = Window::Manager::hoveredOver->XPos() + (x - preX);
							int newY = Window::Manager::hoveredOver->YPos() + (y - preY);
							Window::Manager::Move(Window::Manager::hoveredOver, newX, newY);
							Protocols::WL::Pointer::Moved(nullptr, x, y, x, y);
						}
						else if (action == RESIZE)
						{
							int deltaX = (x - preX);
							int deltaY = (y - preY);
							int XSize = Window::Manager::hoveredOver->XSize();
							int YSize = Window::Manager::hoveredOver->YSize();
							int XPos = Window::Manager::hoveredOver->XPos();
							int YPos = Window::Manager::hoveredOver->YPos();

							if ((side & TOP   ) != 0)
							{
								YPos += deltaY;
								YSize -= deltaY;
							}
							if ((side & BOTTOM) != 0)
							{
								YSize += deltaY;
							}
							if ((side & LEFT  ) != 0)
							{
								XPos += deltaX;
								XSize -= deltaX;
							}
							if ((side & RIGHT ) != 0)
							{
								XSize += deltaX;
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
					}
					
					if (Window::Manager::hoveredOver && action == APPLCATION)
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