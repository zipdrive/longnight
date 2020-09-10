#include <algorithm>
#include <unordered_set>
#include <regex>

#include "../include/battle.h"
#include "../include/battleevent.h"
#include "../include/battleagent.h"
#include "../include/ui.h"
#include "../include/party.h"

#include <iostream>

using namespace std;
using namespace onion;
using namespace battle;




Palette* g_ClearPalette = nullptr;


Party g_Allies = { nullptr, vector<Entity*>() };
Party g_Enemies = { nullptr, vector<Entity*>() };


Entity::Entity() : palette(vec4i(255, 0, 0, 0), vec4i(0, 255, 0, 0), vec4i(0, 0, 255, 0)) {}

Entity::~Entity()
{
	for (auto iter = usables.begin(); iter != usables.end(); ++iter)
		delete *iter;
}

bool Entity::is_ally(Entity* other)
{
	for (auto iter = party->allies.begin(); iter != party->allies.end(); ++iter)
	{
		if (*iter == other) return true;
	}

	return false;
}


Ally::Ally(overworld::Ally& ally)
{
	// Set the ally's party
	party = &g_Allies;

	// Set the agent
	agent = nullptr;

	// Set up the palette
	SinglePalette* ui_palette = get_ui_palette();
	vec4f ui_color;

	ui_palette->get_red_maps_to(ui_color);
	palette.set_red_maps_to(ui_color);

	ui_palette->get_green_maps_to(ui_color);
	palette.set_green_maps_to(ui_color);

	ui_palette->get_blue_maps_to(ui_color);
	palette.set_blue_maps_to(ui_color);

	// Set values
	cur_health = ally.cur_health;
	max_health = ally.max_health;

	cur_shield = 0;
	max_shield = 999;

	// Set up the usables
	for (auto iter = ally.items.begin(); iter != ally.items.end(); ++iter)
	{
		if (*iter)
			usables.push_back((*iter)->generate());
	}
}

Ally::~Ally()
{
	if (cursor)
		delete cursor;
}

void Ally::defeat()
{
	// Reset all status effects to their baseline
	cur_offense = 0;
	cur_defense = 0;
	burn = 0;
	toxin = 0;

	// Change the palette of the background to show that the ally is incapacitated
	palette.set_red_maps_to(vec4i(239, 195, 195, 0));
	palette.set_green_maps_to(vec4i(213, 110, 110, 0));

	// Check if all allies have been defeated. If so, the player loses the battle.
	for (auto iter = party->allies.begin(); iter != party->allies.end(); ++iter)
	{
		if ((*iter)->cur_health > 0)
			return; // Quits the function if any ally is not incapacitated, so the last section isn't reached
	}

	// TODO game over
}


unordered_map<string, unordered_map<string, string>*> Enemy::m_EnemyData{};
unordered_map<string, SpriteSheet*> Enemy::m_EnemySprites{};

int Enemy::load_int(const unordered_map<string, string>& data, string key)
{
	auto iter = data.find(key);
	if (iter != data.end())
		return stoi(iter->second);
	return 0;
}

string Enemy::load_string(const unordered_map<string, string>& data, string key)
{
	auto iter = data.find(key);
	if (iter != data.end())
		return iter->second;
	return "";
}

Enemy::Enemy(string id)
{
	// Load data about the enemies.
	if (m_EnemyData.empty())
	{
		LoadFile file("res/data/enemies.txt");

		while (file.good())
		{
			unordered_map<string, string>* data = new unordered_map<string, string>();
			string data_id = file.load_data(*data);

			m_EnemyData.emplace(data_id, data);
		}
	}

	// Set the enemy's party
	party = &g_Enemies;

	// Load the data for this particular enemy.
	auto iter = m_EnemyData.find(id);
	if (iter != m_EnemyData.end())
	{
		unordered_map<string, string>& data = *iter->second;

		max_health = load_int(data, "health");
		cur_health = max_health;
		max_shield = load_int(data, "shield");
		cur_shield = max_shield;

		base_offense = load_int(data, "offense");
		cur_offense = 0;
		base_defense = load_int(data, "defense");
		cur_defense = 0;

		string type = load_string(data, "type");
		auto sprite_iter = m_EnemySprites.find(type);
		if (sprite_iter != m_EnemySprites.end())
		{
			// If the sprite sheet has been loaded, set the image
			image = new StaticSpriteGraphic(sprite_iter->second, Sprite::get_sprite("battle enemy " + id), &palette);
		}
		else
		{
			// Load the sprite sheet
			string path = "sprites/enemies/" + type + ".png";
			SpriteSheet* sheet = SpriteSheet::generate(path.c_str());
			m_EnemySprites.emplace(type, sheet);

			image = new StaticSpriteGraphic(sheet, Sprite::get_sprite("battle enemy " + id), &palette);
		}

		if (image)
			dimensions = vec2i(image->get_width(), image->get_height());

		// Load items
		string items = load_string(data, "items");
		regex comma("(.*),\\s*(.*)");
		smatch match;
		while (regex_match(items, match, comma))
		{
			if (overworld::Item* item = overworld::Item::get_item(match[1].str()))
				usables.push_back(item->generate());

			items = match[2].str();
		}
		if (overworld::Item* item = overworld::Item::get_item(items))
			usables.push_back(item->generate());
	}
}

Enemy::~Enemy()
{
	delete image;
}

void Enemy::defeat()
{
	// Enqueue the events that animate the enemy's defeat in reverse (using the priority queue as a stack)
	primary_queue_insert(new EjectEvent(this));
	primary_queue_insert(new DelayEvent(0.1f));
	primary_queue_insert(new RecolorEvent(&palette, vec3i(), 1.f, 0.01f));
	primary_queue_insert(new DelayEvent(0.1f));
	primary_queue_insert(new RecolorEvent(&palette, vec3i(255, 255, 255), 1.f, 0.25f));
}



int Event::start()
{
	return EVENT_CONTINUE;
}

int Event::update(int frames_passed)
{
	return EVENT_STOP;
}


void Queue::get_next_event()
{
	while (!m_Queue.empty())
	{
		if ((m_Current = m_Queue.pop()) != nullptr)
		{
			if (m_Current->start() == EVENT_CONTINUE)
			{
				break;
			}
			else
			{
				delete m_Current;
				m_Current = nullptr;
			}
		}
	}
}

void Queue::__update(int frames_passed)
{
	if (m_Current)
	{
		if (m_Current->update(frames_passed) == EVENT_STOP)
		{
			delete m_Current;
			m_Current = nullptr;

			get_next_event();
		}
	}
	else
	{
		get_next_event();
	}
}

void Queue::insert(Event* event)
{
	int p = 0;
	Event* e = nullptr;
	m_Queue.peek(p, e);

	m_Queue.push(event, p - 1);
}

void Queue::insert(Event* event, int priority)
{
	m_Queue.push(event, priority);
}

void Queue::clear()
{
	vector<Event*> queue;
	m_Queue.dump(queue);

	for (auto iter = queue.begin(); iter != queue.end(); ++iter)
		delete *iter;
}


void TemporaryQueue::__update(int frames_passed)
{
	Queue::__update(frames_passed);

	if (m_Queue.empty() && m_Current == nullptr)
		delete this;
}


Queue* g_PrimaryQueue;

void battle::primary_queue_insert(Event* event)
{
	g_PrimaryQueue->insert(event);
}

void battle::primary_queue_insert(Event* event, int priority)
{
	g_PrimaryQueue->insert(event, priority);
}



#define TIMELINE_SPEED		9000

TimelineEvent::TimelineEvent(const vector<Entity*>& entities)
{
	m_Entities = entities;
}

int TimelineEvent::update(int frames_passed)
{
	int ret = EVENT_CONTINUE;
	int dt = frames_passed * TIMELINE_SPEED / UpdateEvent::frames_per_second;

	for (auto iter = m_Entities.begin(); iter != m_Entities.end(); ++iter)
	{
		if ((*iter)->cur_health > 0) // If the entity is not incapacitated
		{
			int prior = ((*iter)->time - 1) / (TIMELINE_MAX / 5);
			(*iter)->time -= dt;

			if ((*iter)->time <= 0) // If the entity has reached the front of the timeline
			{
				primary_queue_insert(new EntityEvent(*iter), (*iter)->time); // Let it take a turn
				ret = EVENT_STOP;
			}
			else if (((*iter)->time - 1) / (TIMELINE_MAX / 5) != prior) // If the entity has passed one of the tick points
			{
				primary_queue_insert(new TickEvent(*iter), 1); // Trigger any tick listeners
				ret = EVENT_STOP;
			}
		}
		else // If the entity is incapacitated
		{
			(*iter)->time = TIMELINE_MAX; // Push it to the back of the timeline and make it stay there
		}
	}

	if (ret == EVENT_STOP)
	{
		primary_queue_insert(new TimelineEvent(m_Entities), 2);
	}
	return ret;
}


EntityEvent* EntityEvent::m_ActiveEntity{ nullptr };

EntityEvent::TS::TS(Target target_type, int target_index) : target_type(target_type), target_index(target_index) {}

EntityEvent::EntityEvent(Entity* entity)
{
	m_Entity = entity;
}

#define USABLE_ANGLE	0.4833219467f

void EntityEvent::__display() const
{
	// Display the cursors
	int bob = (m_CursorBob * 4 / UpdateEvent::frames_per_second) % 4;
	bob = bob > 2 ? 2 - bob : bob - 2;

	for (auto iter = m_Cursors.begin(); iter != m_Cursors.end(); ++iter)
	{
		mat_push();
		mat_translate(iter->get(0) + bob, iter->get(1) + bob, -0.5f);
		((Ally*)m_Entity)->cursor->display();
		mat_pop();
	}

	// Display the active entity
	mat_push();
	mat_translate(m_Entity->coordinates.get(0) + (m_Entity->dimensions.get(0) / 2), m_Entity->coordinates.get(1) + m_Entity->dimensions.get(1), 0.f);
	
	// Display all usables
	float angle = m_UsableAngle;
	for (auto iter = m_Entity->usables.begin(); iter != m_Entity->usables.end(); ++iter)
	{
		if (Graphic* icon = (*iter)->icon)
		{
			mat_push();
			mat_scale(1.25f, 1.f, 1.f);
			mat_rotatez(angle);
			mat_translate(0.f, 37.f, 0.f);
			mat_rotatez(-angle);
			mat_scale(0.8f, 1.f, 1.f);
			mat_translate(-icon->get_width() / 2, -icon->get_height() / 2, 0.f);
			(*iter)->icon->display();
			mat_pop();
		}

		angle -= USABLE_ANGLE;
	}
	mat_pop();

	// TODO
}

void EntityEvent::display()
{
	if (m_ActiveEntity)
		m_ActiveEntity->__display();
}

void EntityEvent::hover_cursor(Entity* entity)
{
	m_Cursors.push_back(entity->coordinates + vec2i(entity->dimensions.get(0) / 2, entity->dimensions.get(1) / 2));
}

void EntityEvent::reset_cursor()
{
	// Clear the existing cursors
	m_Cursors.clear();

	// Set the new cursors
	if (m_Turn.usable != nullptr && m_Selecting != m_SelectSequence.end())
	{
		switch (m_Selecting->target_type)
		{
		case TARGET_ALL_ENEMIES:
		case TARGET_RANDOM_ENEMY:
		{
			for (auto iter = m_Entity->party->enemies->allies.begin(); iter != m_Entity->party->enemies->allies.end(); ++iter)
				hover_cursor(*iter);
			break;
		}
		case TARGET_SINGLE_ENEMY:
		{
			hover_cursor(m_Entity->party->enemies->allies[m_SelectedTarget]);
			break;
		}
		case TARGET_ALL_ALLIES:
		{
			for (auto iter = m_Entity->party->allies.begin(); iter != m_Entity->party->allies.end(); ++iter)
				hover_cursor(*iter);
			break;
		}
		case TARGET_SINGLE_ALLY:
		{
			hover_cursor(m_Entity->party->allies[m_SelectedTarget]);
			break;
		}
		case TARGET_SELF:
		{
			hover_cursor(m_Entity);
			break;
		}
		}
	}
}

int EntityEvent::start()
{
	// Make sure the entity is still at the front of the timeline
	if (m_Entity->time > 0) return EVENT_STOP;

	// Check if the entity has an Agent to decide what to do
	if (m_Entity->agent)
	{
		// We're enqueueing everything in the reverse order that it will happen (so basically, using our priority queue as a stack)

		// Queue the listener triggering events
		primary_queue_insert(new TickEvent(m_Entity));
		primary_queue_insert(new TurnEndEvent(m_Entity));

		// Queue the entity's action
		m_Entity->agent->decide(m_Turn);
		m_Turn.enqueue();

		// Queue making the entity flash to indicate who is acting
		float filter = dynamic_cast<Enemy*>(m_Entity) == nullptr ? 1.f : 0.5f;
		primary_queue_insert(new FlashEvent(&m_Entity->palette, vec3i(255, 255, 255), filter, 0.1f, 0.f));
		primary_queue_insert(new FlashEvent(&m_Entity->palette, vec3i(255, 255, 255), filter, 0.1f, 0.f));

		return EVENT_STOP;
	}
	else
	{
		// Set as the active entity event
		m_ActiveEntity = this;

		// Set the initial data for the turn
		m_Turn.usable = nullptr;
		m_Turn.user = m_Entity;
		m_SelectedUsable = 0;
		m_SelectedTarget = 0;

		m_UsableAngle = 0.f;
		m_TargetUsableAngle = 0.f;

		// Use keyboard input to decide what to do
		unfreeze();
	}

	return EVENT_CONTINUE;
}

#define USABLE_ANGLE_SPEED (USABLE_ANGLE * 4.f)

int EntityEvent::update(int frames_passed)
{
	if (m_UsableAngle < m_TargetUsableAngle)
	{
		m_UsableAngle += frames_passed * USABLE_ANGLE_SPEED / UpdateEvent::frames_per_second;
		
		if (m_UsableAngle >= m_TargetUsableAngle) m_UsableAngle = m_TargetUsableAngle;
	}
	else if (m_UsableAngle > m_TargetUsableAngle)
	{
		m_UsableAngle -= frames_passed * USABLE_ANGLE_SPEED / UpdateEvent::frames_per_second;

		if (m_UsableAngle <= m_TargetUsableAngle) m_UsableAngle = m_TargetUsableAngle;
	}

	m_CursorBob += frames_passed;

	if (m_Turn.usable != nullptr && m_Selecting == m_SelectSequence.end())
	{
		// Queue triggering listeners
		primary_queue_insert(new TickEvent(m_Entity));
		primary_queue_insert(new TurnEndEvent(m_Entity));

		// Queue the selected usable with the selected targets
		m_Turn.enqueue();

		// Unset as the active entity event
		m_ActiveEntity = nullptr;

		return EVENT_STOP;
	}

	return EVENT_CONTINUE;
}

int EntityEvent::trigger(const KeyEvent& event_data)
{
	if (event_data.pressed) // If the key was pressed and not released/held down
	{
		if (m_Turn.usable) // Selecting targets
		{
			switch (event_data.control)
			{
			case KEY_LEFT:
			case KEY_RIGHT:
			{
				int r = event_data.control == KEY_LEFT ? -1 : 1;
				int s = m_Selecting->target_type == TARGET_SINGLE_ENEMY ? m_Entity->party->enemies->allies.size() : m_Entity->party->allies.size();

				if (m_Selecting->target_type == TARGET_SINGLE_ENEMY || m_Selecting->target_type == TARGET_SINGLE_ALLY)
				{
					m_SelectedTarget = (m_SelectedTarget + r + s) % s;
					reset_cursor();
				}

				break;
			}
			case KEY_SELECT:
			{
				if (m_Selecting->target_index >= 0)
				{
					if (m_Selecting->target_type == TARGET_SINGLE_ENEMY)
					{
						m_Turn.targets[m_Selecting->target_index] = m_Entity->party->enemies->allies[m_SelectedTarget];
					}
					else
					{
						m_Turn.targets[m_Selecting->target_index] = m_Entity->party->allies[m_SelectedTarget];
					}
				}

				++m_Selecting;
				reset_cursor();
				break;
			}
			case KEY_CANCEL:
			{
				if (m_Selecting == m_SelectSequence.begin())
				{
					m_Turn.usable = nullptr;
				}
				else
				{
					--m_Selecting;
				}

				reset_cursor();
				break;
			}
			}
		}
		else // Selecting what usable to use
		{
			switch (event_data.control)
			{
			case KEY_LEFT:
			{
				m_SelectedUsable = max(0, m_SelectedUsable - 1);
				m_TargetUsableAngle = USABLE_ANGLE * m_SelectedUsable;
				break;
			}
			case KEY_RIGHT:
			{
				int s = m_Entity->usables.size() - 1;
				m_SelectedUsable = min(m_SelectedUsable + 1, s);
				m_TargetUsableAngle = USABLE_ANGLE * m_SelectedUsable;
				break;
			}
			case KEY_SELECT:
			{
				// Set which usable to use
				m_Turn.usable = m_Entity->usables[m_SelectedUsable];

				// Set the sequence of targets to select
				m_SelectSequence.clear();
				m_Turn.targets.clear();
				for (int k = 0; k < m_Turn.usable->targets.size(); ++k)
				{
					Target t = m_Turn.usable->targets[k].target;

					if (t == TARGET_SINGLE_ALLY || t == TARGET_SINGLE_ENEMY)
					{
						m_Turn.targets.push_back(nullptr);
						m_SelectSequence.push_back(EntityEvent::TS(t, m_Turn.targets.size() - 1));
					}
					else
					{
						if (t == TARGET_RANDOM_ENEMY)
						{
							vector<Entity*>& opts = m_Entity->party->enemies->allies;
							m_Turn.targets.push_back(opts[rand() % opts.size()]);
						}
						
						m_SelectSequence.push_back(EntityEvent::TS(t, -1));
					}
				}

				auto iter = m_SelectSequence.begin();
				while (iter != m_SelectSequence.end())
				{
					if (iter->target_type != TARGET_SINGLE_ALLY && iter->target_type != TARGET_SINGLE_ENEMY && iter != m_SelectSequence.begin())
					{
						if (iter == m_SelectSequence.begin())
						{
							m_SelectSequence.erase(iter);
							iter = m_SelectSequence.begin();
						}
						else
						{
							m_SelectSequence.erase(iter--);
							++iter;
						}
					}
					else
					{
						++iter;
					}
				}

				// Hover the cursor over the currently selected target(s)
				m_Selecting = m_SelectSequence.begin();
				m_SelectedTarget = 0;
				reset_cursor();

				break;
			}
			}
		}
	}

	return EVENT_STOP;
}




TickEvent::TickEvent(Entity* entity)
{
	m_Entity = entity;
}


TurnEndEvent::TurnEndEvent(Entity* entity)
{
	m_Entity = entity;
}



DelayEvent::DelayEvent(float duration)
{
	m_RemainingDuration = round(duration * UpdateEvent::frames_per_second);
}

int DelayEvent::update(int frames_passed)
{
	m_RemainingDuration -= frames_passed;
	return m_RemainingDuration > 0 ? EVENT_CONTINUE : EVENT_STOP;
}


ShakeEvent::ShakeEvent(vec2i& coordinates, const vec2i& amplitude, float duration) : m_Coordinates(coordinates)
{
	m_Amplitude = amplitude;
	m_RemainingDuration = round(duration * UpdateEvent::frames_per_second);
}

int ShakeEvent::start()
{
	m_Center = m_Coordinates;
	return EVENT_CONTINUE;
}

int ShakeEvent::update(int frames_passed)
{
	m_RemainingDuration -= frames_passed;

	if (m_RemainingDuration > 0)
	{
		m_Coordinates = m_Center + vec2i((rand() % (2 * m_Amplitude.get(0)) + 1) - m_Amplitude.get(0), (rand() % (2 * m_Amplitude.get(1)) + 1) - m_Amplitude.get(1));
		return EVENT_CONTINUE;
	}
	else
	{
		m_Coordinates = m_Center;
		return EVENT_STOP;
	}
}


RecolorEvent::RecolorEvent(SinglePalette* palette, const vec3i& color, float filter, float transition_duration)
{
	m_Palette = palette;

	m_Red = vec4f(color.get(0) / 255.f, filter * color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Green = vec4f(filter * color.get(0) / 255.f, color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Blue = vec4f(filter * color.get(0) / 255.f, filter * color.get(1) / 255.f, color.get(2) / 255.f, 0.f);

	m_Duration = ceil(transition_duration * UpdateEvent::frames_per_second);
}

void RecolorEvent::change(int frames)
{
	vec4f prior;

	m_Palette->get_red_maps_to(prior);
	m_Palette->set_red_maps_to(vec4f(prior + (frames * m_Red)));

	m_Palette->get_green_maps_to(prior);
	m_Palette->set_green_maps_to(vec4f(prior + (frames * m_Green)));

	m_Palette->get_blue_maps_to(prior);
	m_Palette->set_blue_maps_to(vec4f(prior + (frames * m_Blue)));
}

int RecolorEvent::start()
{
	vec4f prior;

	m_Palette->get_red_maps_to(prior);
	m_Red = vec4f((1.f / m_Duration) * (m_Red - prior));

	m_Palette->get_green_maps_to(prior);
	m_Green = vec4f((1.f / m_Duration) * (m_Green - prior));

	m_Palette->get_blue_maps_to(prior);
	m_Blue = vec4f((1.f / m_Duration) * (m_Blue - prior));

	return EVENT_CONTINUE;
}

int RecolorEvent::update(int frames_passed)
{
	int frames = min(m_Duration, frames_passed);

	m_Duration -= frames;
	change(frames);

	return m_Duration <= 0 ? EVENT_STOP : EVENT_CONTINUE;
}


FlashEvent::FlashEvent(SinglePalette* palette, const vec3i& color, float filter, float transition_duration, float hold_duration)
{
	m_Palette = palette;

	m_Red = vec4f(color.get(0) / 255.f, filter * color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Green = vec4f(filter * color.get(0) / 255.f, color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Blue = vec4f(filter * color.get(0) / 255.f, filter * color.get(1) / 255.f, color.get(2) / 255.f, 0.f);

	m_RiseDuration = ceil(transition_duration * UpdateEvent::frames_per_second);
	m_FallDuration = m_RiseDuration;
	m_ApexDuration = round(hold_duration * UpdateEvent::frames_per_second);
}

void FlashEvent::change(int frames)
{
	vec4f prior;

	m_Palette->get_red_maps_to(prior);
	m_Palette->set_red_maps_to(vec4f(prior + (frames * m_Red)));

	m_Palette->get_green_maps_to(prior);
	m_Palette->set_green_maps_to(vec4f(prior + (frames * m_Green)));

	m_Palette->get_blue_maps_to(prior);
	m_Palette->set_blue_maps_to(vec4f(prior + (frames * m_Blue)));
}

int FlashEvent::start()
{
	m_BasePaletteMatrix = m_Palette->get_red_palette_matrix();

	vec4f prior;

	m_Palette->get_red_maps_to(prior);
	m_Red = vec4f((1.f / m_RiseDuration) * (m_Red - prior));

	m_Palette->get_green_maps_to(prior);
	m_Green = vec4f((1.f / m_RiseDuration) * (m_Green - prior));

	m_Palette->get_blue_maps_to(prior);
	m_Blue = vec4f((1.f / m_RiseDuration) * (m_Blue - prior));

	return EVENT_CONTINUE;
}

int FlashEvent::update(int frames_passed)
{
	if (m_RiseDuration > 0)
	{
		// Rise the flash
		int frames = min(m_RiseDuration, frames_passed);
		m_RiseDuration -= frames;
		m_ApexDuration -= frames_passed - frames;

		change(frames);
	}
	else if (m_ApexDuration > 0)
	{
		// Hold the flash
		int frames = min(m_ApexDuration, frames_passed);
		m_ApexDuration -= frames;
		m_FallDuration -= frames_passed - frames;

		change(frames - frames_passed);
	}
	else if (m_FallDuration > frames_passed)
	{
		// Fall the flash
		m_FallDuration -= frames_passed;

		change(-frames_passed);
	}
	else
	{
		m_Palette->set_red_maps_to(vec4f(m_BasePaletteMatrix.get(0, 0), m_BasePaletteMatrix.get(1, 0), m_BasePaletteMatrix.get(2, 0), m_BasePaletteMatrix.get(3, 0)));
		m_Palette->set_green_maps_to(vec4f(m_BasePaletteMatrix.get(0, 1), m_BasePaletteMatrix.get(1, 1), m_BasePaletteMatrix.get(2, 1), m_BasePaletteMatrix.get(3, 1)));
		m_Palette->set_blue_maps_to(vec4f(m_BasePaletteMatrix.get(0, 2), m_BasePaletteMatrix.get(1, 2), m_BasePaletteMatrix.get(2, 2), m_BasePaletteMatrix.get(3, 2)));

		return EVENT_STOP;
	}

	return EVENT_CONTINUE;
}


NumberEvent::NumberEvent(int number, const vec2i& start)
{
	m_Animation = new Animation(number, start);
}

int NumberEvent::start()
{
	m_Animation->unfreeze();
	return EVENT_STOP;
}


list<NumberEvent::Animation*> NumberEvent::Animation::m_ActiveAnimations{};

NumberEvent::Animation::Animation(int number, const vec2i& start)
{
	m_Number = number;
	m_Coordinates = vec2f(start.get(0), start.get(1));
}

void NumberEvent::Animation::display_all()
{
	for (auto iter = m_ActiveAnimations.begin(); iter != m_ActiveAnimations.end(); ++iter)
	{
		mat_translate(0.f, 0.f, -0.01f);
		(*iter)->display();
	}
}

#define NUMBER_RISE_DURATION	0.25f
#define NUMBER_STILL_DURATION	0.5f

#define NUMBER_RISE_SPEED		40.f

void NumberEvent::Animation::__update(int frames_passed)
{
	m_Duration -= frames_passed;

	if (m_Duration > NUMBER_STILL_DURATION * UpdateEvent::frames_per_second) // Is rising
	{
		m_Coordinates.set(1, 0, m_Coordinates.get(1) + (NUMBER_RISE_SPEED * frames_passed / UpdateEvent::frames_per_second));
	}
	else if (m_Duration <= 0) // Has finished animating
	{
		for (auto iter = m_ActiveAnimations.begin(); iter != m_ActiveAnimations.end(); ++iter)
		{
			if (*iter == this)
			{
				m_ActiveAnimations.erase(iter);
				break;
			}
		}

		delete this;
	}
}

void NumberEvent::Animation::unfreeze()
{
	UpdateListener::unfreeze();
	m_ActiveAnimations.push_back(this);

	m_Duration = round((NUMBER_RISE_DURATION + NUMBER_STILL_DURATION) * UpdateEvent::frames_per_second);
}

void NumberEvent::Animation::display() const
{
	mat_push();
	mat_translate(m_Coordinates.get(0), m_Coordinates.get(1), 0.f);
	battle::State::display_number(m_Number, false, TEXT_CENTER, get_ui_palette());
	mat_pop();
}




DamageEvent::DamageEvent(Queue* queue, Entity* user, Entity* target, int damage, DamageSource source) : user(user), target(target), source(source), damage(damage)
{
	m_Queue = queue;
}

Event* DamageEvent::generate_effect()
{
	if (source == NORMAL_DAMAGE)
		return new ShakeEvent(target->coordinates, vec2i(3, 1), 0.25f);
	else if (source == BURN_DAMAGE)
		return new FlashEvent(&target->palette, vec3i(220, 28, 28), 1.f, 0.125f, 0.f);
	else if (source == TOXIN_DAMAGE)
		return new FlashEvent(&target->palette, vec3i(56, 164, 26), 1.f, 0.125f, 0.f);
}


InflictStatusEvent::InflictStatusEvent(Entity* user, Entity* target, Status status, int value) : user(user), target(target), status(status), value(value) {}




DefeatEvent::DefeatEvent(Entity* entity)
{
	m_Entity = entity;
}

int DefeatEvent::start()
{
	m_Entity->defeat();
	return EVENT_STOP;
}

EjectEvent::EjectEvent(Entity* entity)
{
	m_Entity = entity;
}

int EjectEvent::start()
{
	vector<Entity*>& ally_vec = m_Entity->party->allies;
	for (auto iter = ally_vec.begin(); iter != ally_vec.end(); ++iter)
	{
		if (*iter == m_Entity)
		{
			ally_vec.erase(iter);
			break;
		}
	}
	delete m_Entity;

	// Check if all enemies have been defeated. If so, end the battle.
	if (ally_vec.empty())
	{
		// TODO player wins
	}

	return EVENT_STOP;
}




EffectSequence::EffectSequence(vector<Effect*>& effects) : effects(effects) {}

EffectSequence::~EffectSequence()
{
	for (auto iter = effects.begin(); iter != effects.end(); ++iter)
		delete *iter;
}

void EffectSequence::enqueue(Entity* user, Entity* target) const
{
	for (auto iter = effects.rbegin(); iter != effects.rend(); ++iter)
	{
		primary_queue_insert((*iter)->generate_event(user, target));
	}
}


TargetSequence::TargetSelection::TargetSelection() {}

TargetSequence::TargetSelection::TargetSelection(Target target, shared_ptr<EffectSequence> effects) : target(target), effects(effects) {}

TargetSequence::TargetSequence(TargetSequence* other) : targets(other->targets)
{
	animation = other->animation;
}

TargetSequence::TargetSequence(Effect* animation, Target target, vector<Effect*>& target_effects, int speed) : animation(animation)
{
	targets.resize(2);

	targets[0].target = target;
	targets[0].effects = shared_ptr<EffectSequence>(new EffectSequence(target_effects));

	targets[1].target = TARGET_SELF;
	targets[1].effects = shared_ptr<EffectSequence>(new EffectSequence(vector<Effect*>({ new InflictStatusEffect(TIME_STATUS, TIMELINE_MAX * (6 - speed) / 5) })));
}

void TargetSequence::push_back(Target target, Effect* effect)
{
	push_back(target, vector<Effect*>({ effect }));
}

void TargetSequence::push_back(Target target, vector<Effect*>& effects)
{
	targets.emplace_back(target, shared_ptr<EffectSequence>(new EffectSequence(effects)));
}

void TargetSequence::enqueue(Entity* user, vector<Entity*>& single_targets) const
{
	auto targ_iter = single_targets.rbegin();

	for (auto iter = targets.rbegin(); iter != targets.rend(); ++iter)
	{
		// Figure out who the targets for this sequence are
		vector<Entity*> t;
		switch (iter->target)
		{
		case TARGET_SINGLE_ALLY:
		case TARGET_SINGLE_ENEMY:
		case TARGET_RANDOM_ENEMY:
			if (targ_iter != single_targets.rend())
			{
				t = vector<Entity*>({ *targ_iter++ });
			}
			else
			{
				// TODO error
			}
			break;
		case TARGET_ALL_ALLIES:
			t = user->party->allies;
			break;
		case TARGET_ALL_ENEMIES:
			t = user->party->enemies->allies;
			break;
		case TARGET_SELF:
			t = vector<Entity*>({ user });
			break;
		}

		// Queue up the events
		for (auto t_iter = t.rbegin(); t_iter != t.rend(); ++t_iter)
		{
			iter->effects->enqueue(user, *t_iter);
		}
	}
}


Usable::Usable(onion::Graphic* icon, Effect* animation, Target target, vector<Effect*>& target_effects, int speed) : TargetSequence(animation, target, target_effects, speed), icon(icon) {}


DamageEffect::DamageEffect(int damage) : damage(damage) {}

Event* DamageEffect::generate_event(Entity* user, Entity* target) const
{
	return new DamageEvent(g_PrimaryQueue, user, target, damage, NORMAL_DAMAGE);
}


InflictStatusEffect::InflictStatusEffect(Status status, int value) : status(status), value(value) {}

Event* InflictStatusEffect::generate_event(Entity* user, Entity* target) const
{
	return new InflictStatusEvent(user, target, status, value);
}



Agent::Agent(Entity* self)
{
	m_Self = self;
}




SpriteSheet* battle::State::m_SpriteSheet{ nullptr };

SPRITE_KEY battle::State::m_Sprites[BATTLE_SPRITE_COUNT]{};

#define TIMELINE_LEFT_CAP			0
#define TIMELINE_RIGHT_CAP			1
#define TIMELINE_ICON				2

#define ALLY_BG						3
#define ALLY_SHIELD					4
#define ALLY_HEALTH					5
#define ALLY_BAR_IN					31

#define COLOR_SHIELD				6
#define COLOR_HEALTH				7

#define SMALL_NUMBERS				8
#define LARGE_NUMBERS				18

#define ENEMY_BAR_CAP				28
#define ENEMY_BAR_MID				29
#define ENEMY_BAR_IN				30

#define STATUS_OFFENSE				32
#define STATUS_DEFENSE				33
#define STATUS_BURN					34
#define STATUS_TOXIN				35

#define CURSORS						36

battle::State::State(const vector<string>& enemies) 
{
	// Load the battle UI sprite sheet
	if (!m_SpriteSheet)
	{
		m_SpriteSheet = SpriteSheet::generate("sprites/battle.png");

		m_Sprites[TIMELINE_LEFT_CAP] = Sprite::get_sprite("battle timeline left cap")->key;
		m_Sprites[TIMELINE_RIGHT_CAP] = Sprite::get_sprite("battle timeline right cap")->key;

		m_Sprites[TIMELINE_ICON] = Sprite::get_sprite("battle timeline icon bg")->key;

		m_Sprites[ALLY_BG] = Sprite::get_sprite("battle ally bg")->key;
		m_Sprites[ALLY_SHIELD] = Sprite::get_sprite("battle ally shield")->key;
		m_Sprites[ALLY_HEALTH] = Sprite::get_sprite("battle ally health")->key;
		m_Sprites[ALLY_BAR_IN] = Sprite::get_sprite("battle ally bar in")->key;

		m_Sprites[ENEMY_BAR_CAP] = Sprite::get_sprite("battle enemy bar cap")->key;
		m_Sprites[ENEMY_BAR_MID] = Sprite::get_sprite("battle enemy bar mid")->key;
		m_Sprites[ENEMY_BAR_IN] = Sprite::get_sprite("battle enemy bar in")->key;

		m_Sprites[COLOR_SHIELD] = Sprite::get_sprite("battle color shield")->key;
		m_Sprites[COLOR_HEALTH] = Sprite::get_sprite("battle color health")->key;

		m_Sprites[STATUS_OFFENSE] = Sprite::get_sprite("battle status offense")->key;
		m_Sprites[STATUS_DEFENSE] = Sprite::get_sprite("battle status defense")->key;
		m_Sprites[STATUS_BURN] = Sprite::get_sprite("battle status burn")->key;
		m_Sprites[STATUS_TOXIN] = Sprite::get_sprite("battle status toxin")->key;

		m_Sprites[CURSORS] = Sprite::get_sprite("battle cursor linh")->key;
		m_Sprites[CURSORS + 1] = Sprite::get_sprite("battle cursor mosi")->key;
		m_Sprites[CURSORS + 2] = Sprite::get_sprite("battle cursor jude")->key;

		for (int k = 9; k >= 0; --k)
		{
			m_Sprites[SMALL_NUMBERS + k] = Sprite::get_sprite("battle small number " + to_string(k))->key;
			m_Sprites[LARGE_NUMBERS + k] = Sprite::get_sprite("battle large number " + to_string(k))->key;
		}

		g_ClearPalette = new SinglePalette(vec4i(255, 0, 0, 0), vec4i(0, 255, 0, 0), vec4i(0, 0, 255, 0));
	}

	// Create the background graphic
	Application* app = get_application_settings();
	m_Background = SolidColorGraphic::generate(218, 183, 183, 255, app->width, app->height - 80);

	// Set data for allies
	vector<overworld::Ally>& party = overworld::get_party();

	g_Allies.allies.resize(party.size());

	Sprite* ally_bg = Sprite::get_sprite("battle ally bg");
	vec2i ally_dimensions(ally_bg->width, ally_bg->height);
	int ally_x = -(ally_bg->width * g_Allies.allies.size() / 2);

	PALETTE_MATRIX ui_palette = get_ui_palette()->get_red_palette_matrix();

	for (int k = party.size() - 1; k >= 0; --k)
	{
		Ally* ally = new Ally(party[k]);
		g_Allies.allies[k] = ally;

		string name = k == 0 ? "linh" : (k == 1 ? "mosi" : "jude");
		ally->cursor = new StaticSpriteGraphic(m_SpriteSheet, Sprite::get_sprite("battle cursor " + name), g_ClearPalette);

		ally->coordinates = vec2i(ally_x, -1);
		ally->dimensions = ally_dimensions;

		ally->time = (rand() % (4 * TIMELINE_MAX / 5)) + (TIMELINE_MAX / 5);

		ally_x += ally_bg->width;
	}

	g_Allies.enemies = &g_Enemies;

	// Set data for enemies
#define ENEMY_PADDING	80
	int enemy_width = ENEMY_PADDING * (enemies.size() - 1);

	g_Enemies.allies.resize(enemies.size());
	for (int k = g_Enemies.allies.size() - 1; k >= 0; --k)
	{
		Enemy* enemy = new Enemy(enemies[k]);
		g_Enemies.allies[k] = enemy;

		enemy_width += enemy->image->get_width();

		// debug TODO remove
		enemy->agent = new RandomAgent(enemy);

		enemy->time = (rand() % (4 * TIMELINE_MAX / 5)) + (TIMELINE_MAX / 5);
	}

	enemy_width = -enemy_width / 2;
	for (auto iter = g_Enemies.allies.begin(); iter != g_Enemies.allies.end(); ++iter)
	{
		Enemy* enemy = (Enemy*)*iter;
		enemy->coordinates = vec2i(enemy_width, (app->height - enemy->image->get_height()) / 2);
		enemy_width += enemy->image->get_width() + ENEMY_PADDING;
	}

	g_Enemies.enemies = &g_Allies;

	// Start up the queues
	g_PrimaryQueue = new Queue();

	vector<Entity*> all_entities;
	for (auto iter = g_Allies.allies.begin(); iter != g_Allies.allies.end(); ++iter)
		all_entities.push_back(*iter);
	for (auto iter = g_Enemies.allies.begin(); iter != g_Enemies.allies.end(); ++iter)
		all_entities.push_back(*iter);
	g_PrimaryQueue->insert(new TimelineEvent(all_entities), 2);
	g_PrimaryQueue->unfreeze();
}

battle::State::~State()
{
	// Delete the allies and enemies.
	for (auto iter = g_Allies.allies.begin(); iter != g_Allies.allies.end(); ++iter)
		delete *iter;
	for (auto iter = g_Enemies.allies.begin(); iter != g_Enemies.allies.end(); ++iter)
		delete *iter;

	// Delete the primary queue.
	delete g_PrimaryQueue;
}

void battle::State::display_number(int number, bool small, TextAlignment alignment, Palette* palette, int leading_zeroes)
{
	mat_push();
	switch (alignment)
	{
	case TEXT_RIGHT:
	{
		while (leading_zeroes-- > 0 || number > 0)
		{
			int r = number % 10;

			if (small)
			{
				mat_translate(r == 1 ? -4.f : -5.f, 0.f, 0.f);
				m_SpriteSheet->display(m_Sprites[SMALL_NUMBERS + r], palette);
			}
			else
			{
				mat_translate(-9.f, 0.f, 0.f);
				m_SpriteSheet->display(m_Sprites[LARGE_NUMBERS + r], palette);
			}

			number = number / 10;
		}
		break;
	}
	case TEXT_LEFT:
	case TEXT_CENTER:
	{
		vector<int> numbers;

		while (leading_zeroes-- > 0 || number > 0)
		{
			numbers.push_back(number % 10);
			number = number / 10;
		}

		if (!numbers.empty())
		{
			if (alignment == TEXT_LEFT)
			{
				for (int k = numbers.size() - 1; k >= 0; --k)
				{
					if (small)
					{
						int r = numbers[k];
						m_SpriteSheet->display(m_Sprites[SMALL_NUMBERS + r], palette);
						mat_translate(r == 1 ? 4.f : 5.f, 0.f, 0.f);
					}
					else
					{
						m_SpriteSheet->display(m_Sprites[LARGE_NUMBERS + numbers[k]], palette);
						mat_translate(9.f, 0.f, 0.f);
					}
				}
			}
			else
			{
				int half_index = numbers.size() / 2;

				if (numbers.size() % 2 == 1)
				{
					mat_translate(small ? 2.f : 5.f, 0.f, 0.f);
				}

				mat_push();
				for (int k = half_index - 1; k >= 0; --k)
				{
					if (small)
					{
						int r = numbers[k];
						m_SpriteSheet->display(m_Sprites[SMALL_NUMBERS + r], palette);
						mat_translate(r == 1 ? 4.f : 5.f, 0.f, 0.f);
					}
					else
					{
						m_SpriteSheet->display(m_Sprites[LARGE_NUMBERS + numbers[k]], palette);
						mat_translate(9.f, 0.f, 0.f);
					}
				}
				mat_pop();
				for (int k = half_index; k < numbers.size(); ++k)
				{
					if (small)
					{
						int r = numbers[k];
						mat_translate(r == 1 ? -4.f : -5.f, 0.f, 0.f);
						m_SpriteSheet->display(m_Sprites[SMALL_NUMBERS + r], palette);
					}
					else
					{
						mat_translate(-9.f, 0.f, 0.f);
						m_SpriteSheet->display(m_Sprites[LARGE_NUMBERS + numbers[k]], palette);
					}
				}
			}
		}

		break;
	}
	}
	mat_pop();
}

void battle::State::display_status(Entity* entity)
{
	Palette* ui_palette = get_ui_palette();

	struct StatusEffectIcon
	{
		SPRITE_KEY key;
		int value;
		int width;
	};

	int width = 0;
	vector<StatusEffectIcon> icons = {
		{ m_Sprites[STATUS_OFFENSE], entity->cur_offense, 0 },
		{ m_Sprites[STATUS_DEFENSE], entity->cur_defense, 0 },
		{ m_Sprites[STATUS_BURN], entity->burn, 0 },
		{ m_Sprites[STATUS_TOXIN], entity->toxin, 0 }
	};

	for (int k = icons.size() - 1; k >= 0; --k)
	{
		if (icons[k].value <= 0)
		{
			icons.erase(icons.begin() + k);
		}
		else
		{
			icons[k].width = icons[k].value > 99 ? 22 : 17;
			width += icons[k].width;
		}
	}


	if (!icons.empty())
	{
		int x = (width / 2) + (2 * (icons.size() - 1));

		mat_push();
		mat_translate(-x, 0.f, 0.f);

		for (auto iter = icons.begin(); iter != icons.end(); ++iter)
		{
			m_SpriteSheet->display(iter->key, g_ClearPalette);
			mat_translate(7.f, -3.f, -0.001f);
			display_number(iter->value, true, TEXT_LEFT, ui_palette);
			mat_translate(iter->width - 3, 3.f, 0.f);
		}

		mat_pop();
	}
}

void battle::State::display() const
{
	static Application*& app = get_application_settings();
	static Palette* ui_palette = get_ui_palette();

	// Set up an orthographic transform
	mat_push();
	mat_translate(0.f, -1.f, 0.f);
	mat_scale(2.f / app->width, 2.f / app->height, 1.f);

	// Draw the background
	mat_push();
	mat_translate(-0.5f * app->width, 40.f, 0.01f);
	m_Background->display();
	mat_pop();

	// Draw the timeline
	mat_push();
	mat_translate(-218.f, app->height - 22, 0.f);
	m_SpriteSheet->display(m_Sprites[TIMELINE_LEFT_CAP], ui_palette);
	mat_translate(11.f, 2.f, 0.f);
	for (int k = 5; k > 0; --k)
	{
		m_SpriteSheet->display(m_Sprites[TIMELINE_RIGHT_CAP], ui_palette);
		mat_translate(85.f, 0.f, 0.f);
	}

	// Draw the individual icons on the timeline
	mat_translate(-443.f, -9.f, -0.101f);
	for (auto iter = g_Allies.allies.begin(); iter != g_Allies.allies.end(); ++iter)
	{
		int t = (*iter)->time;

		mat_push();
		mat_translate(428 * t / TIMELINE_MAX, 0.f, 0.1f * t / TIMELINE_MAX);
		m_SpriteSheet->display(m_Sprites[TIMELINE_ICON], ui_palette);
		mat_pop();
	}
	for (auto iter = g_Enemies.allies.begin(); iter != g_Enemies.allies.end(); ++iter)
	{
		int t = (*iter)->time;

		mat_push();
		mat_translate(428 * t / TIMELINE_MAX, 0.f, 0.1f * t / TIMELINE_MAX);
		m_SpriteSheet->display(m_Sprites[TIMELINE_ICON], ui_palette);
		mat_pop();
	}
	mat_pop();

	// Draw the allies
	for (int k = 0; k < 3; ++k)
	{
		Entity* ally = g_Allies.allies[k];

		mat_push();
		mat_translate(ally->coordinates.get(0), ally->coordinates.get(1), 0.f);
		m_SpriteSheet->display(m_Sprites[ALLY_BG], &ally->palette);

		// Display status effects
		mat_push();
		mat_translate(ally->dimensions.get(0) / 2, 9.f, -0.001f);
		display_status(ally);
		mat_pop();

		// Display health values
		int cur = ally->cur_health;

		mat_push();
		mat_translate(22.f, 30.f, -0.001f);
		mat_scale(89.f * cur / ally->max_health, 5.f, 1.f);
		m_SpriteSheet->display(m_Sprites[COLOR_HEALTH], g_ClearPalette);
		mat_pop();
		if (cur < ally->max_health && cur > 0)
		{
			mat_push();
			mat_translate(22.f + floor(89.f * cur / ally->max_health), 30.f, -0.0015f);
			m_SpriteSheet->display(m_Sprites[ALLY_BAR_IN], ui_palette);
			mat_pop();
		}

		mat_translate(13.f, 33.f, -0.002f);
		m_SpriteSheet->display(m_Sprites[ALLY_HEALTH], ui_palette);

		mat_push();
		mat_translate(104.f, -7.f, -0.001f);
		display_number(cur, true, TEXT_RIGHT, ui_palette, 3);
		mat_pop();

		// Display shield values
		cur = ally->cur_shield;

		mat_push();
		mat_translate(9.f, 15.f, 0.001f);
		mat_scale(ceil(89.f * cur / ally->max_shield), 5.f, 1.f);
		m_SpriteSheet->display(m_Sprites[COLOR_SHIELD], g_ClearPalette);
		mat_pop();
		if (cur < ally->max_shield && cur > 0)
		{
			mat_push();
			mat_translate(9.f + floor(89.f * cur / ally->max_shield), 15.f, 0.0005f);
			m_SpriteSheet->display(m_Sprites[ALLY_BAR_IN], ui_palette);
			mat_pop();
		}

		mat_translate(0.f, 18.f, 0.f);
		m_SpriteSheet->display(m_Sprites[ALLY_SHIELD], ui_palette);

		mat_push();
		mat_translate(104.f, -7.f, -0.001f);
		display_number(cur, true, TEXT_RIGHT, ui_palette, 3);
		mat_pop();

		mat_pop();
	}

	// Draw the enemies
	for (auto iter = g_Enemies.allies.begin(); iter != g_Enemies.allies.end(); ++iter)
	{
		Enemy* enemy = (Enemy*)*iter;

		// Display the enemy sprite
		mat_push();
		mat_translate(enemy->coordinates.get(0), enemy->coordinates.get(1), 0.f);
		enemy->image->display();

		// Display the enemy's Health
		int health_width = max(enemy->max_health / 25, 1);
		mat_translate(0.5f * enemy->dimensions.get(0), enemy->dimensions.get(1) + 11, 0.f);

		mat_push();
		mat_translate(-(health_width / 2), 0.f, 0.f);
		mat_push();
		mat_scale(health_width, 1.f, 1.f);
		m_SpriteSheet->display(m_Sprites[ENEMY_BAR_MID], ui_palette);
		mat_translate(0.f, 1.f, -0.001f);
		mat_scale((float)enemy->cur_health / enemy->max_health, 2.f, 1.f);
		m_SpriteSheet->display(m_Sprites[COLOR_HEALTH], g_ClearPalette);
		mat_pop();
		if (enemy->cur_health < enemy->max_health && enemy->cur_health > 0)
		{
			mat_push();
			mat_translate(health_width * enemy->cur_health / enemy->max_health, 1.f, -0.002f);
			m_SpriteSheet->display(m_Sprites[ENEMY_BAR_IN], ui_palette);
			mat_pop();
		}

		mat_translate(-1.f, 0.f, 0.f);
		m_SpriteSheet->display(m_Sprites[ENEMY_BAR_CAP], ui_palette);
		mat_translate(1 + health_width, 0.f, 0.f);
		m_SpriteSheet->display(m_Sprites[ENEMY_BAR_CAP], ui_palette);
		mat_pop();

		// Display the enemy's Shield
		mat_translate(0.f, 3.f, 0.f);

		if (enemy->max_shield > 0)
		{
			int shield_width = max(enemy->max_shield / 25, 1);

			mat_push();
			mat_translate(-(shield_width / 2), 0.f, 0.f);
			mat_push();
			mat_scale(shield_width, 1.f, 1.f);
			m_SpriteSheet->display(m_Sprites[ENEMY_BAR_MID], ui_palette);
			mat_translate(0.f, 1.f, -0.001f);
			mat_scale((float)enemy->cur_shield / enemy->max_shield, 2.f, 1.f);
			m_SpriteSheet->display(m_Sprites[COLOR_SHIELD], g_ClearPalette);
			mat_pop();
			if (enemy->cur_shield < enemy->max_shield && enemy->cur_shield > 0)
			{
				mat_push();
				mat_translate(shield_width * enemy->cur_shield / enemy->max_shield, 1.f, -0.002f);
				m_SpriteSheet->display(m_Sprites[ENEMY_BAR_IN], ui_palette);
				mat_pop();
			}

			mat_translate(-1.f, 0.f, 0.f);
			m_SpriteSheet->display(m_Sprites[ENEMY_BAR_CAP], ui_palette);
			mat_translate(1 + shield_width, 0.f, 0.f);
			m_SpriteSheet->display(m_Sprites[ENEMY_BAR_CAP], ui_palette);
			mat_pop();
		}

		// Display the enemy's status effects
		mat_translate(0.f, 12.f, 0.f);
		battle::State::display_status(enemy);

		mat_pop();
	}

	// Display any number animations
	NumberEvent::Animation::display_all();

	// Display any cursors and shit
	EntityEvent::display();

	// Clean up the transform
	mat_pop();
}