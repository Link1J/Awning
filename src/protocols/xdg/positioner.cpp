#include "positioner.hpp"
#include <spdlog/spdlog.h>

namespace Awning::Protocols::XDG::Positioner
{
	const struct xdg_positioner_interface interface = {
		.destroy                   = Interface::Destroy,
		.set_size                  = Interface::Set_Size,
		.set_anchor_rect           = Interface::Set_Anchor_Rect,
		.set_anchor                = Interface::Set_Anchor,
		.set_gravity               = Interface::Set_Gravity,
		.set_constraint_adjustment = Interface::Set_Constraint_Adjustment,
		.set_offset                = Interface::Set_Offset
	};

	Data data;

	namespace Interface
	{
		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Positioner::Destroy(resource);
		}

		void Set_Size(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height)
		{
			data.instances[resource].width  = width ;
			data.instances[resource].height = height;
		}

		void Set_Anchor_Rect(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
		{
			data.instances[resource].x      = x     ;
			data.instances[resource].y      = y     ;
			data.instances[resource].width  = width ;
			data.instances[resource].height = height;
		}

		void Set_Anchor(struct wl_client* client, struct wl_resource* resource, uint32_t anchor)
		{
		}

		void Set_Gravity(struct wl_client* client, struct wl_resource* resource, uint32_t gravity)
		{
		}

		void Set_Constraint_Adjustment(struct wl_client* client, struct wl_resource* resource, uint32_t constraint_adjustment)
		{
		}

		void Set_Offset(struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y)
		{
			data.instances[resource].x = x;
			data.instances[resource].y = y;
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &xdg_positioner_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.instances[resource] = Data::Instance();

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!data.instances.contains(resource))
			return;

		data.instances.erase(resource);
	}
}