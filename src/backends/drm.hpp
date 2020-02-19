#pragma once
#include "wm/texture.hpp"
#include "manager.hpp"

namespace Awning::Backend::DRM 
{
	void Start();
	void Poll();
	void Draw();
	
	Displays GetDisplays();
}