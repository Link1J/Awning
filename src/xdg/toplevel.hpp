#pragma once

#include "protocols/xdg-shell-protocol.h"
#include <unordered_map>
#include <string>
#include "wm/window.hpp"

namespace Awning::XDG::TopLevel
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* surface;
			wl_resource* parent;
			std::string title;
			std::string appid;
			WM::Window* window;
		};

		std::unordered_map<wl_resource*, Instance> toplevels;
	};

	extern const struct xdg_toplevel_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Parent(struct wl_client* client, struct wl_resource* resource, struct wl_resource* parent);
		void Set_Title(struct wl_client* client, struct wl_resource* resource, const char* title);
		void Set_App_id(struct wl_client* client, struct wl_resource* resource, const char* app_id);
		void Show_Window_Menu(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y);
		void Move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial);
		void Resize(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial, uint32_t edges);
		void Set_Max_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height);
		void Set_Min_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height);
		void Set_Maximized(struct wl_client* client, struct wl_resource* resource);
		void Unset_Maximized(struct wl_client* client, struct wl_resource* resource);
		void Set_Fullscreen(struct wl_client* client, struct wl_resource* resource, struct wl_resource* output);
		void Unset_Fullscreen(struct wl_client* client, struct wl_resource* resource);
		void Set_Minimized(struct wl_client* client, struct wl_resource* resource);
	}

	void Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface);
	void Destroy(struct wl_resource* resource);

	void Raised(void* data);
}