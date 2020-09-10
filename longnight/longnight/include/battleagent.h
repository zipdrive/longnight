#pragma once
#include "battle.h"

namespace battle
{
	
	// An agent that acts totally at random.
	class RandomAgent : public Agent
	{
	public:
		/// <summary>Constructs an agent that acts totally at random.</summary>
		/// <param name="self">The entity that the agent acts for.</param>
		RandomAgent(Entity* self);

		/// <summary>Picks a random action for the entity.</summary>
		/// <param name="turn">To be filled out with the data for the turn.</param>
		void decide(Turn& turn);
	};

}