#ifndef GENERAL_MODELS_FBX_IMPORTER_HPP
#define GENERAL_MODELS_FBX_IMPORTER_HPP

namespace General
{
	namespace Models
	{
		namespace Fbx
		{
			struct Context;

			class ModelImporter : public Importer
			{
			private:
				Context* mContext;
			public:
				ModelImporter(const char* filename);
				~ModelImporter();

				virtual bool IsValid() override;

				virtual Model* Import() override;
			};
		}
	}
}

#endif // GENERAL_MODELS_FBX_IMPORTER_HPP