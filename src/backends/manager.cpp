#include "manager.hpp"

#include "X11.hpp"
#include "fbdev.hpp"
#include "evdev.hpp"

namespace Awning::Backend
{
	Functions::Poll Poll;
	Functions::Draw Draw;
	Functions::Data Data;
	Functions::Hand Hand;

	std::vector<Output> outputs;

	void Init(API output, API input)
	{
		switch (output)
		{
		case API::X11:
			X11::Start();
			Poll = X11::Poll;
			Draw = X11::Draw;
			Data = X11::Data;
			break;
		case API::FBDEV:
			FBDEV::Start();
			Poll = FBDEV::Poll;
			Draw = FBDEV::Draw;
			Data = FBDEV::Data;
			break;
		
		default:
			break;
		}

		switch (input)
		{
		case API::X11:
			Hand = X11::Hand;
			break;
		case API::EVDEV:
			EVDEV::Start();
			Hand = EVDEV::Hand;
			break;
		
		default:
			break;
		}
	}

	std::vector<Output> Outputs::Get()
	{
		return outputs;
	}

	void Outputs::Add(Output output)
	{
		outputs.push_back(output);
	}

	void Outputs::Update(int a, Output output)
	{
		outputs[a] = output;
	}
}