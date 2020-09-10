#pragma once
#include "item.h"

namespace overworld
{


	// A character controlled by the player.
	struct Ally
	{
		// The current Health of the ally.
		int cur_health = 999;

		// The maximum Health of the ally.
		int max_health = 999;


		// The items held by the ally.
		std::vector<Item*> items;
	};


	/// <summary>Retrieves all ally characters.</summary>
	/// <returns>An array of all player-controlled characters.</returns>
	std::vector<Ally>& get_party();


}