#pragma once
#include <onions/graphics.h>
#include <onions/event.h>


// Keyboard controls
#define KEY_LEFT				1
#define KEY_LEFT_DEFAULT		263

#define KEY_UP					2
#define KEY_UP_DEFAULT			265

#define KEY_RIGHT				3
#define KEY_RIGHT_DEFAULT		262

#define KEY_DOWN				4
#define KEY_DOWN_DEFAULT		264


#define KEY_SELECT				5
#define KEY_SELECT_DEFAULT		90

#define KEY_CANCEL				6
#define KEY_CANCEL_DEFAULT		88


onion::Palette* get_clear_palette();
onion::SinglePalette* get_ui_palette();

onion::Font* get_ui_font();