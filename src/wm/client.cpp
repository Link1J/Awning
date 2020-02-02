#include "client.hpp"
#include "window.hpp"

#include "wayland/surface.hpp"

#include <unordered_map>

namespace Awning::WM::Client
{
	struct Data
	{
		struct Windows
		{
			void* surface = nullptr;
		};
		std::unordered_map<Window*, Windows> windows;
	};

	std::unordered_map<void*,Data> clients;

	void Create(void* id)
	{
		clients[id] = Data();
	}

	void Bind(void* id, Window* window)
	{
		window->client = id;
		clients[id].windows[window] = Data::Windows();
	}

	void Unbind(Window* window)
	{
		clients[window->client].windows.erase(window);
	}

	void* Surface(Window* window)
	{
		void* surface = clients[window->client].windows[window].surface;
		if (!surface)
		{
			for (auto& [resource, data] : Wayland::Surface::data.surfaces)
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
}