#pragma once
#include "protocols/handler/wlr-layer-shell.h"
#include "wm/window.hpp"

#include <unordered_map>
#include <stdint.h>

namespace Awning::Protocols::WLR::Layer_Surface
{
	struct Instance 
	{
		wl_resource* surface;
		wl_resource* parent;
		wl_resource* output;
		std::string title;
		std::string appid;
		std::string name_space;
		Window* window;

		uint32_t anchor;
		bool configuring = false;
		int baseX, baseY;

		int marginsTop, marginsLeft, marginsBottom, marginsRight;
	};

	extern const struct zwlr_layer_surface_v1_interface interface;
	extern std::unordered_map<wl_resource*, Instance> instances;
	
	namespace Interface
	{
		void Set_Size(struct wl_client* client, struct wl_resource* resource, uint32_t width, uint32_t height);
		void Set_Anchor(struct wl_client* client, struct wl_resource* resource, uint32_t anchor);
		void Set_Exclusive_Zone(struct wl_client* client, struct wl_resource* resource, int32_t zone);
		void Set_Margin(struct wl_client* client, struct wl_resource* resource, int32_t top, int32_t right, int32_t bottom, int32_t left);
		void Set_Keyboard_Interactivity(struct wl_client* client, struct wl_resource* resource, uint32_t keyboard_interactivity);
		void Get_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* popup);
		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial);
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Set_Layer(struct wl_client* client, struct wl_resource* resource, uint32_t layer);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* output, uint32_t layer); 
	void Destroy(struct wl_resource* resource);

	void Resized(void* data, int width, int height);
	void ResizedOutput(void* data, int width, int height);

	void ReconfigureWindow(struct wl_resource* resource);
}