#pragma once
#include <map>
#include <onions/event.h>

namespace battle
{

	// Predefinitions
	class Entity;
	struct Turn;

	template <typename... _Args>
	class Listener;

	
	
	template <typename... _Args>
	class ListenerQueue
	{
	private:
		// A queue of events, in order of priority.
		std::multimap<int, Listener<_Args...>*> m_Queue;

	public:
		/// <summary>Checks if there are no listeners in the queue.</summary>
		/// <returns>True if the queue is empty, false otherwise.</returns>
		bool empty()
		{
			return m_Queue.empty();
		}

		/// <summary>Pushes a listener into the queue.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="listener">The listener to add.</param>
		void push(int priority, Listener<_Args...>* listener)
		{
			m_Queue.emplace(priority, listener);
		}

		/// <summary>Removes a listener from the queue.</summary>
		/// <param name="listener">The listener to remove.</param>
		void pop(Listener<_Args...>* listener)
		{
			for (auto iter = m_Queue.begin(); iter != m_Queue.end(); ++iter)
			{
				if (iter->second == listener)
				{
					m_Queue.erase(iter);
					break;
				}
			}
		}

		/// <summary>Triggers all listeners.</summary>
		void trigger(_Args... args)
		{
			for (auto iter = m_Queue.begin(); iter != m_Queue.end(); ++iter)
			{
				iter->second->trigger(args...);
			}
		}
	};

	template <typename... _Args>
	class Listener
	{
	public:
		typedef ListenerQueue<_Args...> _Queue;
		typedef std::unordered_map<Entity*, _Queue*> _Map;

	private:
		_Map& m_ListenerMap;

	protected:
		// The entity being listened to.
		Entity* m_Entity;

	public:
		Listener(_Map& map, int priority, Entity* entity) : m_ListenerMap(map)
		{
			m_Entity = entity;

			_Queue* queue = nullptr;

			auto iter = m_ListenerMap.find(entity);
			if (iter == m_ListenerMap.end())
			{
				queue = new _Queue();
				m_ListenerMap.emplace(entity, queue);
			}
			else
			{
				queue = iter->second;
			}

			if (queue)
			{
				queue->push(priority, this);
			}
		}

		virtual ~Listener()
		{
			auto iter = m_ListenerMap.find(m_Entity);
			if (iter != m_ListenerMap.end())
			{
				iter->second->pop(this);

				if (iter->second->empty())
				{
					m_ListenerMap.erase(m_Entity);
				}
			}
		}

		virtual void trigger(_Args... args) = 0;
	};

	
	
	// A listener that triggers whenever a particular entity passes a tick.
	class TickListener : public Listener<>
	{
	public:
		/// <summary>Creates a listener for when a particular entity passes a tick.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		TickListener(int priority, Entity* entity);
	};
	

	
	class TurnBeginListener : public Listener<Turn*>
	{
	public:
		/// <summary>Creates a listener for when a particular entity begins their turn.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		TurnBeginListener(int priority, Entity* entity);
	};
	
	// A listener that triggers whenever a particular entity ends their turn.
	class TurnEndListener : public Listener<>
	{
	public:
		/// <summary>Creates a listener for when a particular entity's turn ends.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		TurnEndListener(int priority, Entity* entity);
	};



	class DamageEvent;

	// A listener that triggers before a particular entity takes damage.
	class BeforeTakeDamageListener : public Listener<DamageEvent*>
	{
	public:
		/// <summary>Creates a listener for when a particular entity's turn ends.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		BeforeTakeDamageListener(int priority, Entity* entity);
	};
	
	// A listener that triggers whenever a particular entity deals damage.
	class DealDamageListener : public Listener<DamageEvent* >
	{
	public:
		/// <summary>Creates a listener for when a particular entity deals damage.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		DealDamageListener(int priority, Entity* entity);
	};

	// A listener that triggers after a particular entity takes damage.
	class AfterTakeDamageListener : public Listener<DamageEvent*>
	{
	public:
		/// <summary>Creates a listener that triggers after a particular entity takes damage.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		AfterTakeDamageListener(int priority, Entity* entity);
	};


	
	class InflictStatusEvent;

	// A listener that triggers before a particular entity is inflicted with a status effect.
	class BeforeStatusInflictedListener : public Listener<InflictStatusEvent*>
	{
	public:
		/// <summary>Creates a listener for when a particular entity's turn ends.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		BeforeStatusInflictedListener(int priority, Entity* entity);
	};

	// A listener that triggers whenever a particular entity inflicts a status effect.
	class InflictStatusListener : public Listener<InflictStatusEvent* >
	{
	public:
		/// <summary>Creates a listener for when a particular entity deals damage.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		InflictStatusListener(int priority, Entity* entity);
	};

	// A listener that triggers after a particular entity is inflicted with a status effect.
	class AfterStatusInflictedListener : public Listener<InflictStatusEvent*>
	{
	public:
		/// <summary>Creates a listener that triggers after a particular entity is inflicted with a status effect.</summary>
		/// <param name="priority">The priority for the listener. Low numbers trigger before high numbers.</param>
		/// <param name="entity">The entity in question.</param>
		AfterStatusInflictedListener(int priority, Entity* entity);
	};


	


}