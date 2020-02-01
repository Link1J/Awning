#include "manager.hpp"

#include <list>

static std::list<Awning::WM::Window::Data*> windowList;

namespace Awning::WM::Window
{
	Data* Create()
	{
		Data* newWindow = new Data();
		windowList.emplace_back(newWindow);
		return newWindow;
	}

	void Destory(Data*& window)
	{
		auto curr = windowList.begin();
		while (curr != windowList.end())
		{
			if (*curr == window)
				break;
		}

		delete window;
		window = nullptr;
		windowList.erase(curr);
	}

	void SetTexture(Data* window, Texture::Data texture)
	{
		window->texture = texture;
	}

	void Map(Data* window, bool map)
	{
		window->mapped = map;
	}

	void Raise(Data* window)
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
}

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
}