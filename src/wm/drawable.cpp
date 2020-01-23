#include "drawable.hpp"

namespace Awning::WM::Drawable
{
	std::unordered_map<wl_resource*, Awning::WM::Drawable::Data> drawables;
}