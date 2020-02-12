#include "window.hpp"

namespace Awning::WM
{
	Window* Window::Create(void* client)
	{
		Window* window = new Window();

		window->data = nullptr;
		window->texture = nullptr;
		window->mapped = false;
		window->pos.x = 0;
		window->pos.y = 0;
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

		window->data = nullptr;
		window->texture = nullptr;
		window->mapped = false;
		window->pos.x = 0;
		window->pos.y = 0;
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

	Texture::Data* Window::Texture()
	{
		return texture;
	}

	void Window::Mapped(bool map)
	{
		mapped = map;
	}

	bool Window::Mapped()
	{
		return mapped;
	}

	int Window::XPos()
	{
		return pos.x;
	}

	int Window::YPos()
	{
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
		return offset.x;
	}

	int Window::YOffset()
	{
		return offset.x;
	}

	void Window::Texture(Texture::Data* texture)
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
}