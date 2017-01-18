namespace Components
{
	class Singleton : public Component
	{
	public:
		Singleton();

#if defined(DEBUG) || defined(FORCE_UNIT_TESTS)
		const char* getName() { return "Singleton"; };
#endif

		static bool IsFirstInstance();

	private:
		static bool FirstInstance;
	};
}
