#include "STDInclude.hpp"

namespace Assets
{
	void Isnd_alias_list_t::load(Game::XAssetHeader* header, const std::string& name, Components::ZoneBuilder::Zone* builder)
	{
		Components::FileSystem::File aliasFile(Utils::String::VA("sounds/%s", name.data()));
		if (!aliasFile.exists())
		{
			header->sound = Components::AssetHandler::FindOriginalAsset(this->getType(), name.data()).sound;
			return;
		}

		Game::snd_alias_list_t* aliasList = builder->getAllocator()->allocate<Game::snd_alias_list_t>();
		if (!aliasList)
		{
			Components::Logger::Print("Error allocating memory for sound alias structure!\n");
			return;
		}

		aliasList->head = builder->getAllocator()->allocate<Game::snd_alias_t>();
		if (!aliasList->head)
		{
			Components::Logger::Print("Error allocating memory for sound alias structure!\n");
			return;
		}
		aliasList->head->soundFile = builder->getAllocator()->allocate<Game::SoundFile>();
		if (!aliasList->head->soundFile)
		{
			Components::Logger::Print("Error allocating memory for sound alias structure!\n");
			return;
		}

		aliasList->count = 1;

		std::string errors;
		json11::Json infoData = json11::Json::parse(aliasFile.getBuffer(), errors);

		if (!infoData.is_object())
		{
			Components::Logger::Error("Failed to load sound %s!", name.data());
			return;
		}

		Game::snd_alias_t* alias = aliasList->head;

		// try and parse everything and if it fails then fail for the whole file
		auto type = infoData["type"];
		auto subtitle = infoData["subtitle"];
		auto secondaryAliasName = infoData["secondaryAliasName"];
		auto chainAliasName = infoData["chainAliasName"];
		auto soundFile = infoData["soundFile"];
		auto sequence = infoData["sequence"];
		auto volMin = infoData["volMin"];
		auto volMax = infoData["volMax"];
		auto pitchMin = infoData["pitchMin"];
		auto pitchMax = infoData["pitchMax"];
		auto distMin = infoData["distMin"];
		auto distMax = infoData["distMax"];
		auto flags = infoData["flags"];
		auto slavePercentage = infoData["slavePercentage"];
		auto probability = infoData["probability"];
		auto lfePercentage = infoData["lfePercentage"];
		auto centerPercentage = infoData["centerPercentage"];
		auto startDelay = infoData["startDelay"];
		auto volumeFalloffCurve = infoData["volumeFalloffCurve"];
		auto envelopMin = infoData["envelopMin"];
		auto envelopMax = infoData["envelopMax"];
		auto envelopPercentage = infoData["envelopPercentage"];
		auto speakerMap = infoData["speakerMap"];

		if (type.is_null() || soundFile.is_null())
		{
			Components::Logger::Error("Failed to parse sound %s! Each alias must have at least a type and a soundFile", name.data());
			return;
		}

#define CHECK(x, type) (x.is_##type##() || x.is_null())

		// TODO: actually support all of those properties
		if (CHECK(type, string) && CHECK(subtitle, string) && CHECK(secondaryAliasName, string) && CHECK(chainAliasName, string) &&
			CHECK(soundFile, string) && CHECK(sequence, number) && CHECK(volMin, number) && CHECK(volMax, number) && CHECK(pitchMin, number) &&
			CHECK(pitchMax, number) && CHECK(distMin, number) && CHECK(distMax, number) && CHECK(flags, number) && CHECK(slavePercentage, number) &&
			CHECK(probability, number) && CHECK(lfePercentage, number) && CHECK(centerPercentage, number) && CHECK(startDelay, number) &&
			CHECK(volumeFalloffCurve, string) && CHECK(envelopMin, number) && CHECK(envelopMax, number) && CHECK(envelopPercentage, number) &&
			CHECK(speakerMap, string))
		{

			alias->soundFile->exists = true;

			if (subtitle.is_string())
			{
				alias->subtitle = subtitle.string_value().data();
			}
			if (secondaryAliasName.is_string())
			{
				alias->secondaryAliasName = secondaryAliasName.string_value().data();
			}
			if (chainAliasName.is_string())
			{
				alias->chainAliasName = chainAliasName.string_value().data();
			}

			alias->sequence = sequence.int_value();
			alias->volMin = float(volMin.number_value());
			alias->volMax = float(volMax.number_value());
			alias->pitchMin = float(pitchMin.number_value());
			alias->pitchMax = float(pitchMax.number_value());
			alias->distMin = float(distMin.number_value());
			alias->distMax = float(distMax.number_value());
			alias->flags = flags.int_value();
			alias->___u15.slavePercentage = float(slavePercentage.number_value());
			alias->probability = float(probability.number_value());
			alias->lfePercentage = float(lfePercentage.number_value());
			alias->centerPercentage = float(centerPercentage.number_value());
			alias->startDelay = startDelay.int_value();
			alias->envelopMin = float(envelopMin.number_value());
			alias->envelopMax = float(envelopMax.number_value());
			alias->envelopPercentage = float(envelopPercentage.number_value());

			if (volumeFalloffCurve.is_string())
			{
				alias->volumeFalloffCurve = Components::AssetHandler::FindAssetForZone(Game::XAssetType::ASSET_TYPE_SOUND_CURVE, volumeFalloffCurve.string_value(), builder).sndCurve;
			}

			if (type.string_value() == "loaded"s)
			{
				alias->soundFile->type = Game::SAT_LOADED;
				alias->soundFile->u.loadSnd = Components::AssetHandler::FindAssetForZone(Game::XAssetType::ASSET_TYPE_LOADED_SOUND, soundFile.string_value(), builder).loadSnd;
			}
			else if (type.string_value() == "streamed"s)
			{
				alias->soundFile->type = Game::SAT_STREAMED;
				std::string streamedFile = soundFile.string_value();
				int split = streamedFile.find_last_of('/');
				alias->soundFile->u.streamSnd.filename.info.raw.dir = builder->getAllocator()->duplicateString(streamedFile.substr(0, split).c_str());
				alias->soundFile->u.streamSnd.filename.info.raw.name = builder->getAllocator()->duplicateString(streamedFile.substr(split).c_str());
			}
			else
			{
				Components::Logger::Error("Failed to parse sound %s! Invalid sound type %s", name.data(), type.string_value().c_str());
			}
		}
		else
		{
			Components::Logger::Error("Failed to parse sound %s!", name.data());
			return;
		}

#undef CHECK

	}

	void Isnd_alias_list_t::mark(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		Game::snd_alias_list_t* asset = header.sound;

		for (int i = 0; i < asset->count; ++i)
		{
			Game::snd_alias_t* alias = &asset->head[i];

			if (alias->soundFile && alias->soundFile->type == Game::snd_alias_type_t::SAT_LOADED)
			{
				builder->loadAsset(Game::XAssetType::ASSET_TYPE_LOADED_SOUND, alias->soundFile->u.loadSnd);
			}

			if (alias->volumeFalloffCurve)
			{
				builder->loadAsset(Game::XAssetType::ASSET_TYPE_SOUND_CURVE, alias->volumeFalloffCurve);
			}
		}
	}

	void Isnd_alias_list_t::save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		AssertSize(Game::snd_alias_list_t, 12);

		Utils::Stream* buffer = builder->getBuffer();
		Game::snd_alias_list_t* asset = header.sound;
		Game::snd_alias_list_t* dest = buffer->dest<Game::snd_alias_list_t>();
		buffer->save(asset);

		buffer->pushBlock(Game::XFILE_BLOCK_VIRTUAL);

		if (asset->aliasName)
		{
			buffer->saveString(builder->getAssetName(this->getType(), asset->aliasName));
			Utils::Stream::ClearPointer(&dest->aliasName);
		}

		if (asset->head)
		{
			if (builder->hasPointer(asset->head))
			{
				dest->head = builder->getPointer(asset->head);
			}
			else
			{
				AssertSize(Game::snd_alias_t, 100);

				buffer->align(Utils::Stream::ALIGN_4);
				builder->storePointer(asset->head);

				Game::snd_alias_t* destHead = buffer->dest<Game::snd_alias_t>();
				buffer->saveArray(asset->head, asset->count);

				for (int i = 0; i < asset->count; ++i)
				{
					Game::snd_alias_t* destAlias = &destHead[i];
					Game::snd_alias_t* alias = &asset->head[i];

					if (alias->aliasName)
					{
						buffer->saveString(alias->aliasName);
						Utils::Stream::ClearPointer(&destAlias->aliasName);
					}

					if (alias->subtitle)
					{
						buffer->saveString(alias->subtitle);
						Utils::Stream::ClearPointer(&destAlias->subtitle);
					}

					if (alias->secondaryAliasName)
					{
						buffer->saveString(alias->secondaryAliasName);
						Utils::Stream::ClearPointer(&destAlias->secondaryAliasName);
					}

					if (alias->chainAliasName)
					{
						buffer->saveString(alias->chainAliasName);
						Utils::Stream::ClearPointer(&destAlias->chainAliasName);
					}

					if (alias->mixerGroup)
					{
						buffer->saveString(alias->mixerGroup);
						Utils::Stream::ClearPointer(&destAlias->mixerGroup);
					}

					if (alias->soundFile)
					{
						if (builder->hasPointer(alias->soundFile))
						{
							destAlias->soundFile = builder->getPointer(alias->soundFile);
						}
						else
						{
							AssertSize(Game::snd_alias_t, 100);

							buffer->align(Utils::Stream::ALIGN_4);
							builder->storePointer(alias->soundFile);

							Game::SoundFile* destSoundFile = buffer->dest<Game::SoundFile>();
							buffer->save(alias->soundFile);

							// Save_SoundFileRef
							{
								if (alias->soundFile->type == Game::snd_alias_type_t::SAT_LOADED)
								{
									destSoundFile->u.loadSnd = builder->saveSubAsset(Game::XAssetType::ASSET_TYPE_LOADED_SOUND, alias->soundFile->u.loadSnd).loadSnd;
								}
								else
								{
									// Save_StreamedSound
									{
										if (alias->soundFile->u.streamSnd.filename.info.raw.dir)
										{
											buffer->saveString(alias->soundFile->u.streamSnd.filename.info.raw.dir);
											Utils::Stream::ClearPointer(&destSoundFile->u.streamSnd.filename.info.raw.dir);
										}

										if (alias->soundFile->u.streamSnd.filename.info.raw.name)
										{
											buffer->saveString(alias->soundFile->u.streamSnd.filename.info.raw.name);
											Utils::Stream::ClearPointer(&destSoundFile->u.streamSnd.filename.info.raw.name);
										}
									}
								}
							}

							Utils::Stream::ClearPointer(&destAlias->soundFile);
						}
					}

					if (alias->volumeFalloffCurve)
					{
						destAlias->volumeFalloffCurve = builder->saveSubAsset(Game::XAssetType::ASSET_TYPE_SOUND_CURVE, alias->volumeFalloffCurve).sndCurve;
					}

					if (alias->speakerMap)
					{
						if (builder->hasPointer(alias->speakerMap))
						{
							destAlias->speakerMap = builder->getPointer(alias->speakerMap);
						}
						else
						{
							AssertSize(Game::SpeakerMap, 408);

							buffer->align(Utils::Stream::ALIGN_4);
							builder->storePointer(alias->speakerMap);

							Game::SpeakerMap* destSoundFile = buffer->dest<Game::SpeakerMap>();
							buffer->save(alias->speakerMap);

							if (alias->speakerMap->name)
							{
								buffer->saveString(alias->speakerMap->name);
								Utils::Stream::ClearPointer(&destSoundFile->name);
							}

							Utils::Stream::ClearPointer(&destAlias->speakerMap);
						}
					}
				}

				Utils::Stream::ClearPointer(&dest->head);
			}
		}

		buffer->popBlock();
	}
}
