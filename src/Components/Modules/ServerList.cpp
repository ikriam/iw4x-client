#include "STDInclude.hpp"

namespace Components
{
	bool ServerList::SortAsc = true;
	int ServerList::SortKey = ServerList::Column::Ping;

	unsigned int ServerList::CurrentServer = 0;
	ServerList::Container ServerList::RefreshContainer;

	std::vector<ServerList::ServerInfo> ServerList::OnlineList;
	std::vector<ServerList::ServerInfo> ServerList::OfflineList;
	std::vector<ServerList::ServerInfo> ServerList::FavouriteList;

	std::vector<unsigned int> ServerList::VisibleList;

	std::vector<ServerList::ServerInfo>* ServerList::GetList()
	{
		if (ServerList::IsOnlineList())
		{
			return &ServerList::OnlineList;
		}
		else if (ServerList::IsOfflineList())
		{
			return &ServerList::OfflineList;
		}
		else if (ServerList::IsFavouriteList())
		{
			return &ServerList::FavouriteList;
		}

		return nullptr;
	}

	bool ServerList::IsFavouriteList()
	{
		return (Dvar::Var("ui_netSource").Get<int>() == 2);
	}

	bool ServerList::IsOfflineList()
	{
		return (Dvar::Var("ui_netSource").Get<int>() == 0);
	}

	bool ServerList::IsOnlineList()
	{
		return (Dvar::Var("ui_netSource").Get<int>() == 1);
	}

	unsigned int ServerList::GetServerCount()
	{
		return ServerList::VisibleList.size();
	}

	const char* ServerList::GetServerText(unsigned int index, int column)
	{
		ServerList::ServerInfo* info = ServerList::GetServer(index);

		if (info)
		{
			return ServerList::GetServerText(info, column);
		}

		return "";
	}

	const char* ServerList::GetServerText(ServerList::ServerInfo* server, int column)
	{
		if (!server) return "";

		switch (column)
		{
			case Column::Password:
			{
				return (server->Password ? "X" : "");
			}

			case Column::Matchtype:
			{
				return ((server->MatchType == 1) ? "P" : "M");
			}

			case Column::Hostname:
			{
				return server->Hostname.data();
			}

			case Column::Mapname:
			{
				if (server->SVRunning)
				{
					return Game::UI_LocalizeMapName(server->Mapname.data());
				}
				else
				{
					return Utils::String::VA("^3%s", Game::UI_LocalizeMapName(server->Mapname.data()));
				}
			}

			case Column::Players:
			{
				return Utils::String::VA("%i (%i)", server->Clients, server->MaxClients);
			}

			case Column::Gametype:
			{
				return Game::UI_LocalizeGameType(server->Gametype.data());
			}

			case Column::Mod:
			{
				if (server->Mod != "")
				{
					return (server->Mod.data() + 5);
				}

				return "";
			}

			case Column::Ping:
			{
				return Utils::String::VA("%i", server->Ping);
			}
		}

		return "";
	}

	void ServerList::SelectServer(unsigned int index)
	{
		ServerList::CurrentServer = index;

		ServerList::ServerInfo* info = ServerList::GetCurrentServer();

		if (info)
		{
			Dvar::Var("ui_serverSelected").Set(true);
			Dvar::Var("ui_serverSelectedMap").Set(info->Mapname);
		}
		else
		{
			Dvar::Var("ui_serverSelected").Set(false);
		}
	}

	void ServerList::UpdateVisibleList()
	{
		auto list = ServerList::GetList();
		if (!list) return;

		std::vector<ServerList::ServerInfo> tempList(*list);

		if (tempList.empty())
		{
			ServerList::Refresh();
		}
		else
		{
			list->clear();

			ServerList::RefreshContainer.Mutex.lock();

			ServerList::RefreshContainer.SendCount = 0;
			ServerList::RefreshContainer.SentCount = 0;

			for (auto server : tempList)
			{
				ServerList::InsertRequest(server.Addr, false);
			}

			ServerList::RefreshContainer.Mutex.unlock();
		}
	}

	void ServerList::RefreshVisibleList()
	{
		Dvar::Var("ui_serverSelected").Set(false);

		ServerList::VisibleList.clear();

		auto list = ServerList::GetList();
		if (!list) return;

		// Refresh entirely, if there is no entry in the list
		if (list->empty())
		{
			ServerList::Refresh();
			return;
		}

		bool ui_browserShowFull     = Dvar::Var("ui_browserShowFull").Get<bool>();
		bool ui_browserShowEmpty    = Dvar::Var("ui_browserShowEmpty").Get<bool>();
		int ui_browserShowHardcore  = Dvar::Var("ui_browserKillcam").Get<int>();
		int ui_browserShowPassword  = Dvar::Var("ui_browserShowPassword").Get<int>();
		int ui_browserMod           = Dvar::Var("ui_browserMod").Get<int>();
		int ui_joinGametype         = Dvar::Var("ui_joinGametype").Get<int>();

		for (unsigned int i = 0; i < list->size(); ++i)
		{
			ServerList::ServerInfo* info = &(*list)[i];

			// Filter full servers
			if (!ui_browserShowFull && info->Clients >= info->MaxClients) continue;

			// Filter empty servers
			if (!ui_browserShowEmpty && info->Clients <= 0) continue;

			// Filter hardcore servers
			if ((ui_browserShowHardcore == 0 && info->Hardcore) || (ui_browserShowHardcore == 1 && !info->Hardcore)) continue;

			// Filter servers with password
			if ((ui_browserShowPassword == 0 && info->Password) || (ui_browserShowPassword == 1 && !info->Password)) continue;

			// Don't show modded servers
			if ((ui_browserMod == 0 && info->Mod.size()) || (ui_browserMod == 1 && !info->Mod.size())) continue;

			// Filter by gametype
			if (ui_joinGametype > 0 && (ui_joinGametype -1) < *Game::gameTypeCount  && Game::gameTypes[(ui_joinGametype - 1)].gameType != info->Gametype) continue;

			ServerList::VisibleList.push_back(i);
		}

		ServerList::SortList();
	}

	void ServerList::Refresh()
	{
		Dvar::Var("ui_serverSelected").Set(false);
		Localization::Set("MPUI_SERVERQUERIED", "Sent requests: 0/0");

// 		ServerList::OnlineList.clear();
// 		ServerList::OfflineList.clear();
// 		ServerList::FavouriteList.clear();

		auto list = ServerList::GetList();
		if (list) list->clear();

		ServerList::VisibleList.clear();
		ServerList::RefreshContainer.Mutex.lock();
		ServerList::RefreshContainer.Servers.clear();
		ServerList::RefreshContainer.SendCount = 0;
		ServerList::RefreshContainer.SentCount = 0;
		ServerList::RefreshContainer.Mutex.unlock();

		if (ServerList::IsOfflineList())
		{
			Discovery::Perform();
		}
		else if (ServerList::IsOnlineList())
		{
#ifdef USE_LEGACY_SERVER_LIST
			ServerList::RefreshContainer.AwatingList = true;
			ServerList::RefreshContainer.AwaitTime = Game::Com_Milliseconds();

			int masterPort = Dvar::Var("masterPort").Get<int>();
			const char* masterServerName = Dvar::Var("masterServerName").Get<const char*>();

			ServerList::RefreshContainer.Host = Network::Address(Utils::VA("%s:%u", masterServerName, masterPort));

			Logger::Print("Sending serverlist request to master: %s:%u\n", masterServerName, masterPort);

			Network::SendCommand(ServerList::RefreshContainer.Host, "getservers", Utils::VA("IW4 %i full empty", PROTOCOL));
			//Network::SendCommand(ServerList::RefreshContainer.Host, "getservers", "0 full empty");
#else
			Node::SyncNodeList();
#endif
		}
		else if (ServerList::IsFavouriteList())
		{
			ServerList::LoadFavourties();
		}
	}

	void ServerList::StoreFavourite(std::string server)
	{
		//json11::Json::parse()
		std::vector<std::string> servers;

		if (Utils::IO::FileExists("players/favourites.json"))
		{
			std::string data = Utils::IO::ReadFile("players/favourites.json");
			json11::Json object = json11::Json::parse(data, data);

			if (!object.is_array())
			{
				Logger::Print("Favourites storage file is invalid!\n");
				Game::MessageBox("Favourites storage file is invalid!", "Error");
				return;
			}

			auto storedServers = object.array_items();

			for (unsigned int i = 0; i < storedServers.size(); ++i)
			{
				if (!storedServers[i].is_string()) continue;
				if (storedServers[i].string_value() == server)
				{
					Game::MessageBox("Server already marked as favourite.", "Error");
					return;
				}

				servers.push_back(storedServers[i].string_value());
			}
		}

		servers.push_back(server);

		json11::Json data = json11::Json(servers);
		Utils::IO::WriteFile("players/favourites.json", data.dump());
		Game::MessageBox("Server added to favourites.", "Success");
	}

	void ServerList::LoadFavourties()
	{
		if (ServerList::IsFavouriteList() && Utils::IO::FileExists("players/favourites.json"))
		{
			auto list = ServerList::GetList();
			if (list) list->clear();

			std::string data = Utils::IO::ReadFile("players/favourites.json");
			json11::Json object = json11::Json::parse(data, data);

			if (!object.is_array())
			{
				Logger::Print("Favourites storage file is invalid!\n");
				Game::MessageBox("Favourites storage file is invalid!", "Error");
				return;
			}

			auto servers = object.array_items();

			for (unsigned int i = 0; i < servers.size(); ++i)
			{
				if(!servers[i].is_string()) continue;
				ServerList::InsertRequest(servers[i].string_value(), true);
			}
		}
	}

	void ServerList::InsertRequest(Network::Address address, bool acquireMutex)
	{
		if (acquireMutex) ServerList::RefreshContainer.Mutex.lock();

		ServerList::Container::ServerContainer container;
		container.Sent = false;
		container.Target = address;

		bool alreadyInserted = false;
		for (auto &server : ServerList::RefreshContainer.Servers)
		{
			if (server.Target == container.Target)
			{
				alreadyInserted = true;
				break;
			}
		}

		if (!alreadyInserted)
		{
			ServerList::RefreshContainer.Servers.push_back(container);

			auto list = ServerList::GetList();
			if (list)
			{
				for (auto server : *list)
				{
					if (server.Addr == container.Target)
					{
						--ServerList::RefreshContainer.SendCount;
						--ServerList::RefreshContainer.SentCount;
						break;
					}
				}
			}

			++ServerList::RefreshContainer.SendCount;
		}

		if (acquireMutex) ServerList::RefreshContainer.Mutex.unlock();
	}

	void ServerList::Insert(Network::Address address, Utils::InfoString info)
	{
		std::lock_guard<std::mutex> _(ServerList::RefreshContainer.Mutex);

		for (auto i = ServerList::RefreshContainer.Servers.begin(); i != ServerList::RefreshContainer.Servers.end();)
		{
			// Our desired server
			if (i->Target == address && i->Sent)
			{
				// Challenge did not match
				if (i->Challenge != info.Get("challenge"))
				{
					// Shall we remove the server from the queue?
					// Better not, it might send a second response with the correct challenge.
					// This might happen when users refresh twice (or more often) in a short period of time
					break;
				}

				ServerInfo server;
				server.Hostname = info.Get("hostname");
				server.Mapname = info.Get("mapname");
				server.Gametype = info.Get("gametype");
				server.Shortversion = info.Get("shortversion");
				server.Mod = info.Get("fs_game");
				server.MatchType = atoi(info.Get("matchtype").data());
				server.Clients = atoi(info.Get("clients").data());
				server.SecurityLevel = atoi(info.Get("securityLevel").data());
				server.MaxClients = atoi(info.Get("sv_maxclients").data());
				server.Password = (atoi(info.Get("isPrivate").data()) != 0);
				server.Hardcore = (atoi(info.Get("hc").data()) != 0);
				server.SVRunning = (atoi(info.Get("sv_running").data()) != 0);
				server.Ping = (Game::Sys_Milliseconds() - i->SendTime);
				server.Addr = address;

				// Remove server from queue
				i = ServerList::RefreshContainer.Servers.erase(i);

				// Check if already inserted and remove
				auto list = ServerList::GetList();
				if (!list) return;

				unsigned int k = 0;
				for (auto j = list->begin(); j != list->end(); ++k)
				{
					if (j->Addr == address)
					{
						j = list->erase(j);
					}
					else
					{
						++j;
					}
				}

				// Also remove from visible list
				for (auto j = ServerList::VisibleList.begin(); j != ServerList::VisibleList.end();)
				{
					if (*j == k)
					{
						j = ServerList::VisibleList.erase(j);
					}
					else
					{
						++j;
					}
				}

				if (info.Get("gamename") == "IW4"
					&& server.MatchType 
#ifndef DEBUG
					&& server.Shortversion == SHORTVERSION
#endif
					)
				{
					auto lList = ServerList::GetList();

					if (lList)
					{
						lList->push_back(server);
						ServerList::RefreshVisibleList();
					}
				}

				break;
			}
			else
			{
				++i;
			}
		}
	}

	ServerList::ServerInfo* ServerList::GetCurrentServer()
	{
		return ServerList::GetServer(ServerList::CurrentServer);
	}

	void ServerList::SortList()
	{
		qsort(ServerList::VisibleList.data(), ServerList::VisibleList.size(), sizeof(int), [] (const void* first, const void* second)
		{
			const unsigned int server1 = *static_cast<const unsigned int*>(first);
			const unsigned int server2 = *static_cast<const unsigned int*>(second);

			ServerInfo* info1 = nullptr;
			ServerInfo* info2 = nullptr;

			auto list = ServerList::GetList();
			if (!list) return 0;

			if (list->size() > server1) info1 = &(*list)[server1];
			if (list->size() > server2) info2 = &(*list)[server2];

			if (!info1) return 1;
			if (!info2) return -1;

			// Numerical comparisons
			if (ServerList::SortKey == ServerList::Column::Ping)
			{
				return ((info1->Ping - info2->Ping) * (ServerList::SortAsc ? 1 : -1));
			}
			else if (ServerList::SortKey == ServerList::Column::Players)
			{
				return ((info1->Clients - info2->Clients) * (ServerList::SortAsc ? 1 : -1));
			}

			std::string text1 = Colors::Strip(ServerList::GetServerText(info1, ServerList::SortKey));
			std::string text2 = Colors::Strip(ServerList::GetServerText(info2, ServerList::SortKey));

			// ASCII-based comparison
			return (text1.compare(text2) * (ServerList::SortAsc ? 1 : -1));
		});
	}

	ServerList::ServerInfo* ServerList::GetServer(unsigned int index)
	{
		if (ServerList::VisibleList.size() > index)
		{
			auto list = ServerList::GetList();
			if (!list) return nullptr;

			if (list->size() > ServerList::VisibleList[index])
			{
				return &(*list)[ServerList::VisibleList[index]];
			}
		}

		return nullptr;
	}

	void ServerList::Frame()
	{
		if (!ServerList::RefreshContainer.Mutex.try_lock()) return;

		if (ServerList::RefreshContainer.AwatingList)
		{
			// Check if we haven't got a response within 10 seconds
			if (Game::Sys_Milliseconds() - ServerList::RefreshContainer.AwaitTime > 5000)
			{
				ServerList::RefreshContainer.AwatingList = false;

				Logger::Print("We haven't received a response from the master within %d seconds!\n", (Game::Sys_Milliseconds() - ServerList::RefreshContainer.AwaitTime) / 1000);
			}
		}

		// Send requests to 10 servers each frame
		int SendServers = 10;
		
		for (unsigned int i = 0; i < ServerList::RefreshContainer.Servers.size(); ++i)
		{
			ServerList::Container::ServerContainer* server = &ServerList::RefreshContainer.Servers[i];
			if (server->Sent) continue;

			// Found server we can send a request to
			server->Sent = true;
			SendServers--;

			server->SendTime = Game::Sys_Milliseconds();
			server->Challenge = fmt::sprintf("%X", Utils::Cryptography::Rand::GenerateInt());

			++ServerList::RefreshContainer.SentCount;

			Network::SendCommand(server->Target, "getinfo", server->Challenge);

			// Display in the menu, like in COD4
			Localization::Set("MPUI_SERVERQUERIED", fmt::sprintf("Sent requests: %d/%d", ServerList::RefreshContainer.SentCount, ServerList::RefreshContainer.SendCount));

			if (SendServers <= 0) break;
		}

		ServerList::RefreshContainer.Mutex.unlock();
	}

	void ServerList::UpdateSource()
	{
		Dvar::Var netSource("ui_netSource");

		int source = netSource.Get<int>();

		if (++source > netSource.Get<Game::dvar_t*>()->max.i)
		{
			source = 0;
		}

		netSource.Set(source);

		ServerList::RefreshVisibleList();
	}

	void ServerList::UpdateGameType()
	{
		Dvar::Var joinGametype("ui_joinGametype");

		int gametype = joinGametype.Get<int>();

		if (++gametype > *Game::gameTypeCount)
		{
			gametype = 0;
		}

		joinGametype.Set(gametype);

		ServerList::RefreshVisibleList();
	}

	ServerList::ServerList()
	{
		ServerList::OnlineList.clear();
		ServerList::VisibleList.clear();

		Dvar::OnInit([] ()
		{
			Dvar::Register<bool>("ui_serverSelected", false, Game::dvar_flag::DVAR_FLAG_NONE, "Whether a server has been selected in the serverlist");
			Dvar::Register<const char*>("ui_serverSelectedMap", "mp_afghan", Game::dvar_flag::DVAR_FLAG_NONE, "Map of the selected server");
		});

		Localization::Set("MPUI_SERVERQUERIED", "Sent requests: 0/0");

		Network::Handle("getServersResponse", [] (Network::Address address, std::string data)
		{
			if (ServerList::RefreshContainer.Host != address) return; // Only parse from host we sent to

			ServerList::RefreshContainer.AwatingList = false;

			std::lock_guard<std::mutex> _(ServerList::RefreshContainer.Mutex);

			int offset = 0;
			int count = ServerList::RefreshContainer.Servers.size();
			ServerList::MasterEntry* entry = nullptr;

			// Find first entry
			do 
			{
				entry = reinterpret_cast<ServerList::MasterEntry*>(const_cast<char*>(data.data()) + offset++);
			}
			while (!entry->HasSeparator() && !entry->IsEndToken());

			for (int i = 0; !entry[i].IsEndToken() && entry[i].HasSeparator(); ++i)
			{
				Network::Address serverAddr = address;
				serverAddr.SetIP(entry[i].IP);
				serverAddr.SetPort(ntohs(entry[i].Port));
				serverAddr.SetType(Game::NA_IP);

				ServerList::InsertRequest(serverAddr, false);
			}

			Logger::Print("Parsed %d servers from master\n", ServerList::RefreshContainer.Servers.size() - count);
		});

		// Set default masterServerName + port and save it 
#ifdef USE_LEGACY_SERVER_LIST
		Utils::Hook::Set<char*>(0x60AD92, "localhost");
		Utils::Hook::Set<BYTE>(0x60AD90, Game::dvar_flag::DVAR_FLAG_SAVED); // masterServerName
		Utils::Hook::Set<BYTE>(0x60ADC6, Game::dvar_flag::DVAR_FLAG_SAVED); // masterPort
#endif

		// Add server list feeder
		UIFeeder::Add(2.0f, ServerList::GetServerCount, ServerList::GetServerText, ServerList::SelectServer);

		// Add required UIScripts
		UIScript::Add("UpdateFilter", ServerList::RefreshVisibleList);
		UIScript::Add("RefreshFilter", ServerList::UpdateVisibleList);

		UIScript::Add("RefreshServers", ServerList::Refresh);
		UIScript::Add("JoinServer", [] ()
		{
			ServerList::ServerInfo* info = ServerList::GetServer(ServerList::CurrentServer);

			if (info)
			{
				Party::Connect(info->Addr);
			}
		});
		UIScript::Add("ServerSort", [] (UIScript::Token token)
		{
			int key = token.Get<int>();

			if (ServerList::SortKey == key)
			{
				ServerList::SortAsc = !ServerList::SortAsc;
			}
			else
			{
				ServerList::SortKey = key;
				ServerList::SortAsc = true;
			}

			Logger::Print("Sorting server list by token: %d\n", ServerList::SortKey);
			ServerList::SortList();
		});
		UIScript::Add("CreateListFavorite", [] ()
		{
			ServerList::ServerInfo* info = ServerList::GetCurrentServer();

			if (info)
			{
				ServerList::StoreFavourite(info->Addr.GetString());
			}
		});
		UIScript::Add("CreateFavorite", [] ()
		{
			ServerList::StoreFavourite(Dvar::Var("ui_favoriteAddress").Get<std::string>());
		});
		UIScript::Add("CreateCurrentServerFavorite", []()
		{
			if (Dvar::Var("cl_ingame").Get<bool>())
			{
				std::string addressText = Network::Address(*Game::connectedHost).GetString();
				if (addressText != "0.0.0.0:0" && addressText != "loopback")
				{
					ServerList::StoreFavourite(addressText);
				}
			}
		});

		// Add required ownerDraws
		UIScript::AddOwnerDraw(220, ServerList::UpdateSource);
		UIScript::AddOwnerDraw(253, ServerList::UpdateGameType);

		// Add frame callback
		Renderer::OnFrame(ServerList::Frame);

		// This is placed here in case the anticheat has been disabled!
#ifndef DEBUG
		Renderer::OnFrame(AntiCheat::ReadIntegrityCheck);
#endif
	}

	ServerList::~ServerList()
	{
		ServerList::OnlineList.clear();
		ServerList::OfflineList.clear();
		ServerList::FavouriteList.clear();
		ServerList::VisibleList.clear();

		ServerList::RefreshContainer.Mutex.lock();
		ServerList::RefreshContainer.AwatingList = false;
		ServerList::RefreshContainer.Servers.clear();
		ServerList::RefreshContainer.Mutex.unlock();
	}
}
