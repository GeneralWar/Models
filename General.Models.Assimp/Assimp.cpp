#include "pch.h"
#include "Assimp.hpp"
#include "Importers/Importer.hpp"

namespace General
{
	namespace Models
	{
		const Model* load_model_from_assimp(const ImportParams* params)
		{
			CHECK(params && params->filename && strlen(params->filename), nullptr);

			AssimpModelImporter importer(*params);
			return importer.Import();
		};
	}
}