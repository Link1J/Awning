#pragma once
#include <X11/X.h>
#include <X11/Xlib.h>

namespace Awning::WM::X::Server
{
	extern int x_fd[2], wl_fd[2], wm_fd[2];
	
	void Setup();
	void Run();
}