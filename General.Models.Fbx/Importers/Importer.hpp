#ifndef GENERAL_MODELS_FBX_IMPORTER_HPP
#define GENERAL_MODELS_FBX_IMPORTER_HPP

namespace fbxsdk
{
	class FbxManager;
	class FbxImporter;

	class FbxScene;
	class FbxNode;
	class FbxMesh;
	class FbxPose;

	class FbxLayerElementUV;
	typedef FbxLayerElementUV FbxGeometryElementUV;

	class FbxAnimStack;
	class FbxAnimLayer;

	class FbxProperty;
	class FbxSurfaceMaterial;
}

namespace General
{
	namespace Models
	{
		class FbxModelImporter : public Importer
		{
		public:
			struct Matrix
			{
				double values[16];
			};
		private:
			FbxManager* mManager;
			FbxImporter* mImporter;

			std::unordered_map<Mesh*, std::vector<Normal>> mMeshNormals;

			std::unordered_map<std::string, const FbxGeometryElementUV*> mFbxUVSets;

			std::unordered_map<Mesh*, FbxMesh*> mMesh2FbxMap;
			std::unordered_map<FbxNode*, Node*> mFbx2NodeMap;

			FbxPose* mPose;
			std::unordered_map<FbxNode*, Matrix> mFbxPoseMap;

			std::vector<FbxAnimStack*> mAnimationStacks;
			std::vector<FbxAnimLayer*> mAnimationLayers;
			std::vector<Animation*> mAnimations;
		public:
			FbxModelImporter(const ImportParams& params);
			~FbxModelImporter();
		private:
			bool checkImportStatus();
		protected:
			virtual bool internalImport(Model* model) override;
		private:
			void import(FbxScene* scene);

			void checkNode(Node* node, FbxNode* fbxNode);

			Mesh* checkMesh(FbxMesh* fbxMesh);
			int checkMeshVertices(const FbxMesh* fbxMesh, Mesh* mesh);
			int checkMeshIndices(const FbxMesh* fbxMesh, Mesh* mesh);
			void checkMeshUVs(const FbxMesh* fbxMesh, Mesh* mesh);

			Material* checkMaterial(/*const */FbxSurfaceMaterial* fbxMaterial);
			void enumerateMaterialPropertyForTexture(Material* material, /*const */FbxSurfaceMaterial* fbxMaterial);
			void checkPropertyForTexture(Material* material, const FbxProperty* property);
			const char* checkTexture(Material* material, FbxTexture* fbxTexture);

			void checkAnimations(FbxScene* scene);
			void checkAnimationNodeFrames(FbxNode* fbxNode, Node* node);

			void checkSkinWeights();

			void postProcess();
			void checkVerticesWithPose(Mesh* mesh, std::vector<Vertex>& vertices);
			void checkVerticesWithUVAndNormals(Mesh* mesh, const UVSet* uvSet, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices);
			void checkVerticesWithUVAndNormals(Mesh* mesh, const std::vector<Normal> normals, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices);
			void checkVerticesWithUVAndNormals(Mesh* mesh, const UVSet* uvSet, const std::vector<Normal> normals, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices);
		};
	}
}

#endif // GENERAL_MODELS_FBX_IMPORTER_HPP