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
			int xPos, yPos, xLPos, yLPos;
		};

		std::unordered_map<wl_client*,Interface> pointers;
		wl_resource* pre_shell;
	};

	extern const struct wl_pointer_interface interface;
	extern Data data;

	void Moved(double x, double y);
	void Button(uint32_t button, bool pressed);
}

namespace Awning::Wayland::Seat
{
	struct Data 
	{
		wl_global* global;
	};

	extern const struct wl_seat_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Get_Pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Get_Touch(struct wl_client* client, struct wl_resource* resource, uint32_t id);
		void Release(struct wl_client* client, struct wl_resource* resource);
	}

	void Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}