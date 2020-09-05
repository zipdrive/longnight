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

	Turn turn = { &m_BerserkAction, m_Self, { enemies[rand() % enemies.size()] } };
	turn.enqueue();
}