#include <algorithm>
#include <unordered_set>

#include "../include/battle.h"
#include "../include/battleevent.h"
#include "../include/ui.h"

#include <iostream>

using namespace std;
using namespace onion;
using namespace battle;




Palette* g_ClearPalette = nullptr;


Party g_Allies = { nullptr, vector<Entity*>() };
Party g_Enemies = { nullptr, vector<Entity*>() };


Entity::Entity() : palette(vec4i(255, 0, 0, 0), vec4i(0, 255, 0, 0), vec4i(0, 0, 255, 0)) {}

bool Entity::is_ally(Entity* other)
{
	for (auto iter = party->allies.begin(); iter != party->allies.end(); ++iter)
	{
		if (*iter == other) return true;
	}

	return false;
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
		base_defense = load_int(data, "defense");

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

		dimensions = vec2i(image->get_width(), image->get_height());
	}
}

Enemy::~Enemy()
{
	delete image;
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



#define TIMELINE_SPEED		3000

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


EntityEvent::EntityEvent(Entity* entity)
{
	m_Entity = entity;
}

int EntityEvent::start()
{
	// Make sure the entity is still at the front of the timeline
	if (m_Entity->time > 0) return EVENT_STOP;

	// Check if the entity has an Agent to decide what to do
	if (m_Entity->agent)
	{
		primary_queue_insert(new TickEvent(m_Entity));
		primary_queue_insert(new TurnEndEvent(m_Entity));

		// TODO use agent to generate a turn

		return EVENT_STOP;
	}
	else
	{
		// Use keyboard input to decide what to do
		unfreeze();
	}

	return EVENT_CONTINUE;
}

int EntityEvent::update(int frames_passed)
{
	// TODO

	primary_queue_insert(new TickEvent(m_Entity));
	primary_queue_insert(new TurnEndEvent(m_Entity));
	return EVENT_STOP;
}

int EntityEvent::trigger(const KeyEvent& event_data)
{
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


FlashEvent::FlashEvent(SinglePalette* palette, const vec3i& color, float filter, float transition_duration, float hold_duration)
{
	m_Palette = palette;

	m_Red = vec4f(color.get(0) / 255.f, filter * color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Green = vec4f(filter * color.get(0) / 255.f, color.get(1) / 255.f, filter * color.get(2) / 255.f, 0.f);
	m_Blue = vec4f(filter * color.get(0) / 255.f, filter * color.get(1) / 255.f, color.get(2) / 255.f, 0.f);

	m_RiseDuration = round(transition_duration * UpdateEvent::frames_per_second);
	m_FallDuration = m_RiseDuration;
	m_ApexDuration = round(hold_duration * UpdateEvent::frames_per_second);
}

void FlashEvent::change(int frames)
{
	PALETTE_MATRIX m = m_Palette->get_red_palette_matrix();

	m_Palette->set_red_maps_to(vec4f(vec4f(m.get(0, 0), m.get(1, 0), m.get(2, 0), m.get(3, 0)) + (frames * m_Red)));
	m_Palette->set_green_maps_to(vec4f(vec4f(m.get(0, 1), m.get(1, 1), m.get(2, 1), m.get(3, 1)) + (frames * m_Green)));
	m_Palette->set_blue_maps_to(vec4f(vec4f(m.get(0, 2), m.get(1, 2), m.get(2, 2), m.get(3, 2)) + (frames * m_Blue)));
}

int FlashEvent::start()
{
	m_BasePaletteMatrix = m_Palette->get_red_palette_matrix();

	m_Red = vec4f((1.f / m_RiseDuration) * (m_Red - vec4f(m_BasePaletteMatrix.get(0, 0), m_BasePaletteMatrix.get(1, 0), m_BasePaletteMatrix.get(2, 0), m_BasePaletteMatrix.get(3, 0))));
	m_Green = vec4f((1.f / m_RiseDuration) * (m_Green - vec4f(m_BasePaletteMatrix.get(0, 1), m_BasePaletteMatrix.get(1, 1), m_BasePaletteMatrix.get(2, 1), m_BasePaletteMatrix.get(3, 1))));
	m_Blue = vec4f((1.f / m_RiseDuration) * (m_Blue - vec4f(m_BasePaletteMatrix.get(0, 2), m_BasePaletteMatrix.get(1, 2), m_BasePaletteMatrix.get(2, 2), m_BasePaletteMatrix.get(3, 2))));

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




DamageEvent::DamageEvent(Queue* queue, Entity* target, int damage, DamageSource source)
{
	m_Queue = queue;

	m_Target = target;
	m_Damage = damage;
	m_Source = source;
}

Event* DamageEvent::generate_effect()
{
	if (m_Source == NORMAL_DAMAGE)
		return new ShakeEvent(m_Target->coordinates, vec2i(3, 1), 0.25f);
	else if (m_Source == BURN_DAMAGE)
		return new FlashEvent(&m_Target->palette, vec3i(220, 28, 28), 1.f, 0.125f, 0.f);
	else if (m_Source == TOXIN_DAMAGE)
		return new FlashEvent(&m_Target->palette, vec3i(56, 164, 26), 1.f, 0.125f, 0.f);
}

int DamageEvent::start()
{
	if (m_Target->cur_health > 0)
	{
		int dh = m_Damage;

		// Damage the target's Shield first, if the target has any and the damage isn't from Toxin
		if (m_Target->cur_shield > 0 && m_Source != TOXIN_DAMAGE)
		{
			int ds = min(m_Target->cur_shield, m_Damage);

			m_Target->cur_shield -= ds;
			dh -= ds;
		}

		// Damage the target's Health
		m_Target->cur_health -= min(m_Target->cur_health, dh);

		// Queue animations for the damage
		m_Queue->insert(new NumberEvent(m_Damage, m_Target->coordinates + vec2i(m_Target->dimensions.get(0) / 2, m_Target->dimensions.get(1) / 2)), INT_MIN);
		m_Queue->insert(generate_effect(), INT_MIN + 1);
		m_Queue->insert(new DelayEvent(m_Source == NORMAL_DAMAGE ? 0.35f : 0.15f), INT_MIN + 2);

		if (m_Target->cur_health <= 0)
		{
			m_Queue->insert(new DefeatEvent(m_Target), INT_MIN + 3);
		}
	}

	return EVENT_STOP;
}


DefeatEvent::DefeatEvent(Entity* entity)
{
	m_Entity = entity;
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
	g_Allies.allies.resize(3);

	Sprite* ally_bg = Sprite::get_sprite("battle ally bg");
	vec2i ally_dimensions(ally_bg->width, ally_bg->height);
	int ally_x = -(ally_bg->width * g_Allies.allies.size() / 2);

	PALETTE_MATRIX ui_palette = get_ui_palette()->get_red_palette_matrix();

	for (int k = 2; k >= 0; --k)
	{
		Entity* ally = new Entity();
		g_Allies.allies[k] = ally;

		ally->coordinates = vec2i(ally_x, -1);
		ally->dimensions = ally_dimensions;

		ally->cur_health = 999; // TODO
		ally->max_health = 999;

		ally->cur_shield = 999;
		ally->max_shield = 999;

		ally->time = (rand() % (4 * TIMELINE_MAX / 5)) + (TIMELINE_MAX / 5);

		ally->palette.set_red_maps_to(vec4f(ui_palette.get(0, 0), ui_palette.get(1, 0), ui_palette.get(2, 0), ui_palette.get(3, 0)));
		ally->palette.set_green_maps_to(vec4f(ui_palette.get(0, 1), ui_palette.get(1, 1), ui_palette.get(2, 1), ui_palette.get(3, 1)));
		ally->palette.set_blue_maps_to(vec4f(ui_palette.get(0, 2), ui_palette.get(1, 2), ui_palette.get(2, 2), ui_palette.get(3, 2)));

		// debug TODO remove
		ally->burn = 162;
		ally->toxin = 20;

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
		enemy->burn = 162;
		enemy->toxin = 20;

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
		{ m_Sprites[STATUS_OFFENSE], entity->base_offense, 0 },
		{ m_Sprites[STATUS_DEFENSE], entity->base_defense, 0 },
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

	// Draw any animations
	NumberEvent::Animation::display_all();

	// Clean up the transform
	mat_pop();
}