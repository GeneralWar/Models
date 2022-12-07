#ifndef GENERAL_MODELS_ASSIMP_IMPORTER_HPP
#define GENERAL_MODELS_ASSIMP_IMPORTER_HPP

struct aiScene;
struct aiNode;
struct aiMesh;

struct aiAnimation;

struct aiBone;
struct aiSkeleton;
struct aiVertexWeight;

namespace General
{
	namespace Models
	{
		class AssimpModelImporter : public Importer
		{
		private:
			std::unordered_map<const aiNode*, Node*> mAssimp2NodeMap;
			std::unordered_map<const aiMesh*, Mesh*> mAssimp2MeshMap;
			std::unordered_map<Mesh*, std::vector<aiBone*>> mMeshBones;
		public:
			AssimpModelImporter(const ImportParams& params);
			~AssimpModelImporter();
		protected:
			virtual bool internalImport(Model* model) override;
		private:
			void checkNode(const aiScene* assimpScene, const aiNode* assimpNode, Node* node);
			void checkMesh(const aiScene* assimpScene, const aiMesh* assimpMesh, Mesh* mesh);

			void checkAnimation(const aiScene* assimpScene, const aiAnimation* assimpAnimation);

			void checkSkeleton(const aiScene* assimpScene);
			void checkBoneWeights(const aiScene* assimpScene, Mesh* mesh, const aiNode* assimpBone, const aiMatrix4x4& offsetMatrix, const unsigned int& weightCount, const aiVertexWeight* weights);
		};
	}
}

#endif // GENERAL_MODELS_ASSIMP_IMPORTER_HPP