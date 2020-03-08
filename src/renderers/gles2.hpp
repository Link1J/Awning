#pragma once

#include <wayland-server.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#include "wm/texture.hpp"
#include "egl.hpp"

namespace Awning::Renderers::GLES2
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