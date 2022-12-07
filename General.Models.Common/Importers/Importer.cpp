#include "pch.h"
#include "Importer.hpp"

namespace General
{
	namespace Models
	{
		Importer::Importer(const ImportParams& params) : mFilename(params.filename), mUnitLevel(params.unitLevel), mHasError(false), mErrorMessage(), mModel(), mScaleFactor(1.0f) { }

		Importer::~Importer() { }

		const std::string& Importer::GetFilename() const 
		{
			return mFilename;
		}

		UnitLevel Importer::GetUnitLevel() const
		{
			return mUnitLevel;
		}

		std::string Importer::getFullPath(const std::string& maybePath) const
		{
			std::filesystem::path path(maybePath);
			if (path.is_absolute())
			{
				return path.make_preferred().string();
			}

			std::filesystem::path filename = mFilename;
			std::filesystem::path directory = filename.parent_path();
			return directory.append(maybePath).lexically_normal().string();
		}

		std::string Importer::findFile(const std::string& maybePath) const
		{
			std::filesystem::path path(maybePath);
			if (path.is_absolute())
			{
				return path.make_preferred().string();
			}

			std::filesystem::path filename = mFilename;
			std::filesystem::path directory = filename.parent_path();
			std::filesystem::path preferredPath = (directory / maybePath).lexically_normal();
			if (std::filesystem::exists(preferredPath))
			{
				return preferredPath.string();
			}

			return Directory::FindFile(directory, path.filename(), true).string();
		}

		const Model* Importer::Import()
		{
			const std::string& filename = mFilename;
			if (filename.empty())
			{
				return nullptr;
			}

			mModel = create_model();
			if (!this->internalImport(mModel))
			{
				destroy_model(mModel);
				return nullptr;
			}
			return mModel;
		}


		void Importer::registerNode(Node* node)
		{
			if (mNodeMap.end() != mNodeMap.find(node->name))
			{
				TRACE_WARN("Duplication node registration for name %s", node->name);
			}

			mNodeMap[node->name] = node;
		}

		Node* Importer::findNode(const std::string& name)
		{
			auto finder = mNodeMap.find(name);
			return mNodeMap.end() == finder ? nullptr : finder->second;
		}

		void Importer::setError(const std::string& error)
		{
			mHasError = true;
			mErrorMessage = error;
		}
	}
}