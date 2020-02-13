#pragma once

#include <wayland-server.h>

#include "wm/texture.hpp"

namespace Awning::Renderers::Software
{
	void Init();
	void Draw();

	namespace FillTextureFrom
	{
		void EGLImage (wl_resource* buffer, WM::Texture* texture, WM::Damage damage);
		void SHMBuffer(wl_resource* buffer, WM::Texture* texture, WM::Damage damage);
	}
}