#ifndef GENERAL_MODELS_FBX_HPP
#define GENERAL_MODELS_FBX_HPP

namespace General
{
	namespace Models
	{
		struct Model;

		GENERAL_API Model* load_model_fbx(const char* filename);
	}
}

#endif