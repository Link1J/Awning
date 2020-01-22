#pragma once

#include <wayland-server.h>
#include <unordered_map>

namespace Awning::Wayland::Shell_Surface
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface;
			double xPosition, yPosition;
		};

		std::unordered_map<wl_resource*, Instance> shells;
	};

	extern const struct wl_shell_surface_interface interface;
	extern Data data;

	namespace Interface
	{
		void Pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial);
		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial);
		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges);
		void Set_Toplevel(struct wl_client* client, struct wl_resource* resource);
		void Set_Transient(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags);
		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, uint32_t method, uint32_t framerate, struct wl_resource* output);
		void Set_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags);
		void Set_Maximized(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output);
		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title);
		void Set_Class(struct wl_client* client, struct wl_resource* resource, const char* class_);
	}
	
	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, struct wl_resource* surface);
	void Destroy(struct wl_resource* resource);
}