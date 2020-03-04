#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11

#include "backends/manager.hpp"

#include <wayland-server.h>

#include <linux/kd.h>

#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "protocols/wl/compositor.hpp"
#include "protocols/wl/seat.hpp"
#include "protocols/wl/output.hpp"
#include "protocols/wl/shell.hpp"
#include "protocols/wl/surface.hpp"
#include "protocols/wl/shell_surface.hpp"
#include "protocols/wl/pointer.hpp"
#include "protocols/wl/data_device_manager.hpp"
#include "protocols/wl/subcompositor.hpp"

#include "protocols/xdg/wm_base.hpp"

#include "protocols/zxdg/decoration.hpp"
#include "protocols/zxdg/output.hpp"

#include "protocols/wlr/output_manager.hpp"

#include "protocols/zwp/dmabuf.hpp"

#include "protocols/kde/decoration.hpp"

#include "wm/x/wm.hpp"
#include "wm/window.hpp"
#include "wm/client.hpp"
#include "wm/manager.hpp"
#include "wm/x/server.hpp"

#include "log.hpp"

#include "renderers/manager.hpp"

#include <fmt/format.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>

#include <iostream>
#include <unordered_map>

void ProtocolLogger(void* user_data, wl_protocol_logger_type direction, const wl_protocol_logger_message* message);
void on_term_signal(int signal_number);
void client_created(struct wl_listener *listener, void *data);
void launchApp(const char** argv);

namespace Awning
{
	namespace Server
	{
		struct Data
		{
			wl_display* display;
			wl_event_loop* event_loop;
			wl_protocol_logger* logger; 
			wl_listener client_listener;
		};
		Data data;
	}
};

extern int tty_fd;

uint32_t lastSerialNum = 1;

uint32_t NextSerialNum()
{
	lastSerialNum++;
	return lastSerialNum;
}

int main(int argc, char* argv[])
{
	bool                   startXWayland = true                             ;
	Awning::Backend  ::API api_output    = Awning::Backend  ::API::DRM      ;
	Awning::Backend  ::API api_input     = Awning::Backend  ::API::libinput ;
	Awning::Renderers::API api_drawing   = Awning::Renderers::API::OpenGLES2;

	for(int a = 0; a < argc; a++)
	{
		auto arg = std::string(argv[a]);
		if (arg == "-noX")
		{
			startXWayland = false;
		}
		if (arg == "-x11")
		{
			api_output    = Awning::Backend  ::API::X11;
			api_input     = Awning::Backend  ::API::X11;
		}
		if (arg == "-drm")
		{
			api_output    = Awning::Backend  ::API::DRM;
		}
		if (arg == "-libinput")
		{
			api_input     = Awning::Backend  ::API::libinput;
		}
		if (arg == "-evdev")
		{
			api_input     = Awning::Backend  ::API::EVDEV;
		}
		if (arg == "-soft")
		{
			api_drawing   = Awning::Renderers::API::Software;
		}
		if (arg == "-GLES2")
		{
			api_drawing   = Awning::Renderers::API::OpenGLES2;
		}
	}

	Awning::Server::data.display = wl_display_create(); 
	const char* socket = wl_display_add_socket_auto(Awning::Server::data.display);
	std::cout << "Wayland Socket: " << socket << std::endl;

	Awning::Server::data.client_listener.notify = client_created;

	Awning::Server::data.event_loop = wl_display_get_event_loop(Awning::Server::data.display);
	wl_display_add_client_created_listener(Awning::Server::data.display, &Awning::Server::data.client_listener);

	Awning::Backend::Init(api_output, api_input);
	Awning::Renderers::Init(api_drawing);

	Awning::Protocols::WL  ::Compositor         ::Add(Awning::Server::data.display);
	Awning::Protocols::WL  ::Seat               ::Add(Awning::Server::data.display);
	//Awning::Protocols::WL  ::Shell              ::Add(Awning::Server::data.display);
	Awning::Protocols::XDG ::WM_Base            ::Add(Awning::Server::data.display);
	Awning::Protocols::ZXDG::Decoration_Manager ::Add(Awning::Server::data.display);
	Awning::Protocols::WL  ::Data_Device_Manager::Add(Awning::Server::data.display);
	Awning::Protocols::WLR ::Output_Manager     ::Add(Awning::Server::data.display);
	Awning::Protocols::ZXDG::Output_Manager     ::Add(Awning::Server::data.display);
	Awning::Protocols::ZWP ::Linux_Dmabuf       ::Add(Awning::Server::data.display);
	Awning::Protocols::WL  ::Subcompositor      ::Add(Awning::Server::data.display);
	Awning::Protocols::KDE ::Decoration_Manager ::Add(Awning::Server::data.display);

	wl_display_init_shm(Awning::Server::data.display);

	if (startXWayland) Awning::WM::X::Server::Setup();

	const char* launchArgs1[] = { "falkon", "-platform", "wayland", NULL };
    const char* launchArgs2[] = { "weston-terminal", NULL };
    const char* launchArgs3[] = { "firefox", NULL };
    const char* launchArgs4[] = { "ksysguard", "-platform", "wayland", NULL };
    const char* launchArgs5[] = { "konsole", "-platform", "wayland", NULL };
    const char* launchArgs6[] = { "termite", NULL };

	int display = Awning::WM::X::Server::display;

	setenv("DISPLAY", fmt::format(":{}", display).c_str(), 1);
	setenv("MOZ_ENABLE_WAYLAND", "1", 1);

	//signal(SIGINT, on_term_signal);

	//launchApp(launchArgs1);
	//launchApp(launchArgs2);
	//launchApp(launchArgs3);
	//launchApp(launchArgs4);
	launchApp(launchArgs5);
	//launchApp(launchArgs6);
	
	while(1)
	{
		Awning::Backend::Poll();
		Awning::Backend::Hand();

		wl_event_loop_dispatch(Awning::Server::data.event_loop, 0);
		wl_display_flush_clients(Awning::Server::data.display);

		Awning::WM::X::EventLoop();

		Awning::Renderers::Draw();

		Awning::Backend::Draw();

		Awning::Protocols::WL::Surface::HandleFrameCallbacks();
	}

	wl_display_destroy(Awning::Server::data.display);
}

void on_term_signal(int signal_number)
{
}

void client_created(struct wl_listener* listener, void* data)
{
	Log::Function::Called("");

	Awning::WM::Client::Create(data);
}

/*void GetSockAddress()
{
	const char *dir = getenv("XDG_RUNTIME_DIR");
	if (!dir) {
		dir = "/tmp";
	}
	if (path_size <= snprintf(ipc_sockaddr->sun_path, path_size,
			"%s/awning-ipc.%i.%i.sock", dir, getuid(), getpid())) {
	}
}*/

void launchApp(const char** argv)
{
	int pid = fork();
	if (pid == 0) 
	{
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);

		int ret = execvp(argv[0], (char**)argv);
		printf("%s did not launch! %d %s\n", argv[0], ret, strerror(errno));
		exit(ret);
	}
}

// Important Comment
//................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................