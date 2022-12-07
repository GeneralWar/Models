#include "pch.h"
#include "Fbx.hpp"
#include "Importers/Importer.hpp"

namespace General
{
	namespace Models
	{
		const Model* load_model_from_fbx(const ImportParams* params)
		{
			CHECK(params && params->filename && strlen(params->filename), nullptr);

			FbxModelImporter importer(*params);
			return importer.Import();
		};
	}
}