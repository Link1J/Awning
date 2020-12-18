#pragma once
#include <systemd/sd-login.h>
#include <sdbus-c++/sdbus-c++.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <spdlog/spdlog.h>
#include <iostream>

namespace Awning::Session
{
	struct FileDescriptor
	{
		sdbus::UnixFd fd;
		uint32_t major;
		uint32_t minor;

		operator int() const
		{
			return fd.get();
		}
	};

	namespace _impl
	{
		inline std::string session_id;
		inline sdbus::ObjectPath session_path;
		inline std::unique_ptr<sdbus::IProxy> logind;
		inline std::unique_ptr<sdbus::IProxy> session;
		inline std::unordered_map<std::string, std::tuple<FileDescriptor, size_t>> devices;
	}

	[[nodiscard]] inline bool TakeControl()
	{
		using namespace _impl;
		try
		{
			session->callMethod("TakeControl")
				.onInterface("org.freedesktop.login1.Session")
				.withArguments(false);
		}
		catch (sdbus::Error error)
		{
			spdlog::error("({}) {}", error.getName(), error.getMessage());
			return false;
		}

		return true;
	}

	inline void Init()
	{
		using namespace _impl;
		logind = sdbus::createProxy("org.freedesktop.login1", "/org/freedesktop/login1");

		std::make_unsigned_t<decltype(getpid())> pid = getpid();
		session_id = std::getenv("XDG_SESSION_ID");

		spdlog::debug("PID: {}", pid);
		spdlog::debug("Session ID: {}", session_id);

		logind->callMethod("GetSession")
			.onInterface("org.freedesktop.login1.Manager")
			.withArguments(session_id)
			.storeResultsTo(session_path);
		
		spdlog::debug("Session Path: {}", session_path);
		session = sdbus::createProxy("org.freedesktop.login1", session_path);

		(void)TakeControl();
	}

	[[nodiscard]] inline auto GetDevice(std::filesystem::path device_path)
	{
		using namespace _impl;

		struct stat stats;
		stat(device_path.c_str(), &stats);

		bool inactive;
		FileDescriptor device;
		device.major = major(stats.st_rdev); //(stats.st_rdev >> 16) & 0xFF;
		device.minor = minor(stats.st_rdev); //(stats.st_rdev >>  0) & 0xFF;

		try
		{
			session->callMethod("TakeDevice")
				.onInterface("org.freedesktop.login1.Session")
				.withArguments(device.major, device.minor)
				.storeResultsTo(device.fd, inactive);
		}
		catch (sdbus::Error error)
		{
			spdlog::critical("({}) {} {} {:02X}{:02X}", error.getName(), error.getMessage(), device_path.string(), device.major, device.minor);
			throw;
		}

		return device;
	}

	inline void ReleaseDevice(FileDescriptor fd)
	{
		using namespace _impl;
		try 
		{
			session->callMethod("ReleaseDevice")
				.onInterface("org.freedesktop.login1.Session")
				.withArguments(fd.major, fd.minor)
				.dontExpectReply();
		}
		catch (sdbus::Error error)
		{
			spdlog::critical("({}) {}", error.getName(), error.getMessage());
			throw;
		}
	}
}