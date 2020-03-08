#pragma once

#include <wayland-server.h>

#include "wm/texture.hpp"

namespace Awning::Renderers::Software
{
	void Init();
	void Draw();

	namespace FillTextureFrom
	{
		void EGLImage   (wl_resource  * buffer, Awning::Texture* texture, Awning::Damage damage);
		void SHMBuffer  (wl_shm_buffer* buffer, Awning::Texture* texture, Awning::Damage damage);
		void LinuxDMABuf(wl_resource  * buffer, Awning::Texture* texture, Awning::Damage damage);
	}
}