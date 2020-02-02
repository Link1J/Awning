#include "manager.hpp"

#include <list>

#include "../window.hpp"

static std::list<Awning::WM::Window*> windowList;

namespace Awning::WM::Manager
{
	namespace Handle
	{
		namespace Input
		{
			namespace Mouse
			{
				void Scroll(int axis, bool direction, float step)
				{
				}

				void Moved(int x, int y)
				{
				}

				void Pressed(uint32_t button)
				{
				}

				void Released(uint32_t button)
				{
				}
			}

			namespace Keyboard
			{
				void Pressed(uint32_t key)
				{
				}

				void Released(uint32_t key)
				{
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
			}

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