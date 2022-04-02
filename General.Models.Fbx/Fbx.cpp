#include "pch.h"
#include "Fbx.hpp"
#include "Importers/Importer.hpp"

namespace General
{
	namespace Models
	{
		using namespace Fbx;

		Model* load_model_fbx(const char* filename)
		{
			AutoRelease<ModelImporter> importer = MakeAutoRelease<ModelImporter>(filename);
			return importer->Import();
		};
	}
}