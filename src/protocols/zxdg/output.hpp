#pragma once

#include "protocols/handler/xdg-shell.h"
#include "protocols/handler/xdg-output.h"
#include <unordered_map>
#include <string>

#include "wm/output.hpp"

namespace Awning::Protocols::ZXDG::Output
{
	struct Data 
	{
		struct Instance 
		{
			wl_resource* output;
			Awning::Output::ID id;
		};

		std::unordered_map<wl_resource*, Instance> instances;
	};

	extern const struct zxdg_output_v1_interface interface;
	extern Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* output);
	void Destroy(struct wl_resource* resource);
}

namespace Awning::Protocols::ZXDG::Output_Manager
{
	struct Data 
	{
	};

	extern const struct zxdg_output_manager_v1_interface interface;
	extern Data data;
	
	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource);
		void Get_Xdg_Output(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* output);
	}

	wl_global* Add (struct wl_display* display, void* data = nullptr                      );
	void       Bind(struct wl_client* wl_client, void* data, uint32_t version, uint32_t id);
}