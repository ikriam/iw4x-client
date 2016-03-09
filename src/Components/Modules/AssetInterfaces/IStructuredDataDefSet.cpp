#include <STDInclude.hpp>

namespace Assets
{
	void IStructuredDataDefSet::Save_StructuredDataEnumArray(Game::StructuredDataEnum* enums, int numEnums, Components::ZoneBuilder::Zone* builder)
	{
		Utils::Stream* buffer = builder->GetBuffer();

		Game::StructuredDataEnum* destEnums = buffer->Dest<Game::StructuredDataEnum>();
		buffer->SaveArray(enums, numEnums);

		for (int i = 0; i < numEnums; ++i)
		{
			Game::StructuredDataEnum* destEnum = &destEnums[i];
			Game::StructuredDataEnum* enum_ = &enums[i];

			if (enum_->indices)
			{
				Assert_Size(Game::StructuredDataEnumEntry, 8);
				buffer->Align(Utils::Stream::ALIGN_4);

				Game::StructuredDataEnumEntry* destIndices = buffer->Dest<Game::StructuredDataEnumEntry>();
				buffer->SaveArray(enum_->indices, enum_->numIndices);

				for (int j = 0; j < enum_->numIndices; ++j)
				{
					Game::StructuredDataEnumEntry* destIndex = &destIndices[j];
					Game::StructuredDataEnumEntry* index = &enum_->indices[j];

					if (index->key)
					{
						buffer->SaveString(index->key);
						destIndex->key = reinterpret_cast<char*>(-1);
					}
				}

				destEnum->indices = reinterpret_cast<Game::StructuredDataEnumEntry*>(-1);
			}
		}
	}

	void IStructuredDataDefSet::Save_StructuredDataStructArray(Game::StructuredDataStruct* structs, int numStructs, Components::ZoneBuilder::Zone* builder)
	{
		Utils::Stream* buffer = builder->GetBuffer();

		Game::StructuredDataStruct* destStructs = buffer->Dest<Game::StructuredDataStruct>();
		buffer->SaveArray(structs, numStructs);

		for (int i = 0; i < numStructs; ++i)
		{
			Game::StructuredDataStruct* destStruct = &destStructs[i];
			Game::StructuredDataStruct* struct_ = &structs[i];

			if (struct_->property)
			{
				Assert_Size(Game::StructuredDataStructProperty, 16);
				buffer->Align(Utils::Stream::ALIGN_4);

				Game::StructuredDataStructProperty* destProperties = buffer->Dest<Game::StructuredDataStructProperty>();
				buffer->SaveArray(struct_->property, struct_->numProperties);

				for (int j = 0; j < struct_->numProperties; ++j)
				{
					Game::StructuredDataStructProperty* destProperty = &destProperties[j];
					Game::StructuredDataStructProperty* property = &struct_->property[j];

					if (property->name)
					{
						buffer->SaveString(property->name);
						destProperty->name = reinterpret_cast<char*>(-1);
					}
				}

				destStruct->property = reinterpret_cast<Game::StructuredDataStructProperty*>(-1);
			}
		}
	}

	void IStructuredDataDefSet::Save(Game::XAssetHeader header, Components::ZoneBuilder::Zone* builder)
	{
		Assert_Size(Game::StructuredDataDefSet, 12);

		Utils::Stream* buffer = builder->GetBuffer();
		Game::StructuredDataDefSet* asset = header.structuredData;
		Game::StructuredDataDefSet* dest = buffer->Dest<Game::StructuredDataDefSet>();
		buffer->Save(asset, sizeof(Game::StructuredDataDefSet));

		buffer->PushBlock(Game::XFILE_BLOCK_VIRTUAL);

		if (asset->name)
		{
			buffer->SaveString(builder->GetAssetName(this->GetType(), asset->name));
			dest->name = reinterpret_cast<char*>(-1);
		}

		if (asset->data)
		{
			Assert_Size(Game::StructuredDataDef, 52);
			buffer->Align(Utils::Stream::ALIGN_4);

			Game::StructuredDataDef* destDataArray = buffer->Dest<Game::StructuredDataDef>();
			buffer->SaveArray(asset->data, asset->count);

			for (int i = 0; i < asset->count; ++i)
			{
				Game::StructuredDataDef* destData = &destDataArray[i];
				Game::StructuredDataDef* data = &asset->data[i];

				if (data->enums)
				{
					Assert_Size(Game::StructuredDataEnum, 12);
					buffer->Align(Utils::Stream::ALIGN_4);

					IStructuredDataDefSet::Save_StructuredDataEnumArray(data->enums, data->numEnums, builder);
					destData->enums = reinterpret_cast<Game::StructuredDataEnum*>(-1);
				}

				if (data->structs)
				{
					Assert_Size(Game::StructuredDataStruct, 16);
					buffer->Align(Utils::Stream::ALIGN_4);

					IStructuredDataDefSet::Save_StructuredDataStructArray(data->structs, data->numStructs, builder);
					destData->structs = reinterpret_cast<Game::StructuredDataStruct*>(-1);
				}

				if (data->indexedArrays)
				{
					Assert_Size(Game::StructuredDataIndexedArray, 16);
					buffer->Align(Utils::Stream::ALIGN_4);

					buffer->SaveArray(data->indexedArrays, data->numIndexedArrays);
					destData->indexedArrays = reinterpret_cast<Game::StructuredDataIndexedArray*>(-1);
				}

				if (data->enumArrays)
				{
					Assert_Size(Game::StructuredDataEnumedArray, 16);
					buffer->Align(Utils::Stream::ALIGN_4);

					buffer->SaveArray(data->enumArrays, data->numEnumArrays);
					destData->enumArrays = reinterpret_cast<Game::StructuredDataEnumedArray*>(-1);
				}
			}

			dest->data = reinterpret_cast<Game::StructuredDataDef*>(-1);
		}

		buffer->PopBlock();
	}
}