#include "client.hpp"
#include "window.hpp"

#include "protocols/wl/surface.hpp"

#include <unordered_map>
#include <algorithm>

namespace Awning::Client
{
	struct Data
	{
		struct Windows
		{
			void* surface = nullptr;
		};
		std::unordered_map<Window*, Windows> windows;
		std::unordered_set<void*> pointers;
		std::unordered_set<void*> keyboards;
		void* wm_base;
	};

	std::unordered_map<void*, Data> clients;

	void Create(void* id)
	{
		clients[id] = Data();
	}

	void Bind::Window(void* id, Awning::Window* window)
	{
		window->client = id;
		clients[id].windows[window] = Data::Windows();
	}

	void Bind::Pointer(void* id, void* pointer)
	{
		clients[id].pointers.emplace(pointer);
	}

	void Bind::Keyboard(void* id, void* keyboard)
	{
		clients[id].keyboards.emplace(keyboard);
	}

	void Unbind::Window(Awning::Window* window)
	{
		clients[window->client].windows.erase(window);
	}

	void Unbind::Pointer(void* id, void* pointer)
	{
		clients[id].pointers.erase(pointer);
	}

	void Unbind::Keyboard(void* id, void* keyboard)
	{
		clients[id].keyboards.erase(keyboard);
	}

	void SetWM(void* id, void* wm)
	{
		clients[id].wm_base = wm;
	}

	void* Get::Surface(Window* window)
	{
		void* surface = clients[window->client].windows[window].surface;
		if (!surface)
		{
			for (auto& [resource, data] : Protocols::WL::Surface::data.surfaces)
			{
				if (data.window == window)
				{
					surface = resource;
					clients[window->client].windows[window].surface = resource;
					break;
				}
			}
		}
		return surface;
	}

	void* Get::WM(void* id)
	{
		return clients[id].wm_base;
	}

	std::vector<void*> Get::All::Client()
	{
		std::vector<void*> keys(clients.size());
		std::transform(clients.begin(), clients.end(), keys.begin(), [](auto pair){return pair.first;});
		return keys;
	}

	std::unordered_set<void*> Get::All::Pointers(void* id)
	{
		return clients[id].pointers;
	}

	std::unordered_set<void*> Get::All::Keyboards(void* id)
	{
		return clients[id].keyboards;
	}

	std::vector<Window*> Get::All::Windows(void* id)
	{
		std::vector<Window*> keys(clients[id].windows.size());
		std::transform(clients[id].windows.begin(), clients[id].windows.end(), keys.begin(), [](auto pair){return pair.first;});
		return keys;
	}

	void Bind::Surface(Awning::Window* window, void* pointer)
	{
		clients[window->client].windows[window].surface = pointer;
	}
}