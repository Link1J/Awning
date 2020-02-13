#pragma once
#include "wm/texture.hpp"

namespace Awning::Backend::X11 
{
	void Start();
	void Poll();
	void Hand();
	void Draw();
	
	Awning::WM::Texture Data();
}