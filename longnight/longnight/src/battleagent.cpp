#include "../include/battleagent.h"

using namespace std;
using namespace battle;


Action BerserkAgent::m_BerserkAction
{
	TARGET_RANDOM_ENEMY,
	nullptr,
	vector<Effect*>({ new DamageEffect(BASE_DAMAGE * 5 / 3) }),
	vector<Effect*>({ new InflictStatusEffect(TIME_STATUS, TIMELINE_MAX) })
};

BerserkAgent::BerserkAgent(Entity* self) : Agent(self) {}

void BerserkAgent::enqueue()
{
	vector<Entity*>& enemies = m_Self->party->enemies->allies;
	Entity* target = nullptr;
	do 
	{
		target = enemies[rand() % enemies.size()];
	} 
	while (target->cur_health == 0);

	Turn turn = { &m_BerserkAction, m_Self, { target } };
	turn.enqueue();
}