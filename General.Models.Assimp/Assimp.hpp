#ifndef GENERAL_MODELS_ASSIMP_HPP
#define GENERAL_MODELS_ASSIMP_HPP

namespace General
{
	namespace Models
	{
		struct Model;
		struct ImportParams;

		EXPORT const Model* load_model_from_assimp(const ImportParams* params);
	}
}

#endif