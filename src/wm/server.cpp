#include "server.hpp"
#include <spdlog/spdlog.h>
#include "client.hpp"

namespace Awning::Server
{
	wl_display        * display;
	wl_event_loop     * event_loop;
	wl_protocol_logger* logger; 
	wl_listener         client_listener;
	std::string         socketname;

	void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
	void ClientCreated(struct wl_listener *listener, void *data);

	void Init()
	{
		display = wl_display_create(); 
		socketname = wl_display_add_socket_auto(display);
		client_listener.notify = ClientCreated;
		event_loop = wl_display_get_event_loop(display);

		wl_display_add_client_created_listener(display, &client_listener);
		wl_display_add_protocol_logger(display, ProtocolLogger, nullptr);

		spdlog::info("Wayland Socket: {}", socketname);
	}

	void ClientCreated(struct wl_listener* listener, void* data)
	{
		Awning::Client::Create(data);
	}

	void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
	{
		const char* direction_strings[] = { 
			"REQUEST", 
			"EVENT  "
		};
	
		//spdlog::debug("[{}] {}: {}", direction_strings[direction], message->resource->object.interface->name, message->message->name);
	}
};
