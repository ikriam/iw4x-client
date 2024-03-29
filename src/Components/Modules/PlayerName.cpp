#include "STDInclude.hpp"

namespace Components
{
	std::string PlayerName::PlayerNames[18];

	int PlayerName::GetClientName(int /*localClientNum*/, int index, char *buf, int size)
	{
		if (index < 0 || index >= 18) return 0;
		return strncpy_s(buf, size, PlayerName::PlayerNames[index].data(), PlayerName::PlayerNames[index].size()) == 0;
	}

	PlayerName::PlayerName()
	{
#if(0) // Disabled for now
		{
			for (int i = 0; i < ARRAYSIZE(PlayerName::PlayerNames); ++i)
			{
				PlayerName::PlayerNames[i] = "mumu";
			}

			Utils::Hook(Game::CL_GetClientName, PlayerName::GetClientName, HOOK_JUMP).install()->quick();
		}
#endif

		//const char* clanname = "ZOB";
		//Utils::Hook::Set<const char*>(0x497656, clanname);
		//Utils::Hook::Set<const char*>(0x497679, clanname);
	}

	PlayerName::~PlayerName()
	{
		for (int i = 0; i < ARRAYSIZE(PlayerName::PlayerNames); ++i)
		{
			PlayerName::PlayerNames[i].clear();
		}
	}
}
