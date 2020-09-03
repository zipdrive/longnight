#pragma once
#include <onions/event.h>

namespace battle
{

	// Predefinitions
	class Entity;



	template <typename... Args>
	class Listener
	{
	public:
		virtual void trigger(Args&&... args) = 0;
	};
	
	
	// A listener that triggers whenever a particular entity passes a tick.
	class TickListener : public Listener<>
	{
	protected:
		// The entity being listened to.
		Entity* m_Entity;

	public:
		/// <summary>Creates a listener for when a particular entity passes a tick.</summary>
		/// <param name="entity">The entity in question.</param>
		TickListener(int priority, Entity* entity);

		/// <summary>Destroys the listener.</summary>
		~TickListener();
	};
	

	
	// A listener that triggers whenever a particular entity ends their turn.
	class TurnEndListener : public Listener<>
	{
	protected:
		// The entity being listened to.
		Entity* m_Entity;

	public:
		/// <summary>Creates a listener for when a particular entity's turn ends.</summary>
		/// <param name="entity">The entity in question.</param>
		TurnEndListener(int priority, Entity* entity);

		/// <summary>Destroys the listener.</summary>
		~TurnEndListener();
	};


	


}