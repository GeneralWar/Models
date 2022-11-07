#ifndef GENERAL_MODELS_IMPORTER_HPP
#define GENERAL_MODELS_IMPORTER_HPP

namespace General
{
	namespace Models
	{
		struct Model;

		class GENERAL_API Importer
		{
		private:
			char* mFilename;
		public:
			Importer(const char* filename);
			virtual ~Importer() = 0;

			inline const char* GetFilename() const;

			virtual bool IsValid() = 0;

			virtual const Model* Import() = 0;
		};
	}
}

#endif // GENERAL_MODELS_IMPORTER_HPP