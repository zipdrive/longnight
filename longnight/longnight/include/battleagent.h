#pragma once
#include "battle.h"

namespace battle
{

	// An agent that damages a random enemy each turn.
	class BerserkAgent : public Agent
	{
	private:
		// An action that damages a random enemy.
		static Action m_BerserkAction;

	public:
		/// <summary>Constructs an agent that damages a random enemy each turn.</summary>
		/// <param name="self">The entity represented by the agent.</param>
		BerserkAgent(Entity* self);

		/// <summary>Enqueues the entity's action.</summary>
		void enqueue();
	};

}