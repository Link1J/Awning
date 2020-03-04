#include "server.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "wm.hpp"

#include "log.hpp"

#include <fmt/format.h>

static const char* lock_fmt = "/tmp/.X%d-lock";
static const char* socket_dir = "/tmp/.X11-unix";
static const char* socket_fmt = "/tmp/.X11-unix/X%d";

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
		extern Data data;
	}
};

static bool set_cloexec(int fd, bool cloexec) {
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
		return false;
	}
	if (cloexec) {
		flags = flags | FD_CLOEXEC;
	} else {
		flags = flags & ~FD_CLOEXEC;
	}
	if (fcntl(fd, F_SETFD, flags) == -1) {
		return false;
	}
	return true;
}

static int open_socket(struct sockaddr_un *addr, size_t path_size) {
	int fd, rc;
	socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

	fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		Log::Report::Error(
			fmt::format(
				"Failed to create socket {}{}",
				addr->sun_path[0] ? addr->sun_path[0] : '@',
				addr->sun_path + 1
			)
		);
		return -1;
	}

	if (addr->sun_path[0]) {
		unlink(addr->sun_path);
	}
	if (bind(fd, (struct sockaddr*)addr, size) < 0) {
		rc = errno;
		Log::Report::Error(
			fmt::format(
				"Failed to bind socket {}{}",
				addr->sun_path[0] ? addr->sun_path[0] : '@',
				addr->sun_path + 1
			)
		);
		goto cleanup;
	}
	if (listen(fd, 1) < 0) {
		rc = errno;
		Log::Report::Error(
			fmt::format(
				"Failed to listen to socket {}{}",
				addr->sun_path[0] ? addr->sun_path[0] : '@',
				addr->sun_path + 1
			)
		);
		goto cleanup;
	}

	return fd;

cleanup:
	close(fd);
	if (addr->sun_path[0]) {
		unlink(addr->sun_path);
	}
	errno = rc;
	return -1;
}

static bool open_sockets(int socks[2], int display) {
	struct sockaddr_un addr = { .sun_family = AF_LOCAL };
	size_t path_size;

	mkdir(socket_dir, 0777);

#ifdef __linux__
	addr.sun_path[0] = 0;
	path_size = snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, socket_fmt, display);
#else
	path_size = snprintf(addr.sun_path, sizeof(addr.sun_path), socket_fmt, display);
#endif
	socks[0] = open_socket(&addr, path_size);
	if (socks[0] < 0) {
		return false;
	}

	path_size = snprintf(addr.sun_path, sizeof(addr.sun_path), socket_fmt, display);
	socks[1] = open_socket(&addr, path_size);
	if (socks[1] < 0) {
		close(socks[0]);
		socks[0] = -1;
		return false;
	}

	return true;
}

void unlink_display_sockets(int display) {
	char sun_path[64];

	snprintf(sun_path, sizeof(sun_path), socket_fmt, display);
	unlink(sun_path);

	snprintf(sun_path, sizeof(sun_path), lock_fmt, display);
	unlink(sun_path);
}

int open_display_sockets(int socks[2]) 
{
	int lock_fd, display;
	char lock_name[64];

	for (display = 0; display <= 32; display++) {
		snprintf(lock_name, sizeof(lock_name), lock_fmt, display);
		if ((lock_fd = open(lock_name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0444)) >= 0) {
			if (!open_sockets(socks, display)) {
				unlink(lock_name);
				close(lock_fd);
				continue;
			}
			char pid[12];
			snprintf(pid, sizeof(pid), "%10d", getpid());
			if (write(lock_fd, pid, sizeof(pid) - 1) != sizeof(pid) - 1) {
				unlink(lock_name);
				close(lock_fd);
				continue;
			}
			close(lock_fd);
			break;
		}

		if ((lock_fd = open(lock_name, O_RDONLY | O_CLOEXEC)) < 0) {
			continue;
		}

		char pid[12] = { 0 }, *end_pid;
		ssize_t bytes = read(lock_fd, pid, sizeof(pid) - 1);
		close(lock_fd);

		if (bytes != sizeof(pid) - 1) {
			continue;
		}
		long int read_pid;
		read_pid = strtol(pid, &end_pid, 10);
		if (read_pid < 0 || read_pid > INT32_MAX || end_pid != pid + sizeof(pid) - 2) {
			continue;
		}
		errno = 0;
		if (kill((pid_t)read_pid, 0) != 0 && errno == ESRCH) {
			if (unlink(lock_name) != 0) {
				continue;
			}
			// retry
			display--;
			continue;
		}
	}

	if (display > 32) {
		Log::Report::Error("No display available in the first 33");
		return -1;
	}

	return display;
}

namespace Awning::WM::X::Server
{
    int pid, display;
	wl_event_source* sigusr1;
	int x_fd[2], wl_fd[2], wm_fd[2];

	int XWM_Start(int signal_number, void* data)
	{
		Log::Function::Called("WM::X::Server");
		Awning::WM::X::Init();
		return 0;
	}

	template<typename... Args>
	void FillArg(char** arg, const std::string format, const Args&... args)
	{
		auto string = fmt::format(format, args...);
		*arg = (char*)malloc(string.length() + 1);
		memset(*arg, 0, string.length() + 1);
		mempcpy(*arg, string.c_str(), string.length());
	}

	void LaunchXwayland(int signal_number)
	{
		Log::Function::Called("WM::X::Server");

		set_cloexec(x_fd [0], false);
		set_cloexec(x_fd [1], false);
		set_cloexec(wm_fd[1], false);
		set_cloexec(wl_fd[1], false);

		char wayland_socket_str[16];
		snprintf(wayland_socket_str, sizeof(wayland_socket_str), "%d", wl_fd[1]);
		setenv("WAYLAND_SOCKET", wayland_socket_str, true);

	    char* XWaylandArgs[] = { 
			"Xwayland", NULL, 
			"-rootless", "-terminate",
			"-listen", NULL,
			"-listen", NULL,
			"-wm", NULL,
			NULL
		};
		
		FillArg(&(XWaylandArgs[1]), ":{}", display);
		FillArg(&(XWaylandArgs[5]), "{}", x_fd [0]);
		FillArg(&(XWaylandArgs[7]), "{}", x_fd [1]);
		FillArg(&(XWaylandArgs[9]), "{}", wm_fd[1]);

		Log::Report::Info(fmt::format("WAYLAND_SOCKET={} Xwayland :{} -rootless -terminate -listen {} -listen {} -wm {}",
		wl_fd[1], display, x_fd[0], x_fd[1], wm_fd[1]));

		int fd = open("/dev/null", O_RDWR);
		//dup2(fd, STDOUT_FILENO);
		//dup2(fd, STDERR_FILENO);

		int ret = execvp("Xwayland", XWaylandArgs);
		printf("Xwayland did not launch! %d %s\n", ret, strerror(errno));
		exit(ret);
	}

	void Setup()
	{
		display = open_display_sockets(x_fd);
		socketpair(AF_UNIX, SOCK_STREAM, 0, wl_fd);
		socketpair(AF_UNIX, SOCK_STREAM, 0, wm_fd);

		set_cloexec(wl_fd[0], true);
		set_cloexec(wl_fd[1], true);
		set_cloexec(wm_fd[0], true);
		set_cloexec(wm_fd[1], true);

		xWaylandClient = wl_client_create(Awning::Server::data.display, wl_fd[0]);
		
		sigusr1 = wl_event_loop_add_signal(Awning::Server::data.event_loop, SIGUSR1, XWM_Start, nullptr);

		int pidT = fork();
		if (pidT == 0) 
		{
			signal(SIGUSR1, SIG_IGN);
			LaunchXwayland(0);
		}

		close(wl_fd[1]);
		close(wm_fd[1]);
	}

	void Run()
	{
		if (pid == 0)
			return;

		//kill(pid, SIGUSR2);

		//close(wl_fd[1]);
		//close(wm_fd[1]);
		//wl_fd[1] = wm_fd[1] = -1;
	}
}