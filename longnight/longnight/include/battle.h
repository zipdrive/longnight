#pragma once
#include <onion.h>
#include "queue.h"


#define TIMELINE_MAX		60000


namespace battle
{

	class Agent;

	struct Party;

	// An ally or enemy.
	struct Entity
	{
		// The entity's current Health.
		int cur_health;

		// The entity's maximum Health.
		int max_health;


		// The entity's current Shield.
		int cur_shield;

		// The entity's maximum Shield.
		int max_shield;


		// The entity's base Offense, to which it will eventually return.
		int base_offense = 0;

		// The entity's current Offense.
		int offense = 0;

		// The entity's base Defense, to which it will eventually return.
		int base_defense = 0;

		// The entity's current Defense.
		int defense = 0;


		// The entity's current Burn. Decreases Shield, then Health at every tick by the current Burn value.
		int burn = 0;

		// The entity's current Toxin. Decreases Health at every tick by the current Toxin value.
		int toxin = 0;


		// The entity's position on the timeline. 0 is the front, 60000 is the back. Each tick is at a multiple of 12000.
		int time;


		// The entity's allies and enemies.
		Party* party;

		// The Agent controlling the entity.
		Agent* agent;


		// The coordinates of the entity on the screen.
		vec2i coordinates;

		// The dimensions of the entity on the screen.
		vec2i dimensions;

		// The palette to display with.
		onion::SinglePalette palette;


		/// <summary>Initializes the palette for the entity.</summary>
		Entity();
		
		/// <summary>Frees the memory of the entity.</summary>
		virtual ~Entity() {}
		
		/// <summary>Checks if an entity is an ally or an enemy of this one.</summary>
		/// <param name="other">Another entity.</param>
		/// <returns>True if the entity is an ally, false if it is an enemy.</returns>
		bool is_ally(Entity* other);
	};

	// A collection of entities who are allied.
	struct Party
	{
		// The entities that need to be defeated.
		Party* enemies;

		// The entities aligned with the party.
		std::vector<Entity*> allies;
	};

	
	// An enemy.
	struct Enemy : public Entity
	{
	private:
		// The data about all enemies.
		static std::unordered_map<std::string, std::unordered_map<std::string, std::string>*> m_EnemyData;

		// The sprite sheets for each enemy type.
		static std::unordered_map<std::string, onion::SpriteSheet*> m_EnemySprites;

		/// <summary>Loads an integer from the data about an enemy.</summary>
		/// <param name="data">The data about a particular enemy.</param>
		int load_int(const std::unordered_map<std::string, std::string>& data, std::string key);

		/// <summary>Loads a string from the data about an enemy.</summary>
		/// <param name="data">The data about a particular enemy.</param>
		std::string load_string(const std::unordered_map<std::string, std::string>& data, std::string key);

	public:
		// The enemy sprite.
		onion::Graphic* image;

		/// <summary>Creates an enemy out of the data with the given ID.</summary>
		/// <param name="id">The ID of the enemy.</param>
		Enemy(std::string id);

		/// <summary>Frees the memory of the enemy graphic.</summary>
		~Enemy();
	};



	// Something that needs to happen one at a time in a certain order.
	class Event
	{
	public:
		/// <summary>A function called when the Event begins animating.</summary>
		/// <returns>EVENT_STOP if the Event is finished, EVENT_CONTINUE otherwise.</returns>
		virtual int start();

		/// <summary>A function called on every update pass, to update the Event's animation.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		/// <returns>EVENT_STOP if the Event is finished, EVENT_CONTINUE otherwise.</returns>
		virtual int update(int frames_passed);
	};


	// A queue of events.
	class Queue : public onion::UpdateListener
	{
	protected:
		// The queue of events.
		PriorityQueue<Event*> m_Queue;

		// The current event being animated.
		Event* m_Current = nullptr;

		/// <summary>Sets m_Current to the next event in the queue that isn't null and doesn't immediately stop.</summary>
		void get_next_event();

		/// <summary>Updates the queue and the current Event.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		virtual void __update(int frames_passed);

	public:
		/// <summary>Adds an Event to the front of the queue.</summary>
		/// <param name="event">The Event to add.</param>
		void insert(Event* event);

		/// <summary>Adds an Event to the queue.</summary>
		/// <param name="event">The Event to add.</param>
		/// <param name="priority">The priority for the Event. Low numbers activate earlier.</param>
		void insert(Event* event, int priority);

		/// <summary>Clears the queue.</summary>
		void clear();
	};

	// A queue that destroys itself once it's been emptied.
	class TemporaryQueue : public Queue
	{
	protected:
		/// <summary>Updates the queue and the current Event. Self-destructs if the queue has been exhausted.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		void __update(int frames_passed);
	};


	/// <summary>Adds an event to the front of the primary queue.</summary>
	/// <param name="event">The event to add.</param>
	void primary_queue_insert(Event* event);

	/// <summary>Adds an event to the primary queue.</summary>
	/// <param name="event">The event to add.</param>
	/// <param name="priority">The priority for the event. Low numbers activate earlier.</param>
	void primary_queue_insert(Event* event, int priority);



	// An event that advances all active entities forward on the timeline.
	class TimelineEvent : public Event
	{
	private:
		// All entities to advance on the timeline.
		std::vector<Entity*> m_Entities;

	public:
		/// <summary>Creates a timeline event.</summary>
		/// <param name="entities">An array of all entities to advance on the timeline.</param>
		TimelineEvent(const std::vector<Entity*>& entities);

		/// <summary>Advances all entities forward on the timeline.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		/// <returns>EVENT_STOP if one or more entities reach the front of the timeline, EVENT_CONTINUE otherwise.</returns>
		int update(int frames_passed);
	};

	// An event that lets an entity take a turn.
	class EntityEvent : public Event, public onion::KeyboardListener
	{
	private:
		// The entity taking a turn.
		Entity* m_Entity;

	public:
		/// <summary>Creates an event where an entity takes a turn.</summary>
		/// <param name="entity">The entity taking a turn.</param>
		EntityEvent(Entity* entity);

		/// <summary>Uses the entity's Agent to decide what it should do.</summary>
		/// <returns>EVENT_STOP if the entity isn't controlled by the player, EVENT_CONTINUE if it is.</returns>
		int start();

		/// <summary>Lets the player select what the entity should do.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		/// <returns>EVENT_STOP if the player has selected an action, EVENT_CONTINUE if not.</returns>
		int update(int frames_passed);

		/// <summary>Responds to a key input.</summary>
		/// <param name="event_data">The data for the key event.</param>
		/// <returns>EVENT_STOP.</returns>
		int trigger(const onion::KeyEvent& event_data);
	};


	// An event that triggers all listeners for when an entity passes a tick.
	class TickEvent : public Event
	{
	private:
		// The entity that passed a tick.
		Entity* m_Entity;

	public:
		/// <summary>Creates a tick event.</summary>
		/// <param name="entity">The entity that passed a tick.</param>
		TickEvent(Entity* entity);

		/// <summary>Triggers all tick listeners for the entity.</summary>
		/// <returns>EVENT_STOP.</returns>
		int start();
	};

	// An event that triggers all listeners for when an entity ends their turn.
	class TurnEndEvent : public Event
	{
	private:
		// The entity ending their turn.
		Entity* m_Entity;

	public:
		/// <summary>Creates a turn end event.</summary>
		/// <param name="entity">The entity that ended their turn.</param>
		TurnEndEvent(Entity* entity);

		/// <summary>Triggers all tick listeners for the entity.</summary>
		/// <returns>EVENT_STOP.</returns>
		int start();
	};



	class DelayEvent : public Event
	{
	private:
		// The remaining duration of the delay, in frames.
		int m_RemainingDuration;

	public:
		/// <summary>Creates an event that delays the next event in the queue.</summary>
		/// <param name="duration">The duration of the delay, in seconds.</param>
		DelayEvent(float duration);

		int update(int frames_passed);
	};
	
	class ShakeEvent : public Event
	{
	private:
		// A reference to the coordinates to shake.
		vec2i& m_Coordinates;

		// The center of the shaking.
		vec2i m_Center;

		// The horizontal and vertical amplitude of the shaking.
		vec2i m_Amplitude;

		// The remaining duration of the shaking, in frames.
		int m_RemainingDuration;

	public:
		/// <summary>Creates an event that shakes a set of coordinates.</summary>
		/// <param name="coordinates">The coordinates to shake.</param>
		/// <param name="amplitude">The amplitude of the shaking, defined separately for both the horizontal and vertical directions.</param>
		/// <param name="duration">The duration of the shaking, in seconds.</param>
		ShakeEvent(vec2i& coordinates, const vec2i& amplitude, float duration);

		int start();

		int update(int frames_passed);
	};

	class FlashEvent : public Event
	{
	private:
		// The palette to flash a different color.
		onion::SinglePalette* m_Palette;

		// The intial palette matrix.
		PALETTE_MATRIX m_BasePaletteMatrix;

		// The change per frame in the color RED maps to.
		vec4f m_Red;

		// The change per frame in the color GREEN maps to.
		vec4f m_Green;

		// The change per frame in the color BLUE maps to.
		vec4f m_Blue;

		// How long is left in the flash rising.
		int m_RiseDuration;

		// How long is left at the height of the flash.
		int m_ApexDuration;

		// How long is left for the flash to return to normal.
		int m_FallDuration;

		/// <summary>Shifts the palette mappings.</summary>
		/// <param name="frames">The number of frames to shift the palette mappings by. Positive numbers shift towards the flash color, negative numbers shift away from it.</param>
		void change(int frames);

	public:
		/// <summary></summary>
		/// <param name="palette">The palette to flash a different color.</param>
		/// <param name="color">The color of the flash.</param>
		/// <param name="filter">0 if the flash filters the sprite through a color, 1 to render it as a solid block of color.</param>
		/// <param name="transition_duration">The duration of the flash's transition, in seconds.</param>
		/// <param name="hold_duration">The duration to hold the flash, in seconds.</param>
		FlashEvent(onion::SinglePalette* palette, const vec3i& color, float filter, float transition_duration, float hold_duration);

		/// <summary>Initializes the flash parameters.</summary>
		/// <returns>EVENT_CONTINUE.</returns>
		int start();

		/// <summary>Updates the flash.</summary>
		/// <param name="frames_passed">The number of frames that have passed since the last update.</param>
		/// <returns>EVENT_STOP if the flash is done, EVENT_CONTINUE otherwise.</returns>
		int update(int frames_passed);
	};

	class NumberEvent : public Event
	{
	public:
		class Animation : public onion::UpdateListener
		{
		protected:
			// All active animations.
			static std::list<Animation*> m_ActiveAnimations;

			// The number to display.
			int m_Number;

			// The coordinates of where to display the number.
			vec2f m_Coordinates;

			// The duration that the animation will remain unfrozen, in frames.
			int m_Duration;

			/// <summary>Updates the animation, and deletes itself when finished.</summary>
			void __update(int frames_passed);

		public:
			/// <summary>Displays all active animated numbers.</summary>
			static void display_all();

			/// <summary>Creates the animation.</summary>
			/// <param name="number">The number to display.</param>
			/// <param name="start">The initial coordinates of where the number is displayed on screen.</param>
			/// <param name="delay">The delay before the animation activates, in seconds.</param>
			Animation(int number, const vec2i& start);

			/// <summary>Starts the animation.</summary>
			void unfreeze();

			/// <summary>Displays the number.</summary>
			void display() const;
		};

	private:
		// The animation for the number.
		Animation* m_Animation;

	public:
		/// <summary>Creates a number that rises.</summary>
		/// <param name="number">The number to display.</param>
		/// <param name="start">The initial coordinates of where the number is displayed on screen.</param>
		/// <param name="delay">The delay before the number first appears, in seconds.</param>
		NumberEvent(int number, const vec2i& start);

		/// <summary>Makes the number appear and start animating independently from the queue.</summary>
		/// <returns>EVENT_STOP.</returns>
		int start();
	};


	
	enum DamageSource
	{
		NORMAL_DAMAGE,
		BURN_DAMAGE,
		TOXIN_DAMAGE
	};
	
	// An event where an entity takes damage.
	class DamageEvent : public Event
	{
	private:
		// The queue that this event has been put in.
		Queue* m_Queue;

		/// <summary>Generates an event that animates the target taking damage.</summary>
		/// <returns>A pointer to an Event allocated with new.</returns>
		Event* generate_effect();

	public:
		// The source of the damage.
		DamageSource source;

		// The entity dealing damage.
		Entity* user;

		// The entity taking damage.
		Entity* target;

		// The base damage dealt.
		int damage;

		// The multiplier for the damage.
		int multiplier;

		/// <summary>Constructs an event where an entity takes damage.</summary>
		/// <param name="queue">The queue to put the animation events into.</param>
		/// <param name="user">The entity dealing damage.</param>
		/// <param name="target">The entity taking damage.</param>
		/// <param name="damage">The amount of base damage to deal.</param>
		/// <param name="source">The type of damage.</param>
		DamageEvent(Queue* queue, Entity* user, Entity* target, int damage, DamageSource source);

		/// <summary>Deals damage to the target entity.</summary>
		/// <returns>EVENT_STOP.</returns>
		int start();
	};


	// A kind of status effect.
	enum Status
	{
		OFFENSE_STATUS,
		DEFENSE_STATUS,
		BURN_STATUS,
		TOXIN_STATUS
	};

	// An event that inflicts a status effect.
	class InflictStatusEvent : public Event
	{
	public:
		// The entity inflicting Burn.
		Entity* user;

		// The entity getting inflicted with Burn.
		Entity* target;

		// The status effect.
		Status status;

		// The amount of status to inflict.
		int value;

		/// <summary>Constructs an event where an entity is inflicted with a status effect.</summary>
		/// <param name="user">The entity inflicting the status effect.</param>
		/// <param name="target">The entity being inflicted with the status effect.</param>
		/// <param name="status">The status effect to inflict.</param>
		/// <param name="value">The amount of status effect to inflict.</param>
		InflictStatusEvent(Entity* user, Entity* target, Status status, int value);

		/// <summary>Inflicts the status effect on the target entity.</summary>
		/// <returns>EVENT_STOP.</returns>
		int start();
	};

	// An event where an entity is reduced to 0 Health.
	class DefeatEvent : public Event
	{
	private:
		// The entity that was reduced to 0 Health.
		Entity* m_Entity;

	public:
		DefeatEvent(Entity* entity);
	};




	class Effect
	{
	public:
		/// <summary>Generates an event.</summary>
		virtual Event* generate_event() = 0;
	};



	
	
	struct Agent
	{
	protected:
		// The entity that the agent represents.
		Entity* m_Self;

	public:
		/// <summary>Constructs an agent.</summary>
		/// <param name="self">The entity that the agent represents.</param>
		Agent(Entity* self);
	};


	
	
	class State : public onion::State
	{
	private:
		// The sheet of sprites for the battle UI.
		static onion::SpriteSheet* m_SpriteSheet;

#define BATTLE_SPRITE_COUNT 36

		// An array of sprites for the battle UI.
		static SPRITE_KEY m_Sprites[BATTLE_SPRITE_COUNT];

		// The background for the battle.
		onion::Graphic* m_Background;

	public:
		/// <summary>Displays a number.</summary>
		/// <param name="number">The number to display.</param>
		/// <param name="small">True if the number should be small, false if it should be large.</param>
		/// <param name="alignment">The alignment of the number, relative to the origin.</param>
		/// <param name="leading_zeroes">The number of leading zeroes.</param>
		/// <param name="palette">The palette to use when displaying the numbers.</param>
		static void display_number(int number, bool small, onion::TextAlignment alignment, onion::Palette* palette, int leading_zeroes = 1);

		/// <summary>Displays the status effects of an entity.</summary>
		/// <param name="entity">The entity to display the status effects of.</param>
		static void display_status(Entity* entity);

		/// <summary>Creates a state for a battle.</summary>
		/// <param name="enemies">A list of enemy IDs.</param>
		State(const std::vector<std::string>& enemies);

		/// <summary>Cleans up after a battle state.</summary>
		~State();

		/// <summary>Displays the state.</summary>
		void display() const;
	};


}