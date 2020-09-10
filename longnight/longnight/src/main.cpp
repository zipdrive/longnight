#include <ctime>
#include "../include/ui.h"
#include "../include/battle.h"
#include "../include/party.h"

using namespace onion;


onion::SinglePalette* g_ClearPalette = nullptr;
onion::SinglePalette* g_UIPalette = nullptr;

onion::Palette* get_clear_palette()
{
	return g_ClearPalette;
}

onion::SinglePalette* get_ui_palette()
{
	return g_UIPalette;
}


onion::Font* g_UIFont = nullptr;

onion::Font* get_ui_font()
{
	return g_UIFont;
}


int main()
{
	// Initialize the Onion library.
	onion::init("settings.ini");

	// Set the random seed
	srand(time(nullptr));

	// Register the keyboard controls
	register_keyboard_control(KEY_SELECT, KEY_SELECT_DEFAULT);
	register_keyboard_control(KEY_CANCEL, KEY_CANCEL_DEFAULT);
	register_keyboard_control(KEY_LEFT, KEY_LEFT_DEFAULT);
	register_keyboard_control(KEY_DOWN, KEY_DOWN_DEFAULT);
	register_keyboard_control(KEY_RIGHT, KEY_RIGHT_DEFAULT);
	register_keyboard_control(KEY_UP, KEY_UP_DEFAULT);

	// Load the UI assets.
	g_ClearPalette = new onion::SinglePalette(vec4i(255, 0, 0, 0), vec4i(0, 255, 0, 0), vec4i(0, 0, 255, 0));
	g_UIPalette = new onion::SinglePalette(vec4i(255, 255, 255, 0), vec4i(200, 184, 176, 0), vec4i(0, 0, 0, 0));
	g_UIFont = onion::Font::load_sprite_font("fonts/outline11.png");

	// Set up allies
	std::vector<overworld::Ally>& party = overworld::get_party();
	party.resize(3);

	for (int k = 2; k >= 0; --k)
	{
		party[k].items.push_back(overworld::Item::get_item("debug offense"));
		party[k].items.push_back(overworld::Item::get_item("debug offense"));
		party[k].items.push_back(overworld::Item::get_item("debug offense"));
		party[k].items.push_back(overworld::Item::get_item("debug offense"));
		party[k].items.push_back(overworld::Item::get_item("debug support"));
	}

	// Set the state.
	onion::set_state(new battle::State({ "trafmimic", "trafmimic" }));

	// Run the Onion main function.
	onion::state_main();
}