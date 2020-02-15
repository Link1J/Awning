#pragma once

#include <wayland-server.h>

#include "wm/texture.hpp"

namespace Awning::Renderers::GLES2
{
	void Init();
	void Draw();

	namespace FillTextureFrom
	{
		void EGLImage (wl_resource  * buffer, WM::Texture* texture, WM::Damage damage);
		void SHMBuffer(wl_shm_buffer* buffer, WM::Texture* texture, WM::Damage damage);
	}
}