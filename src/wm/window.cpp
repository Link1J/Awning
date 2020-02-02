#include "window.hpp"

namespace Awning::WM
{
	Window* Window::Create(void* client)
	{
		Window* window = new Window();

		window->data = nullptr;
		window->mapped = false;
		window->xPos = 10;
		window->yPos = 10;

		Manager::Window::Add(window);
		Client::Bind(client, window);

		return window;
	}

	void Window::Destory(Window*& window)
	{
		Manager::Window::Remove(window);
		Client::Unbind(window);
		delete window;
		window = nullptr;
	}

	Texture::Data* Window::Texture()
	{
		return &texture;
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
		return xPos;
	}

	int Window::YPos()
	{
		return yPos;
	}

	int Window::XSize()
	{
		return xSize;
	}

	int Window::YSize()
	{
		return ySize;
	}

	void Window::Frame(bool frame)
	{
		needsFrame = frame;
	}

	bool Window::Frame()
	{
		return needsFrame;
	}

	void Window::ConfigPos(int xPos, int yPos)
	{
		this->xPos  = xPos ;
		this->yPos  = yPos ;
	}

	void Window::ConfigSize(int xSize, int ySize)
	{
		this->xSize = xSize;
		this->ySize = ySize;
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
}