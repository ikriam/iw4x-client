namespace Assets
{
	class IMaterialTechniqueSet : public Components::AssetHandler::IAsset
	{
		virtual Game::XAssetType GetType() override { return Game::XAssetType::ASSET_TYPE_TECHSET; };

		virtual void Save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
		virtual void Mark(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder) override;
	};
}