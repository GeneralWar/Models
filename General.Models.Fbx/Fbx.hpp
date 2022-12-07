#ifndef GENERAL_MODELS_FBX_HPP
#define GENERAL_MODELS_FBX_HPP

namespace General
{
	namespace Models
	{
		struct Model;
		struct ImportParams;

		EXPORT const Model* load_model_from_fbx(const ImportParams* params);
	}
}

#endif