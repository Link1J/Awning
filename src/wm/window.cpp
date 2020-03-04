#include "window.hpp"

namespace Awning::WM
{
	Window* Window::Create(void* client)
	{
		Window* window = new Window();

		window->data      = nullptr;
		window->texture   = nullptr;
		window->parent    = nullptr;
		window->mapped    = false;
		window->pos.x     = INT32_MIN;
		window->pos.y     = INT32_MIN;
		window->minSize.x = 1;
		window->minSize.y = 1;
		window->maxSize.x = INT32_MAX;
		window->maxSize.y = INT32_MAX;

		Manager::Window::Add(window);
		Client::Bind::Window(client, window);

		return window;
	}

	Window* Window::CreateUnmanged(void* client)
	{
		Window* window = new Window();

		window->data      = nullptr;
		window->texture   = nullptr;
		window->parent    = nullptr;
		window->mapped    = false;
		window->pos.x     = INT32_MIN;
		window->pos.y     = INT32_MIN;
		window->minSize.x = 1;
		window->minSize.y = 1;
		window->maxSize.x = INT32_MAX;
		window->maxSize.y = INT32_MAX;

		Client::Bind::Window(client, window);

		return window;
	}

	void Window::Destory(Window*& window)
	{
		Manager::Window::Remove(window);
		Client::Unbind::Window(window);
		delete window;
		window = nullptr;
	}

	WM::Texture* Window::Texture()
	{
		return texture;
	}

	void Window::Mapped(bool map)
	{
		mapped = map;
	}

	bool Window::Mapped()
	{
		if (parent)
			return parent->Mapped() && texture && mapped;
		return texture && mapped;
	}

	int Window::XPos()
	{
		if (parent && parentOffsets)
			return parent->XPos() + pos.x;
		return pos.x;
	}

	int Window::YPos()
	{
		if (parent && parentOffsets)
			return parent->YPos() + pos.y;
		return pos.y;
	}

	int Window::XSize()
	{
		return size.x;
	}

	int Window::YSize()
	{
		return size.y;
	}

	void Window::Frame(bool frame)
	{
		needsFrame = frame;
	}

	bool Window::Frame()
	{
		return needsFrame;
	}

	void Window::ConfigPos(int xPos, int yPos, bool offset)
	{
		if (offset)
		{
			this->offset.x = xPos;
			this->offset.y = yPos;
		}
		else
		{
			this->pos.x  = xPos;
			this->pos.y  = yPos;
		}
	}

	void Window::ConfigSize(int xSize, int ySize)
	{
		this->size.x = xSize;
		this->size.y = ySize;
	}

	void Window::SetRaised(Manager::Functions::Window::Raised raised)
	{
		Raised = raised;
	}

	void Window::Data(void* data)
	{
		this->data = data;
	}

	void* Window::Client()
	{
		return client;
	}

	void Window::SetResized(Manager::Functions::Window::Resized resized)
	{
		Resized = resized;
	}

	int Window::XOffset()
	{
		if (parent && parentOffsets)
			return parent->XOffset() + offset.x;
		return offset.x;
	}

	int Window::YOffset()
	{
		if (parent && parentOffsets)
			return parent->YOffset() + offset.y;
		return offset.y;
	}

	void Window::Texture(WM::Texture* texture)
	{
		this->texture = texture;
	}

	void Window::ConfigMinSize(int xSize, int ySize)
	{
		this->minSize.x = xSize;
		this->minSize.y = ySize;
	}

	void Window::ConfigMaxSize(int xSize, int ySize)
	{
		if (xSize == 0 || ySize == 0)
		{
			this->maxSize.x = INT32_MAX;
			this->maxSize.y = INT32_MAX;
		}
		else
		{
			this->maxSize.x = xSize;
			this->maxSize.y = ySize;
		}
	}

	void Window::Parent(WM::Window* parent, bool offsets)
	{
		this->parent = parent;
		parentOffsets = offsets;
	}

	void Window::AddSubwindow(WM::Window* child)
	{
		child->Parent(this, true);
		child->drawingManaged = true;
		subwindows.push_back(child);
	}

	std::vector<Window*> Window::GetSubwindows()
	{
		return subwindows;
	}

	bool Window::DrawingManaged()
	{
		return drawingManaged;                                       
	}

	void Window::SetMoved(Manager::Functions::Window::Moved moved)
	{
		Moved = moved;
	}
}