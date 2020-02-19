#pragma once
#include "wm/texture.hpp"
#include "manager.hpp"

namespace Awning::Backend::X11 
{
	void Start();
	void Poll();
	void Hand();
	void Draw();
	
	Displays GetDisplays();
}