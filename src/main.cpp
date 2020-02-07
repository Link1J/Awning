#include "backends/manager.hpp"

#include <wayland-server.h>
#include "protocols/xdg-shell-protocol.h"

#include <linux/kd.h>

#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "wayland/compositor.hpp"
#include "wayland/seat.hpp"
#include "wayland/output.hpp"
#include "wayland/shell.hpp"
#include "wayland/surface.hpp"
#include "wayland/shell_surface.hpp"
#include "wayland/pointer.hpp"

#include "xdg/wm_base.hpp"
#include "xdg/decoration.hpp"

#include "wm/x/wm.hpp"
#include "wm/window.hpp"
#include "wm/client.hpp"
#include "wm/manager.hpp"

#include "log.hpp"

#include <iostream>
#include <unordered_map>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
int XWM_Start(int signal_number, void *data);
void LaunchXwayland(int signal_number);
void on_term_signal(int signal_number);
void client_created(struct wl_listener *listener, void *data);

// -------------------------------------------------------------------
// --- Reversed iterable

template <typename T>
struct reversion_wrapper { T& iterable; };
template <typename T>
auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }
template <typename T>
auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }
template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_event_source* sigusr1;
			wl_protocol_logger* logger; 
			wl_listener client_listener;
		};
	}	
	Server::Data server;
};

struct something { int x; int y; };
extern something cursor;
extern int tty_fd;

uint32_t lastSerialNum = 1;

uint32_t NextSerialNum()
{
	lastSerialNum++;
	return lastSerialNum;
}

int main(int argc, char* argv[])
{
	bool noX = false;
	Awning::Backend::API api_output = Awning::Backend::API::FBDEV;
	Awning::Backend::API api_input  = Awning::Backend::API::EVDEV;

	for(int a = 0; a < argc; a++)
	{
		auto arg = std::string(argv[a]);
		if (arg == "-noX")
		{
			noX = true;
		}
		if (arg == "-fbdev")
		{
			api_output = Awning::Backend::API::FBDEV;
			api_input  = Awning::Backend::API::EVDEV;
		}
		if (arg == "-x11")
		{
			api_output = Awning::Backend::API::X11;
			api_input  = Awning::Backend::API::X11;
		}
	}

    int pid;
    int status, ret;

	if (!noX)
	{
		pid = fork();
	}

	if (!noX && pid == 0) 
	{
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, LaunchXwayland);
		pause();
   		exit(-1);
	}

	Awning::server.display = wl_display_create(); 
	const char* socket = wl_display_add_socket_auto(Awning::server.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	Awning::Backend::Init(api_output, api_input);

	Awning::server.client_listener.notify = client_created;

	Awning::server.event_loop = wl_display_get_event_loop(Awning::server.display);
	wl_display_add_protocol_logger(Awning::server.display, ProtocolLogger, nullptr);
	wl_display_add_client_created_listener(Awning::server.display, &Awning::server.client_listener);

	Awning::server.sigusr1 = wl_event_loop_add_signal(Awning::server.event_loop, SIGUSR1, XWM_Start, nullptr);

	{
		using namespace Awning;
		using namespace Awning::Wayland;

		Compositor::data.global = wl_global_create(server.display, &wl_compositor_interface, 3, nullptr, Compositor::Bind);
		Seat      ::data.global = wl_global_create(server.display, &wl_seat_interface      , 4, nullptr, Seat      ::Bind);
		Output    ::data.global = wl_global_create(server.display, &wl_output_interface    , 3, nullptr, Output    ::Bind);
		Shell     ::data.global = wl_global_create(server.display, &wl_shell_interface     , 1, nullptr, Shell     ::Bind);

		wl_display_init_shm(server.display);
	}

	{
		using namespace Awning;
		using namespace Awning::XDG;
		using namespace Awning::ZXDG;

		WM_Base           ::data.global = wl_global_create(server.display, &xdg_wm_base_interface               , 1, nullptr, WM_Base           ::Bind);
		//Decoration_Manager::data.global = wl_global_create(server.display, &zxdg_decoration_manager_v1_interface, 1, nullptr, Decoration_Manager::Bind);
	}

	if (pid != 0)
	{
		kill(pid, SIGUSR2);
	}
	
	while(1)
	{
		Awning::Backend::Poll();
		Awning::Backend::Hand();

		wl_event_loop_dispatch(Awning::server.event_loop, 0);
		wl_display_flush_clients(Awning::server.display);

		Awning::WM::X::EventLoop();

		auto data = Awning::Backend::Data();
		auto list = Awning::WM::Manager::Window::Get();
		void* client = 0;

		memset(data.buffer.u8, 0xEE, data.size);

		for (auto& window : reverse(list))
		{
			auto texture = window->Texture();

			if (!window->Mapped())
				continue;
			if (!texture)
				continue;

			auto winPosX  = window->XPos   ();
			auto winPosY  = window->YPos   ();
			auto winSizeX = (int)texture->width ; //window->XSize  ();
			auto winSizeY = (int)texture->height; //window->YSize  ();
			auto winOffX  = window->XOffset();
			auto winOffY  = window->YOffset();


			for (int x = -Frame::Size::left; x < winSizeX + Frame::Size::right; x++)
				for (int y = -Frame::Size::top; y < winSizeY + Frame::Size::bottom; y++)
				{
					if ((winPosX + x - winOffX) <  0          )
						continue;
					if ((winPosY + y - winOffY) <  0          )
						continue;
					if ((winPosX + x - winOffX) >= data.width )
						continue;
					if ((winPosY + y - winOffY) >= data.height)
						continue;

					int windowOffset = (x) * (texture->bitsPerPixel / 8)
									 + (y) *  texture->bytesPerLine    ;

					int framebOffset = (winPosX + x - winOffX) * (data.bitsPerPixel / 8)
									 + (winPosY + y - winOffY) *  data.bytesPerLine    ;

					uint8_t red, green, blue, alpha;

					if (x < winSizeX && y < winSizeY && x >= 0 && y >= 0)
					{
						if (texture->buffer.u8 != nullptr)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
					}
					else
					{
						if (window->Frame())
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0xFF;
							alpha = 0xFF;
						}
						else 
						{
							red   = 0x00;
							green = 0x00;
							blue  = 0x00;
							alpha = 0x00;
						}
					}

					if (alpha > 0)
					{
						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
				}
			client = window->Client();
		}

		if (Awning::Wayland::Pointer::data.window)
		{
			auto texture = Awning::Wayland::Pointer::data.window->Texture();

			if (texture)
			{
				auto winSizeX = texture->width  ? texture->width  : 3;
				auto winSizeY = texture->height ? texture->height : 3;

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((cursor.x + x) <  0          )
							continue;
						if ((cursor.y + y) <  0          )
							continue;
						if ((cursor.x + x) >= data.width )
							continue;
						if ((cursor.y + y) >= data.height)
							continue;

						int windowOffset = (x) * (texture->bitsPerPixel / 8)
										 + (y) *  texture->bytesPerLine    ;

						int framebOffset = (cursor.x + x) * (data.bitsPerPixel / 8)
										 + (cursor.y + y) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						if (texture->buffer.u8 != nullptr)
						{
							red   = texture->buffer.u8[windowOffset + (texture->red  .offset / 8)];
							green = texture->buffer.u8[windowOffset + (texture->green.offset / 8)];
							blue  = texture->buffer.u8[windowOffset + (texture->blue .offset / 8)];
							alpha = texture->buffer.u8[windowOffset + (texture->alpha.offset / 8)];
						}
						else
						{
							red   = 0x00;
							green = 0xFF;
							blue  = 0x00;
							alpha = 0xFF;
						}

						if (alpha > 0)
						{
							uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
							uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
							uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

							buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
							buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
							buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
						}
					}
			}
			else
			{
				auto winSizeX = 3;
				auto winSizeY = 3;

				for (int x = 0; x < winSizeX; x++)
					for (int y = 0; y < winSizeY; y++)
					{
						if ((cursor.x + x) <  0          )
							continue;
						if ((cursor.y + y) <  0          )
							continue;
						if ((cursor.x + x) >= data.width )
							continue;
						if ((cursor.y + y) >= data.height)
							continue;

						int framebOffset = (cursor.x + x) * (data.bitsPerPixel / 8)
										 + (cursor.y + y) *  data.bytesPerLine    ;

						uint8_t red, green, blue, alpha;

						red   = 0x00;
						green = 0xFF;
						blue  = 0x00;
						alpha = 0xFF;

						uint8_t& buffer_red   = data.buffer.u8[framebOffset + (data.red  .offset / 8)];
						uint8_t& buffer_green = data.buffer.u8[framebOffset + (data.green.offset / 8)];
						uint8_t& buffer_blue  = data.buffer.u8[framebOffset + (data.blue .offset / 8)];

						buffer_red   = red   * (alpha / 256.) + buffer_red   * (1 - alpha / 256.);
						buffer_green = green * (alpha / 256.) + buffer_green * (1 - alpha / 256.);
						buffer_blue  = blue  * (alpha / 256.) + buffer_blue  * (1 - alpha / 256.);
					}
			}			
		}

		Awning::Wayland::Surface::HandleFrameCallbacks();
		Awning::Backend::Draw();
	}

	wl_display_destroy(Awning::server.display);
	ioctl(tty_fd, KDSETMODE, KD_TEXT);
	ioctl(tty_fd, KDSKBMODE, K_RAW);
}

void on_term_signal(int signal_number)
{
}

int XWM_Start(int signal_number, void *data)
{
	Log::Function::Called("");
	Awning::WM::X::Init();
	signal(SIGUSR1, on_term_signal);
	return 0;
}

void LaunchXwayland(int signal_number)
{
	Log::Function::Called("");

    char* XWaylandArgs [] = { "Xwayland", ":1", NULL };

	int fd = open("/dev/null", O_RDWR);
	//dup2(fd, STDOUT_FILENO);
	//dup2(fd, STDERR_FILENO);

	int ret = execvp(XWaylandArgs[0], XWaylandArgs);
	printf("Xwayland did not launch! %d %s\n", ret, strerror(errno));
	exit(ret);
}

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message)
{
	const char* direction_strings[] = { 
		"REQUEST", 
		"EVENT  "
	};

	if (direction == 1)
		return;

	//std::cout << "[" << direction_strings[direction] << "] " << message->resource->object.interface->name << ": " << message->message->name << "\n";
}

void client_created(struct wl_listener* listener, void* data)
{
	Log::Function::Called("");

	Awning::WM::Client::Create(data);

	if (!Awning::WM::X::xWaylandClient)
		Awning::WM::X::xWaylandClient = (wl_client*)data;
}