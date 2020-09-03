#include <map>
#include <unordered_map>
#include "../include/battle.h"
#include "../include/battleevent.h"

using namespace std;
using namespace battle;


template <typename _Listener, typename... TriggerArgs>
class ListenerQueue
{
private:
	// A queue of events, in order of priority.
	multimap<int, _Listener*> m_Queue;

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
	void push(int priority, _Listener* listener)
	{
		m_Queue.emplace(priority, listener);
	}

	/// <summary>Removes a listener from the queue.</summary>
	/// <param name="listener">The listener to remove.</param>
	void pop(_Listener* listener)
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
	void trigger(TriggerArgs&&... args)
	{
		for (auto iter = m_Queue.begin(); iter != m_Queue.end(); ++iter)
		{
			iter->second->trigger(args...);
		}
	}
};



typedef ListenerQueue<TickListener> TickListenerQueue;
unordered_map<Entity*, TickListenerQueue*> g_TickListeners;


TickListener::TickListener(int priority, Entity* entity) : m_Entity(entity)
{
	TickListenerQueue* queue = nullptr;

	auto iter = g_TickListeners.find(entity);
	if (iter == g_TickListeners.end())
	{
		queue = new TickListenerQueue();
		g_TickListeners.emplace(entity, queue);
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

TickListener::~TickListener()
{
	auto iter = g_TickListeners.find(m_Entity);
	if (iter != g_TickListeners.end())
	{
		iter->second->pop(this);

		if (iter->second->empty())
		{
			g_TickListeners.erase(m_Entity);
		}
	}
}


int TickEvent::start()
{
	auto iter = g_TickListeners.find(m_Entity);
	if (iter != g_TickListeners.end())
	{
		iter->second->trigger();
	}

	// debug TODO remove later
	if (m_Entity->time <= 0)
	{
		m_Entity->time = TIMELINE_MAX;

		Queue::insert(new NumberEvent(254, m_Entity->coordinates + vec2i(m_Entity->dimensions.get(0) / 2, m_Entity->dimensions.get(1) / 2)), INT_MIN);
		Queue::insert(new ShakeEvent(m_Entity->coordinates, vec2i(3, 1), 0.25f), INT_MIN + 1);
		Queue::insert(new DelayEvent(0.35f), INT_MIN + 2);
		Queue::insert(new NumberEvent(189, m_Entity->coordinates + vec2i(m_Entity->dimensions.get(0) / 2, m_Entity->dimensions.get(1) / 2)), INT_MIN + 3);
		Queue::insert(new FlashEvent(&m_Entity->palette, vec3i(255, 255, 255), 0.5f, 0.2f, 0.f), INT_MIN + 4);
	}

	return EVENT_STOP;
}




typedef ListenerQueue<TurnEndListener> TurnEndListenerQueue;
unordered_map<Entity*, TurnEndListenerQueue*> g_TurnEndListeners;


TurnEndListener::TurnEndListener(int priority, Entity* entity) : m_Entity(entity)
{
	TurnEndListenerQueue* queue = nullptr;

	auto iter = g_TurnEndListeners.find(entity);
	if (iter == g_TurnEndListeners.end())
	{
		queue = new TurnEndListenerQueue();
		g_TurnEndListeners.emplace(entity, queue);
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

TurnEndListener::~TurnEndListener()
{
	auto iter = g_TurnEndListeners.find(m_Entity);
	if (iter != g_TurnEndListeners.end())
	{
		iter->second->pop(this);

		if (iter->second->empty())
		{
			g_TurnEndListeners.erase(m_Entity);
		}
	}
}


int TurnEndEvent::start()
{
	auto iter = g_TurnEndListeners.find(m_Entity);
	if (iter != g_TurnEndListeners.end())
	{
		iter->second->trigger();
	}
	return EVENT_STOP;
}