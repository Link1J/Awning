
namespace Awning::WM::Input
{
	class Keymap
	{
		static struct xkb_context* ctx;
		struct xkb_keymap* keymap;
		struct xkb_state* state;
	};

	class Device
	{
	public:
		enum class Type { Keyboard, Mouse, Tablet, Switch };
	
	private:
		Type type;
	};

	class Seat
	{

	};
}