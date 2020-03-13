#pragma once
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

#include <spdlog/spdlog.h>

#include <fmt/format.h>

namespace Awning::Utils::Sockets
{
	static bool SetCloexec(int fd, bool cloexec) 
	{
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

	static int Open(struct sockaddr_un *addr, size_t path_size)
	{
		int fd, rc;
		socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

		fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if (fd < 0) {
			spdlog::error(
				"Failed to create socket {}{}",
				addr->sun_path[0] ? addr->sun_path[0] : '@',
				addr->sun_path + 1
			);
			return -1;
		}

		if (addr->sun_path[0]) {
			unlink(addr->sun_path);
		}
		if (bind(fd, (struct sockaddr*)addr, size) < 0) {
			rc = errno;
			//spdlog::error(
			//	"Failed to bind socket {}{}",
			//	addr->sun_path[0] ? addr->sun_path[0] : '@',
			//	addr->sun_path + 1
			//);
			goto cleanup;
		}
		if (listen(fd, 1) < 0) {
			rc = errno;
			spdlog::error(
				"Failed to listen to socket {}{}",
				addr->sun_path[0] ? addr->sun_path[0] : '@',
				addr->sun_path + 1
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
}