#pragma once

#include <wayland-server.h>

#include <unordered_map>

namespace Awning::Wayland::Pointer
{
	struct Data 
	{
		struct Interface 
		{
			wl_resource* resource;
			long long xPos;
			long long yPos; 
			long long xLPos; 
			long long yLPos;
		};

		std::unordered_map<wl_client*,Interface> pointers;
		wl_resource* pre_shell;
		bool moveMode = false;
		int frame;
	};

	extern const struct wl_pointer_interface interface;
	extern Data data;

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id);
	void Moved(double x, double y);
	void Button(uint32_t button, bool pressed);
	void MoveMode();
}
