#include "window.hpp"
#include "input.hpp"
#include "protocols/wl/keyboard.hpp"

namespace Awning
{
	std::list<Window*> Window::Manager::layers     [(int)Layer::END];

	void Window::Manager::Manage(Window*& window, Layer layer)
	{
		window->layer = layer;
		layers[(int)layer].emplace_back(window);
	}

	void Window::Manager::Unmanage(Window*& window)
	{
		if (window->layer >= Layer::END)
			return;
			
		auto curr = layers[(int)window->layer].begin();
		while (curr != layers[(int)window->layer].end())
		{
			if (*curr == window)
				break;
			curr++;
		}

		if (curr == layers[(int)window->layer].end())
			return;
		
		layers[(int)window->layer].erase(curr);
		Raise(*layers[(int)window->layer].begin());
	}

	void Window::Manager::Raise(Window*& window)
	{
		if (window->layer >= Layer::END)
			return;
			
		auto curr = layers[(int)window->layer].begin();
		while (curr != layers[(int)window->layer].end())
		{
			if (*curr == window)
				break;
			curr++;
		}

		if (curr == layers[(int)window->layer].end())
			return;

		layers[(int)window->layer].erase(curr);
		layers[(int)window->layer].emplace_front(window);

		if (window->Raised)
			window->Raised(window->data);

		auto seat = Input::Seat::seats[0];
		seat->active = window;
	}

	void Window::Manager::Resize(Window*& window, int xSize, int ySize)
	{
		if (xSize < window->minSize.x)
			xSize = window->minSize.x;
		if (ySize < window->minSize.y)
			ySize = window->minSize.y;
		if (xSize > window->maxSize.x)
			xSize = window->maxSize.x;
		if (ySize > window->maxSize.y)
			ySize = window->maxSize.y;
		
		if (window->Resized)
			window->Resized(window->data, xSize, ySize);

		window->size.x = xSize;
		window->size.y = ySize;
	}

	void Window::Manager::Move(Window*& window, int xPos, int yPos)
	{
		if (window->Moved)
			window->Moved(window->data, xPos, yPos);

		window->pos.x = xPos;
		window->pos.y = yPos;
	}

	void Window::Manager::Offset(Window*& window, int xOff, int yOff)
	{
		window->offset.x = xOff;
		window->offset.y = yOff;
	}

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
		window->layer     = Manager::Layer::END;

		Client::Bind::Window(client, window);

		return window;
	}

	void Window::Destory(Window*& window)
	{
		if (window->parent)
		{
			auto curr = window->parent->subwindows.begin();
			while (curr != window->parent->subwindows.end())
			{
				if (*curr == window)
					break;
				curr++;
			}
			if (curr != window->parent->subwindows.end())
			{
				window->parent->subwindows.erase(curr);
			}
		}
		Manager::Unmanage(window);
		Client::Unbind::Window(window);

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
		window->layer     = Manager::Layer::END;

		for (auto [id, seat] : Input::Seat::seats)
		{
			if (seat->active  == window)
				seat->active  = nullptr;
			if (seat->hovered == window)
				seat->hovered = nullptr;
		}

		delete window;
		window = nullptr;
	}

	Awning::Texture* Window::Texture()
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

	void Window::SetRaised(Functions::Raised raised)
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

	void Window::SetResized(Functions::Resized resized)
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

	void Window::Texture(Awning::Texture* texture)
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

	void Window::Parent(Window* parent, bool offsets)
	{
		this->parent = parent;
		parentOffsets = offsets;
	}

	void Window::AddSubwindow(Window* child)
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

	void Window::SetMoved(Functions::Moved moved)
	{
		Moved = moved;
	}

	void Window::SetLowered(Functions::Lowered lowered)
	{
		Lowered = lowered;
	}
}