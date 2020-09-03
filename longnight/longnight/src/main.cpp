#include <ctime>
#include "../include/ui.h"
#include "../include/battle.h"


onion::SinglePalette* g_UIPalette = nullptr;

onion::Palette* get_ui_palette()
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

	// Load the UI assets.
	g_UIPalette = new onion::SinglePalette(vec4i(255, 255, 255, 0), vec4i(200, 184, 176, 0), vec4i(0, 0, 0, 0));
	g_UIFont = onion::Font::load_sprite_font("fonts/outline11.png");

	// Set the state.
	onion::set_state(new battle::State({ "trafmimic", "trafmimic" }));

	// Run the Onion main function.
	onion::state_main();
}