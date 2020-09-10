#include "../include/battleagent.h"

using namespace std;
using namespace battle;


RandomAgent::RandomAgent(Entity* self) : Agent(self) {}

void RandomAgent::decide(Turn& turn)
{
	turn.user = m_Self;

	vector<Usable*>& opts = m_Self->usables;
	turn.usable = nullptr;
	do { turn.usable = opts[rand() % opts.size()]; } while (!turn.usable);

	for (auto iter = turn.usable->targets.begin(); iter != turn.usable->targets.end(); ++iter)
	{
		if (iter->target == TARGET_SINGLE_ENEMY || iter->target == TARGET_RANDOM_ENEMY)
		{
			vector<Entity*>& enemies = m_Self->party->enemies->allies;
			Entity* target = nullptr;
			do
			{
				target = enemies[rand() % enemies.size()];
			} 
			while (target->cur_health == 0);

			turn.targets.push_back(target);
		}
		else if (iter->target == TARGET_SINGLE_ALLY)
		{
			vector<Entity*>& allies = m_Self->party->allies;
			turn.targets.push_back(allies[rand() % allies.size()]);
		}
	}
}