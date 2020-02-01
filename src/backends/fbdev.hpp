#pragma once
#include "wm/texture.hpp"

namespace Awning::Backend::FBDEV 
{
	void Start();
	void Poll();
	void Draw();
	
	Awning::WM::Texture::Data Data();
}