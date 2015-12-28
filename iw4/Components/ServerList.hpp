namespace Components
{
	class ServerList : public Component
	{
	public:
		struct ServerInfo
		{
			Network::Address Addr;
			bool Visible;
			std::string Hostname;
			std::string Mapname;
			std::string Gametype;
			std::string Mod;
			int Clients;
			int MaxClients;
			bool Password;
			int Ping;
			int MatchType;
			bool Hardcore;
		};

		ServerList();
		~ServerList();
		const char* GetName() { return "ServerList"; };

		static void Refresh();
		static void Insert(Network::Address address, Utils::InfoString info);

	private:
		enum Column
		{
			Password,
			Hostname,
			Mapname,
			Players,
			Gametype,
			Ping,
		};

#pragma pack(push, 1)
		union MasterEntry
		{
			char Token[7];
			struct
			{
				uint32_t IP;
				uint16_t Port;
			};

			bool IsEndToken()
			{
				// End of transmission or file token
				return (Token[0] == 'E' && Token[1] == 'O' && (Token[2] == 'T' || Token[2] == 'F'));
			}

			bool HasSeparator()
			{
				return (Token[6] == '\\');
			}
		};
#pragma pack(pop)

		struct Container
		{
			struct ServerContainer
			{
				bool Sent;
				int SendTime;
				std::string Challenge;
				Network::Address Target;
			};

			int SentCount;
			int SendCount;
			Network::Address Host;
			std::vector<ServerContainer> Servers;
			std::mutex Mutex;
		};

		static int GetServerCount();
		static const char* GetServerText(int index, int column);
		static void SelectServer(int index);

		static void Frame();

		static unsigned int CurrentServer;
		static Container RefreshContainer;
		static std::vector<ServerInfo> OnlineList;
	};
}