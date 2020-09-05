#include <unordered_map>
#include <algorithm>
#include "../include/battle.h"
#include "../include/battleevent.h"

#include <iostream>

using namespace std;
using namespace battle;



TickListener::_Map g_TickListeners;

TickListener::TickListener(int priority, Entity* entity) : Listener<>(g_TickListeners, priority, entity) {}


int TickEvent::start()
{
	auto iter = g_TickListeners.find(m_Entity);
	if (iter != g_TickListeners.end())
	{
		iter->second->trigger();
	}

	// Create a temporary queue for the burn and toxin animations
	TemporaryQueue* tempqueue = new TemporaryQueue();

	// Deduct Burn from Shield, then Health
	if (m_Entity->burn > 0)
	{
		tempqueue->insert(new DamageEvent(tempqueue, nullptr, m_Entity, m_Entity->burn--, BURN_DAMAGE), 0);
	}

	// Deduct Toxin from Health
	if (m_Entity->toxin > 0)
	{
		tempqueue->insert(new DamageEvent(tempqueue, nullptr, m_Entity, m_Entity->toxin--, TOXIN_DAMAGE), 1);
	}

	tempqueue->unfreeze();


	// debug TODO remove later
	if (m_Entity->time <= 0)
	{
		m_Entity->time = TIMELINE_MAX * 3 / 5;
		primary_queue_insert(new DelayEvent(1.f));
	}

	return EVENT_STOP;
}




TurnEndListener::_Map g_TurnEndListeners;

TurnEndListener::TurnEndListener(int priority, Entity* entity) : Listener<>(g_TurnEndListeners, priority, entity) {}

int TurnEndEvent::start()
{
	auto iter = g_TurnEndListeners.find(m_Entity);
	if (iter != g_TurnEndListeners.end())
	{
		iter->second->trigger();
	}
	return EVENT_STOP;
}




BeforeTakeDamageListener::_Map g_BeforeTakeDamageListeners;

BeforeTakeDamageListener::BeforeTakeDamageListener(int priority, Entity* entity) : Listener<DamageEvent*>(g_BeforeTakeDamageListeners, priority, entity) {}


DealDamageListener::_Map g_DealDamageListeners;

DealDamageListener::DealDamageListener(int priority, Entity* entity) : Listener<DamageEvent*>(g_DealDamageListeners, priority, entity) {}


AfterTakeDamageListener::_Map g_AfterTakeDamageListeners;

AfterTakeDamageListener::AfterTakeDamageListener(int priority, Entity* entity) : Listener<DamageEvent*>(g_AfterTakeDamageListeners, priority, entity) {}


int DamageEvent::start()
{
	if (target->cur_health > 0)
	{
		// Trigger listeners before an entity takes damage
		Entity* t = nullptr;
		do
		{
			if (!target)
				return EVENT_STOP;

			t = target;
			multiplier = source == NORMAL_DAMAGE ? user->cur_offense - target->cur_defense : 0;

			auto iter1 = g_BeforeTakeDamageListeners.find(target);
			if (iter1 != g_BeforeTakeDamageListeners.end())
			{
				iter1->second->trigger(this);
			}
		}
		while (t != target); // Loops in case the target gets changed

		// Trigger listeners to an entity dealing damage
		auto iter2 = g_DealDamageListeners.find(user);
		if (iter2 != g_DealDamageListeners.end())
		{
			iter2->second->trigger(this);
		}

		// Reduce user Offense and target Defense if they activated
		if (source == NORMAL_DAMAGE)
		{
			if (user->cur_offense > user->base_offense)
				--user->cur_offense;
			else if (user->cur_offense < user->base_offense)
				++user->cur_offense;

			if (target->cur_defense > target->base_defense)
				--target->cur_defense;
			else if (target->cur_defense < target->base_defense)
				++target->cur_defense;
		}

		// Set the damage.
		if (multiplier > 0)
		{
			damage = damage * (10 + multiplier) / 10;
			multiplier = 0;
		}
		else if (multiplier < 0)
		{
			damage = damage * 10 / (10 - multiplier);
			multiplier = 0;
		}

		// Damage the target's Shield first, if the target has any and the damage isn't from Toxin
		int dh = damage;

		if (target->cur_shield > 0 && source != TOXIN_DAMAGE)
		{
			int ds = min(target->cur_shield, damage);

			target->cur_shield -= ds;
			dh -= ds;
		}

		// Damage the target's Health
		target->cur_health -= min(target->cur_health, dh);

		// Queue animations for the damage
		m_Queue->insert(new NumberEvent(damage, target->coordinates + vec2i(target->dimensions.get(0) / 2, target->dimensions.get(1) / 2)), INT_MIN);
		m_Queue->insert(generate_effect(), INT_MIN + 1);
		m_Queue->insert(new DelayEvent(source == NORMAL_DAMAGE ? 0.35f : 0.15f), INT_MIN + 2);

		if (target->cur_health <= 0)
		{
			m_Queue->insert(new DefeatEvent(target), INT_MIN + 3);
		}

		// Trigger listeners to after an entity takes damage
		auto iter3 = g_AfterTakeDamageListeners.find(target);
		if (iter3 != g_AfterTakeDamageListeners.end())
		{
			iter3->second->trigger(this);
		}
	}

	return EVENT_STOP;
}




BeforeStatusInflictedListener::_Map g_BeforeStatusInflictedListeners;

BeforeStatusInflictedListener::BeforeStatusInflictedListener(int priority, Entity* entity) : Listener<InflictStatusEvent*>(g_BeforeStatusInflictedListeners, priority, entity) {}


InflictStatusListener::_Map g_InflictStatusListeners;

InflictStatusListener::InflictStatusListener(int priority, Entity* entity) : Listener<InflictStatusEvent*>(g_InflictStatusListeners, priority, entity) {}


AfterStatusInflictedListener::_Map g_AfterStatusInflictedListeners;

AfterStatusInflictedListener::AfterStatusInflictedListener(int priority, Entity* entity) : Listener<InflictStatusEvent*>(g_AfterStatusInflictedListeners, priority, entity) {}


int InflictStatusEvent::start()
{
	if (target->cur_health > 0)
	{
		// Trigger listeners before an entity is inflicted with a status effect
		Entity* t = nullptr;
		do
		{
			if (!target)
				return EVENT_STOP;

			t = target;

			auto iter1 = g_BeforeStatusInflictedListeners.find(target);
			if (iter1 != g_BeforeStatusInflictedListeners.end())
			{
				iter1->second->trigger(this);
			}
		} while (t != target); // Loops in case the target gets changed.

		// Trigger listeners to an entity inflicting a status effect
		auto iter2 = g_InflictStatusListeners.find(user);
		if (iter2 != g_InflictStatusListeners.end())
		{
			iter2->second->trigger(this);
		}

		// Inflict the status effect
		int* s;
		switch (status)
		{
		case HEALTH_STATUS:
			s = &target->cur_health;
			break;
		case SHIELD_STATUS:
			s = &target->cur_shield;
			break;
		case TIME_STATUS:
			s = &target->time;
			break;
		case OFFENSE_STATUS:
			s = &target->cur_offense;
			break;
		case DEFENSE_STATUS:
			s = &target->cur_defense;
			break;
		case BURN_STATUS:
			s = &target->burn;
			break;
		case TOXIN_STATUS:
			s = &target->toxin;
			break;
		}

		if (status == HEALTH_STATUS && *s <= 0 && value > 0)
		{
			// TODO restore original palette
		}

		*s = max(*s + value, 0);

		if (status == HEALTH_STATUS)
			*s = min(*s, target->max_health);
		else if (status == SHIELD_STATUS)
			*s = min(*s, target->max_shield);
		else if (status == TIME_STATUS)
			*s = min(*s, TIMELINE_MAX);
		
		// Trigger listeners after the entity is inflicted with a status effect
		auto iter3 = g_AfterStatusInflictedListeners.find(target);
		if (iter3 != g_AfterStatusInflictedListeners.end())
		{
			iter3->second->trigger(this);
		}
	}

	return EVENT_STOP;
}