#pragma once
#include <onions/graphics.h>
#include "battle.h"

namespace overworld
{


	struct Item
	{
	private:
		// 
		static std::unordered_map<std::string, Item*> m_Items;

		// 
		static onion::SpriteSheet* m_SpriteSheet;

	public:
		/// <summary>Retrieves the item with the given ID.</summary>
		/// <returns>The item with the given ID.</returns>
		static Item* get_item(std::string id);

		// The icon for the item.
		onion::Graphic* icon;

		// The speed of the item, with 1 being the lowest speed and 5 being the highest.
		int speed;

		/// <summary>Virtual deconstructor.</summary>
		virtual ~Item();

		/// <summary>Generates a usable for battle.</summary>
		virtual battle::Usable* generate() = 0;
	};


	// An item used on enemies.
	struct OffenseItem : public Item
	{
		// Which enemies the weapon targets.
		enum
		{
			SINGLE,
			RANDOM,
			ALL
		} target;


		// The percentage of base damage dealt.
		int damage;


		// How much Offense is deducted.
		int offense;

		// How much Defense is deducted.
		int defense;


		// The amount of Burn inflicted.
		int burn;

		// The amount of Toxin inflicted.
		int toxin;


		// The amount of Stun inflicted.
		int stun;


		OffenseItem(const std::unordered_map<std::string, std::string>& data);

		/// <summary>Generates a usable for battle.</summary>
		battle::Usable* generate();
	};

	// An item used on allies.
	struct SupportItem : public Item
	{
		// What allies the item can target.
		enum
		{
			SELF,
			SINGLE,
			ALL
		} target;


		// How much Health the item grants.
		int health;

		// How much Shield the item grants.
		int shield;


		// How much Offense is granted.
		int offense;

		// How much Defense is granted.
		int defense;


		SupportItem(const std::unordered_map<std::string, std::string>& data);

		/// <summary>Generates a usable for battle.</summary>
		battle::Usable* generate();
	};


}