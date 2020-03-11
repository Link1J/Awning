#pragma once

#include <wayland-server.h>

#include <unordered_map>

#include "wm/window.hpp"

namespace Awning::Protocols::WL::Pointer
{
	struct Data 
	{
		struct Instance 
		{
			wl_client* client;
			bool inUse = false;
			int version = 0;
			void* seat;
		};

		std::unordered_map<wl_resource*,Instance> instances;
		wl_resource* inUse;
	};

	extern const struct wl_pointer_interface interface;
	extern Data data;

	namespace Interface
	{
		void Set_Cursor(struct wl_client* client, struct wl_resource* resource, uint32_t serial, struct wl_resource* surface, int32_t hotspot_x, int32_t hotspot_y);
		void Release(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, void* seat);
	void Destroy(struct wl_resource* resource);

	void Enter (void* data, void* object, int x, int y );
	void Leave (void* data, void* object               );
	void Moved (void* data, int x, int y               );
	void Button(void* data, uint32_t button, bool state);
	void Axis  (void* data, uint32_t axis, int step    );
	void Frame (void* data                             );
}