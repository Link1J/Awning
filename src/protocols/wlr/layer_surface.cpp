#include "layer_surface.hpp"
#include <spdlog/spdlog.h>
#include "protocols/wl/surface.hpp"
#include "protocols/wl/output.hpp"
#include "wm/output.hpp"

extern uint32_t NextSerialNum();

struct Anchor
{
	enum {
		Top    = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP   ,
		Left   = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT  ,
		Bottom = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
		Right  = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT ,
	};
};

namespace Awning::Protocols::WLR::Layer_Surface
{
	const struct zwlr_layer_surface_v1_interface interface = {
		.set_size                   = Interface::Set_Size                  ,
		.set_anchor                 = Interface::Set_Anchor                ,
		.set_exclusive_zone         = Interface::Set_Exclusive_Zone        ,
		.set_margin                 = Interface::Set_Margin                ,
		.set_keyboard_interactivity = Interface::Set_Keyboard_Interactivity,
		.get_popup                  = Interface::Get_Popup                 ,
		.ack_configure              = Interface::Ack_Configure             ,
		.destroy                    = Interface::Destroy                   ,
		.set_layer                  = Interface::Set_Layer                 ,
	};

	std::unordered_map<wl_resource*, Instance> instances;
	
	namespace Interface
	{
		void Set_Size(struct wl_client* client, struct wl_resource* resource, uint32_t width, uint32_t height)
		{
			instances[resource].baseX = width ;
			instances[resource].baseY = height;

			ReconfigureWindow(resource);
		}

		void Set_Anchor(struct wl_client* client, struct wl_resource* resource, uint32_t anchor)
		{
			instances[resource].anchor = anchor;

			ReconfigureWindow(resource);
		}

		void Set_Exclusive_Zone(struct wl_client* client, struct wl_resource* resource, int32_t zone)
		{
		}

		void Set_Margin(struct wl_client* client, struct wl_resource* resource, int32_t top, int32_t right, int32_t bottom, int32_t left)
		{
			instances[resource].marginsTop    = top   ;
			instances[resource].marginsLeft   = left  ;
			instances[resource].marginsBottom = bottom;
			instances[resource].marginsRight  = right ;

			ReconfigureWindow(resource);
		}

		void Set_Keyboard_Interactivity(struct wl_client* client, struct wl_resource* resource, uint32_t keyboard_interactivity)
		{
		}

		void Get_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* popup)
		{
		}

		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Layer_Surface::Destroy(resource);
		}

		void Set_Layer(struct wl_client* client, struct wl_resource* resource, uint32_t layer)
		{
			//Window::Manager::Unmanage(instances[resource].window);
			//Window::Manager::Manage(instances[resource].window, (Window::Manager::Layer)layer);
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* output, uint32_t layer) 
	{
		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_layer_surface_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		instances[resource] = Instance();
		instances[resource].surface       = surface;
		instances[resource].output        = output ;
		instances[resource].marginsTop    = 0;
		instances[resource].marginsLeft   = 0;
		instances[resource].marginsBottom = 0;
		instances[resource].marginsRight  = 0;
		instances[resource].window        = Window::Create(wl_client);

		Window::Manager::Manage(instances[resource].window, (Window::Manager::Layer)layer);
		Window::Manager::Move  (instances[resource].window, 0, 0                         );

		WL::Surface::instances[surface].window = instances[resource].window;
		WL::Surface::instances[surface].type   = 0;

		instances[resource].window->Data         (resource);
		instances[resource].window->SetResized   (Resized );
		instances[resource].window->ConfigMinSize(0, 0    );

		auto outputid = WL::Output::resource_to_outputId[output];

		Output::AddResize(outputid, ResizedOutput, resource);

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		auto surface = instances[resource].surface; 

		WL::Surface::instances[surface].window = nullptr;

		auto outputid = WL::Output::resource_to_outputId[instances[resource].output];
		Output::RemoveResize(outputid, ResizedOutput, resource);

		instances[resource].window->Mapped(false);
		instances[resource].window->Texture(nullptr);
		Window::Destory(instances[resource].window);
		instances.erase(resource);
	}

	void Resized(void* data, int width, int height)
	{
		struct wl_resource* resource = (struct wl_resource*)data;
		zwlr_layer_surface_v1_send_configure(resource, NextSerialNum(), width, height);
	}

	void ResizedOutput(void* data, int width, int height)
	{
		ReconfigureWindow((struct wl_resource*)data);
	}

	void ReconfigureWindow(struct wl_resource* resource)
	{
		if (!instances.contains(resource))
			return;

		int check = 0;

		auto output = WL::Output::resource_to_outputId[instances[resource].output];
		auto anchor = instances[resource].anchor;
		auto window = instances[resource].window;

		auto mode = Output::Get::CurrentMode(output);
		
		auto [px, py] = Output::Get::Position        (output      );
		auto [sx, sy] = Output::Get::Mode::Resolution(output, mode);
		auto [rx, ry] = Output::Get::Mode::Resolution(output, mode);

		check = Anchor::Left | Anchor::Right;
		if ((anchor & check) != check) sx = instances[resource].baseX;

		check = Anchor::Top | Anchor::Bottom;
		if ((anchor & check) != check) sy = instances[resource].baseY;

		if ((anchor & Anchor::Left) != 0 && (anchor & Anchor::Right) == 0)
		{
			px += instances[resource].marginsLeft;
		}
		else if ((anchor & Anchor::Left) == 0 && (anchor & Anchor::Right) != 0)
		{
			px += (rx - (sx + instances[resource].marginsRight));
		}
		else if ((anchor & Anchor::Left) != 0 && (anchor & Anchor::Right) != 0)
		{
			px += instances[resource].marginsLeft;
			sx -= (instances[resource].marginsLeft + instances[resource].marginsRight);
		}

		if ((anchor & Anchor::Top) != 0 && (anchor & Anchor::Bottom) == 0)
		{
			py += instances[resource].marginsTop;
		}
		else if ((anchor & Anchor::Top) == 0 && (anchor & Anchor::Bottom) != 0)
		{
			py += (ry - (sy + instances[resource].marginsBottom));
		}
		else if ((anchor & Anchor::Top) != 0 && (anchor & Anchor::Bottom) != 0)
		{
			py += instances[resource].marginsTop;
			sy -= (instances[resource].marginsLeft + instances[resource].marginsBottom);
		}

		if (px != window->XPos() || py != window->YPos() || sx != window->XSize() || sy != window->YSize())
		{
			Window::Manager::Move  (window, px, py);
			Window::Manager::Resize(window, sx, sy);
		}
	}
}