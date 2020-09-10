#include "../include/party.h"

using namespace std;
using namespace overworld;

vector<Ally> g_Party;

vector<Ally>& overworld::get_party()
{
	return g_Party;
}