#include "layer_surface.hpp"
#include "log.hpp"
#include "protocols/wl/surface.hpp"
#include "protocols/wl/output.hpp"
#include "wm/output.hpp"

extern uint32_t NextSerialNum();

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

	Data data;
	
	namespace Interface
	{
		void Set_Size(struct wl_client* client, struct wl_resource* resource, uint32_t width, uint32_t height)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
			WM::Window::Manager::Resize(data.instances[resource].window, width, height);
		}

		void Set_Anchor(struct wl_client* client, struct wl_resource* resource, uint32_t anchor)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");

			auto output_resource = data.instances[resource].output;
			auto output = WL::Output::data.resource_to_outputId[output_resource];
			auto [px, py] = WM::Output::Get::Position(output);
			auto [sx, sy] = WM::Output::Get::Mode::Resolution(output, WM::Output::Get::CurrentMode(output));

			WM::Window::Manager::Move  (data.instances[resource].window, px, py);
			WM::Window::Manager::Resize(data.instances[resource].window, sx, sy);
		}

		void Set_Exclusive_Zone(struct wl_client* client, struct wl_resource* resource, int32_t zone)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}

		void Set_Margin(struct wl_client* client, struct wl_resource* resource, int32_t top, int32_t right, int32_t bottom, int32_t left)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}

		void Set_Keyboard_Interactivity(struct wl_client* client, struct wl_resource* resource, uint32_t keyboard_interactivity)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}

		void Get_Popup(struct wl_client* client, struct wl_resource* resource, struct wl_resource* popup)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}

		void Ack_Configure(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}

		void Destroy(struct wl_client* client, struct wl_resource* resource)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
			Layer_Surface::Destroy(resource);
		}

		void Set_Layer(struct wl_client* client, struct wl_resource* resource, uint32_t layer)
		{
			Log::Function::Called("Protocols::WLR::Layer_Surface::Interface");
		}
	}

	wl_resource* Create(struct wl_client* wl_client, uint32_t version, uint32_t id, wl_resource* surface, wl_resource* output, uint32_t layer) 
	{
		Log::Function::Called("Protocols::WLR::Layer_Surface");

		struct wl_resource* resource = wl_resource_create(wl_client, &zwlr_layer_surface_v1_interface, version, id);
		if (resource == nullptr) {
			wl_client_post_no_memory(wl_client);
			return resource;
		}
		wl_resource_set_implementation(resource, &interface, nullptr, Destroy);

		data.instances[resource] = Data::Instance();
		data.instances[resource].surface = surface;
		data.instances[resource].output  = output ;
		data.instances[resource].window = WM::Window::Create(wl_client);

		WM::Window::Manager::Manage(data.instances[resource].window, (WM::Window::Manager::Layer)layer);
		WM::Window::Manager::Move  (data.instances[resource].window, 0, 0                             );

		WL::Surface::data.surfaces[surface].window = data.instances[resource].window;
		WL::Surface::data.surfaces[surface].type = 0;

		data.instances[resource].window->Data      (resource);
		data.instances[resource].window->SetResized(Resized );

		return resource;
	}

	void Destroy(struct wl_resource* resource)
	{
		Log::Function::Called("Protocols::WLR::Layer_Surface");

		if (!data.instances.contains(resource))
			return;

		auto surface = data.instances[resource].surface; 

		WL::Surface::data.surfaces[surface].window = nullptr;

		data.instances[resource].window->Mapped(false);
		data.instances[resource].window->Texture(nullptr);
		WM::Window::Destory(data.instances[resource].window);
		data.instances.erase(resource);
	}

	void Resized(void* data, int width, int height)
	{
		Log::Function::Called("Protocols::XDG::TopLevel");
		
		struct wl_resource* resource = (struct wl_resource*)data;
		zwlr_layer_surface_v1_send_configure(resource, NextSerialNum(), width, height);
	}
}