#include "../include/ui.h"
#include "../include/item.h"

using namespace std;
using namespace onion;
using namespace overworld;

unordered_map<string, Item*> Item::m_Items{};

SpriteSheet* Item::m_SpriteSheet{ nullptr };

string get_string_from_key(const unordered_map<string, string>& d, string key)
{
	auto iter = d.find(key);
	return iter == d.end() ? "" : iter->second;
}

int get_int_from_key(const unordered_map<string, string>& d, string key)
{
	string v = get_string_from_key(d, key);
	return v.empty() ? 0 : stoi(v);
}

Item* Item::get_item(string id)
{
	if (!m_SpriteSheet)
	{
		// Load item sprites
		m_SpriteSheet = SpriteSheet::generate("sprites/items.png");
	}

	if (m_Items.empty())
	{
		// Load items
		LoadFile file("res/data/items.txt");

		while (file.good())
		{
			unordered_map<string, string> data;
			string data_id = file.load_data(data);

			Item* item = nullptr;

			// Base the item on what type it is
			string type = get_string_from_key(data, "type");
			if (type.compare("offense") == 0)
			{
				// Load a weapon
				item = new OffenseItem(data);
			}
			else if (type.compare("support") == 0)
			{
				// Load a support item
				item = new SupportItem(data);
			}

			if (item)
			{
				// Load the speed
				item->speed = get_int_from_key(data, "speed");

				// Load the item sprite
				item->icon = new StaticSpriteGraphic(m_SpriteSheet, Sprite::get_sprite("item " + get_string_from_key(data, "icon")), get_clear_palette());

				// Set the data for the given ID
				m_Items.emplace(data_id, item);
			}
		}
	}

	// Find the specific item
	auto iter = m_Items.find(id);
	if (iter != m_Items.end())
		return iter->second;
	return nullptr;
}

Item::~Item()
{
	delete icon;
}


using namespace battle;

OffenseItem::OffenseItem(const std::unordered_map<std::string, std::string>& data)
{
	string t = get_string_from_key(data, "target");
	if (t.compare("all") == 0)
		target = OffenseItem::ALL;
	else if (t.compare("random") == 0)
		target = OffenseItem::RANDOM;
	else
		target = OffenseItem::SINGLE;

	damage = get_int_from_key(data, "damage");

	offense = get_int_from_key(data, "offense");
	defense = get_int_from_key(data, "defense");

	burn = get_int_from_key(data, "burn");
	toxin = get_int_from_key(data, "toxin");
}

Usable* OffenseItem::generate()
{
	vector<Effect*> effects;
	
	if (damage > 0)
		effects.push_back(new DamageEffect(BASE_DAMAGE * damage / 100));

	if (offense > 0)
		effects.push_back(new InflictStatusEffect(OFFENSE_STATUS, -offense));
	if (defense > 0)
		effects.push_back(new InflictStatusEffect(DEFENSE_STATUS, -defense));

	if (burn > 0)
		effects.push_back(new InflictStatusEffect(BURN_STATUS, burn));
	if (toxin > 0)
		effects.push_back(new InflictStatusEffect(TOXIN_STATUS, toxin));

	if (stun > 0)
		effects.push_back(new InflictStatusEffect(TIME_STATUS, stun));

	return new Usable(icon, nullptr, target == ALL ? TARGET_ALL_ENEMIES : (target == RANDOM ? TARGET_RANDOM_ENEMY : TARGET_SINGLE_ENEMY), effects, speed);
}


SupportItem::SupportItem(const std::unordered_map<std::string, std::string>& data)
{
	string t = get_string_from_key(data, "target");
	if (t.compare("all") == 0)
		target = SupportItem::ALL;
	else if (t.compare("self") == 0)
		target = SupportItem::SELF;
	else
		target = SupportItem::SINGLE;

	health = get_int_from_key(data, "health");
	shield = get_int_from_key(data, "shield");

	offense = get_int_from_key(data, "offense");
	defense = get_int_from_key(data, "defense");
}

Usable* SupportItem::generate()
{
	vector<Effect*> effects;

	if (health > 0)
		effects.push_back(new InflictStatusEffect(HEALTH_STATUS, health));
	if (shield > 0)
		effects.push_back(new InflictStatusEffect(SHIELD_STATUS, shield));

	if (offense > 0)
		effects.push_back(new InflictStatusEffect(OFFENSE_STATUS, offense));
	if (defense > 0)
		effects.push_back(new InflictStatusEffect(DEFENSE_STATUS, defense));

	return new Usable(icon, nullptr, target == ALL ? TARGET_ALL_ALLIES : (target == SELF ? TARGET_SELF : TARGET_SINGLE_ALLY), effects, speed);
}