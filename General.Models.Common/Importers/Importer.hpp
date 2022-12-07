#ifndef GENERAL_MODELS_IMPORTER_HPP
#define GENERAL_MODELS_IMPORTER_HPP

namespace General
{
	namespace Models
	{
		struct Model;

		enum UnitLevel
		{
			UNIT_LEVEL_MILLIMETER,
			UNIT_LEVEL_CENTIMETER,
			UNIT_LEVEL_DECIMITER,
			UNIT_LEVEL_METER,
		};

		struct GENERAL_API ImportParams
		{
			UnitLevel unitLevel; 
			const char* filename;
		};

		class GENERAL_API Importer
		{
		private:
			const std::string mFilename;
			UnitLevel mUnitLevel;

			std::unordered_map<std::string, Node*> mNodeMap;

			bool mHasError;
			std::string mErrorMessage;
		protected:
			Model* mModel;
			float mScaleFactor;
		public:
			Importer(const ImportParams& params);
			virtual ~Importer() = 0;

			const std::string& GetFilename() const;
			UnitLevel GetUnitLevel() const;
		protected:
			std::string getFullPath(const std::string& maybePath) const;
			std::string findFile(const std::string& maybePath) const;
		public:
			const Model* Import();
		protected:
			virtual bool internalImport(Model* model) = 0;

			void registerNode(Node* node);
			Node* findNode(const std::string& name);

			void setError(const std::string& error);
		};
	}
}

#endif // GENERAL_MODELS_IMPORTER_HPP