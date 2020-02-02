#pragma once

namespace Awning::WM { class Window; }

namespace Awning::WM::Client
{
	void Create(void* id                );
	void Bind  (void* id, Window* window);
	void Unbind(          Window* window);

	void* Surface(Window* window);
}