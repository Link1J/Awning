#pragma once
#include "wm/texture.hpp"

namespace Awning::Backend::DRM 
{
	void Start();
	void Poll();
	void Draw();
	
	Awning::WM::Texture::Data Data();
}