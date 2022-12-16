#include "pch.h"
#include "Importer.hpp"
using namespace General;

#define _USE_MATH_DEFINES
#include <math.h>

namespace General
{
	namespace Models
	{
		Vector3 vector3_from_ai(const aiVector3D& value)
		{
			Vector3 v = { };
			v.x = static_cast<float>(value.x);
			v.y = static_cast<float>(value.y);
			v.z = static_cast<float>(value.z);
			return v;
		}

		Vector3 vector3_from_ai_quaternion(const aiQuaternion& value)
		{
			aiVector3D translation, rotation, scaling;
			aiMatrix4x4(value.GetMatrix()).Decompose(scaling, rotation, translation);
			//assert(.0f == translation.x && .0f == translation.y && .0f == translation.z && 1.0f == scaling.x && 1.0f == scaling.y && 1.0f == scaling.z);
			return vector3_scale(vector3_from_ai(rotation), static_cast<float>(180.0 / M_PI));
		}

		Vector4 vector4_from_ai_quaternion(const aiQuaternion& value)
		{
			Vector4 v = { };
			v.x = static_cast<float>(value.x);
			v.y = static_cast<float>(value.y);
			v.z = static_cast<float>(value.z);
			v.w = static_cast<float>(value.w);
			return v;
		}

		Transform transform_from_ai(const aiMatrix4x4& matrix)
		{
			aiVector3D translation, rotation, scaling;
			matrix.Decompose(scaling, rotation, translation);

			Transform transform = { };
			transform.translation = vector3_from_ai(translation);
			transform.rotation = vector3_from_ai(rotation);
			transform.scaling = vector3_from_ai(scaling);
			return transform;
		}

		Matrix matrix_from_ai(const aiMatrix4x4& matrix)
		{
			Matrix m = { };
			const float* sourceValues = &matrix.a1;
			float* targetValues = m.values;
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					*(targetValues + i * 4 + j) = static_cast<float>(*(sourceValues + i + j * 4));
				}
			}
			return m;
		}

		Node* create_node_from_ai(const aiNode* assimpNode)
		{
			aiVector3D translation, rotation, scaling;
			assimpNode->mTransformation.Decompose(scaling, rotation, translation);

			Node* node = create_node(assimpNode->mName.C_Str());
			node->localPosition = vector3_from_ai(translation);
			node->localRotation = vector3_scale(vector3_from_ai(rotation), static_cast<float>(180.0 / M_PI));
			node->localScaling = vector3_from_ai(scaling);
			return node;
		}

		AssimpModelImporter::AssimpModelImporter(const ImportParams& params) : Importer(params) { }

		AssimpModelImporter::~AssimpModelImporter() { }

		bool AssimpModelImporter::internalImport(Model* model)
		{
			Assimp::Importer importer;
			importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);
			importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
			const aiScene* assimpScene = importer.ReadFile(this->GetFilename(), aiProcess_MakeLeftHanded | aiProcess_LimitBoneWeights | aiProcess_PopulateArmatureData | aiProcess_GlobalScale);

			this->checkNode(assimpScene, assimpScene->mRootNode, model->root = create_node_from_ai(assimpScene->mRootNode));

			for (uint32_t animationIndex = 0; animationIndex < assimpScene->mNumAnimations; ++animationIndex)
			{
				this->checkAnimation(assimpScene, assimpScene->mAnimations[animationIndex]);
			}

			this->checkSkeleton(assimpScene);

			g_set_string(&model->root->name, std::filesystem::path(this->GetFilename()).replace_extension().filename().string().c_str());

			return true;
		}

		void AssimpModelImporter::checkNode(const aiScene* assimpScene, const aiNode* assimpNode, Node* node)
		{
			assert(assimpNode->mNumMeshes <= 1 && "should optimize mesh collection");
			for (unsigned int i = 0; i < assimpNode->mNumMeshes; ++i)
			{
				const aiMesh* assimpMesh = assimpScene->mMeshes[assimpNode->mMeshes[i]];
				Mesh* mesh = create_mesh(assimpMesh->mName.C_Str());
				this->checkMesh(assimpScene, assimpMesh, mesh);
				mAssimp2MeshMap[assimpMesh] = mesh;
				model_add_mesh(mModel, mesh);
				node->mesh = mesh;
			}

			for (unsigned int i = 0; i < assimpNode->mNumChildren; ++i)
			{
				const aiNode* aiChild = assimpNode->mChildren[i];
				Node* child = create_node_from_ai(aiChild);
				this->checkNode(assimpScene, aiChild, child);
				node_add_child(node, child);
			}

			node->visible = true;
			mAssimp2NodeMap[assimpNode] = node;
			this->registerNode(node);
		}

		void AssimpModelImporter::checkMesh(const aiScene* assimpScene, const aiMesh* assimpMesh, Mesh* mesh)
		{
			const uint32_t vertexCount = assimpMesh->mNumVertices;
			mesh_set_vertex_count(mesh, static_cast<int>(vertexCount));
			const aiVector3D* assimpVertex = assimpMesh->mVertices;
			const aiVector3D* assimpNormal = assimpMesh->mNormals;
			Vertex* vertex = mesh->vertices;
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex, ++vertex, ++assimpVertex, ++assimpNormal)
			{
				vertex->position = vector3_from_ai(*assimpVertex);
				vertex->normal = vector3_from_ai(*assimpNormal);
			}

			const aiFace* assimpFace = assimpMesh->mFaces;
			const uint32_t faceCount = assimpMesh->mNumFaces;
			std::vector<Triangle> triangles;
			triangles.reserve(faceCount * 2);
			Triangle triangle;
			for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex, ++assimpFace)
			{
				if (3u == assimpFace->mNumIndices)
				{
					triangle.index0 = static_cast<int>(assimpFace->mIndices[0]);
					triangle.index1 = static_cast<int>(assimpFace->mIndices[1]);
					triangle.index2 = static_cast<int>(assimpFace->mIndices[2]); 
					triangles.push_back(triangle);
				}
				else if (assimpFace->mNumIndices > 3u)
				{
					triangle.index0 = static_cast<int>(assimpFace->mIndices[0]);
					for (unsigned int i = 2; i < assimpFace->mNumIndices; ++i)
					{
						triangle.index1 = static_cast<int>(assimpFace->mIndices[i - 1]);
						triangle.index2 = static_cast<int>(assimpFace->mIndices[i]);
						triangles.push_back(triangle);
					}
				}
				else
				{
					TRACE_WARN("Should handle this condition, face index count is %u", assimpFace->mNumIndices);
				}
			}
			mesh_set_triangles(mesh, static_cast<int>(triangles.size()), triangles.data());

			const uint32_t& uvSetCount = assimpMesh->GetNumUVChannels();
			for (uint32_t uvSetIndex = 0; uvSetIndex < uvSetCount && uvSetIndex < 4; ++uvSetIndex)
			{
				vertex = mesh->vertices;
				const aiVector3D* aiUV = assimpMesh->mTextureCoords[uvSetIndex];
				for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex, ++vertex, ++aiUV)
				{
					vertex->uv[uvSetIndex].x = aiUV->x;
					vertex->uv[uvSetIndex].y = aiUV->y;
				}
			}

			if (assimpMesh->mMaterialIndex < assimpScene->mNumMaterials)
			{
				const aiMaterial* assimpMaterial = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
				if (assimpMaterial)
				{
					Material* material = create_material(assimpMaterial->GetName().C_Str());
					const uint32_t diffuseCount = assimpMaterial->GetTextureCount(aiTextureType_DIFFUSE);
					assert(diffuseCount <= 1 && "should optimize material collection");
					for (uint32_t textureIndex = 0; textureIndex < diffuseCount; ++textureIndex)
					{
						aiString assimpPath;
						if (aiReturn_SUCCESS == assimpMaterial->GetTexture(aiTextureType_DIFFUSE, textureIndex, &assimpPath))
						{
							std::string filename = this->findFile(assimpPath.C_Str());
							material_set_diffuse(material, filename.c_str(), nullptr);
						}
					}
					mesh_add_material(mesh, material);
					model_add_material(mModel, material);
				}
			}

			for (uint32_t boneIndex = 0; boneIndex < assimpMesh->mNumBones; ++boneIndex)
			{
				mMeshBones[mesh].push_back(assimpMesh->mBones[boneIndex]);
			}
		}

		template <typename AssimpKeyType, typename AssimpKeyDataType, typename GeneralDataType> AnimationCurveNode* animation_curve_from_ai(const aiScene* assimpScene, const uint32_t& keyCount, const AssimpKeyType* assimpAnimationKeys, const double& tickDuration, GeneralDataType(*vector3_from_data)(const AssimpKeyDataType& value), const Node* target, const AnimationCurveNodeType& curveType)
		{
			std::vector<AnimationCurveFrame> frames(keyCount);
			AnimationCurveFrame* frame = frames.data();
			const AssimpKeyType* assimpKey = assimpAnimationKeys;
			for (uint32_t keyIndex = 0; keyIndex < keyCount; ++keyIndex, ++frame, ++assimpKey)
			{
				GeneralDataType data = vector3_from_data(assimpKey->mValue);
				memcpy(frame->data.values, &data, sizeof(GeneralDataType));
				frame->time = static_cast<uint64_t>(assimpKey->mTime * tickDuration * 1000);
			}
			return create_animation_curve_node(target, curveType, static_cast<int>(frames.size()), frames.data());
		}

		void AssimpModelImporter::checkAnimation(const aiScene* assimpScene, const aiAnimation* assimpAnimation)
		{
			CHECK(assimpAnimation, );

			Animation* animation = create_animation(assimpAnimation->mName.C_Str());
			AnimationCurve* animationCurve = const_cast<AnimationCurve*>(animation->curve);			
			for (uint32_t nodeIndex = 0; nodeIndex < assimpAnimation->mNumChannels; ++nodeIndex)
			{
				const aiNodeAnim* assimpNodeAnimation = assimpAnimation->mChannels[nodeIndex];
				Node* node = this->findNode(assimpNodeAnimation->mNodeName.C_Str());
				if (!node)
				{
					TRACE_WARN("Process animation but there is no associated bone %s", assimpNodeAnimation->mNodeName.C_Str());
					continue;
				}

				const double tickDuration = 1.0 / assimpAnimation->mTicksPerSecond;

				if (assimpNodeAnimation->mNumPositionKeys > 0)
				{
					AnimationCurveNode* curveNode = animation_curve_from_ai(assimpScene, assimpNodeAnimation->mNumPositionKeys, assimpNodeAnimation->mPositionKeys, tickDuration, vector3_from_ai, node, AnimationCurveNodeTranslation);
					animation_curve_add_node(animationCurve, curveNode);
				}
				if (assimpNodeAnimation->mNumRotationKeys > 0)
				{
					AnimationCurveNode* curveNode = animation_curve_from_ai(assimpScene, assimpNodeAnimation->mNumRotationKeys, assimpNodeAnimation->mRotationKeys, tickDuration, vector4_from_ai_quaternion, node, AnimationCurveNodeRotation);
					animation_curve_add_node(animationCurve, curveNode);
				}
				if (assimpNodeAnimation->mNumScalingKeys > 0)
				{
					AnimationCurveNode* curveNode = animation_curve_from_ai(assimpScene, assimpNodeAnimation->mNumScalingKeys, assimpNodeAnimation->mScalingKeys, tickDuration, vector3_from_ai, node, AnimationCurveNodeScaling);
					animation_curve_add_node(animationCurve, curveNode);
				}
			}

			model_add_animation(mModel, animation);
		}

		void AssimpModelImporter::checkSkeleton(const aiScene* assimpScene)
		{
			assert(assimpScene->mNumSkeletons <= 1 && "should handle multiple skeletons");
			for (uint32_t skeletonIndex = 0; skeletonIndex < assimpScene->mNumSkeletons; ++skeletonIndex)
			{
				const aiSkeleton* assimpSkeleton = assimpScene->mSkeletons[skeletonIndex];
				for (uint32_t boneIndex = 0; boneIndex < assimpSkeleton->mNumBones; ++boneIndex)
				{
					const aiSkeletonBone* assimpBone = assimpSkeleton->mBones[boneIndex];
					auto meshFinder = mAssimp2MeshMap.find(assimpBone->mMeshId);
					if (mAssimp2MeshMap.end() == meshFinder)
					{
						if (assimpBone->mNode)
						{
							TRACE_WARN("No binding mesh of bone %s", assimpBone->mNode->mName.C_Str());
						}
						continue;
					}

					this->checkBoneWeights(assimpScene, meshFinder->second, assimpBone->mNode, assimpBone->mOffsetMatrix, assimpBone->mNumnWeights, assimpBone->mWeights);
				}
			}

			for (const std::pair<Mesh*, std::vector<aiBone*>>& pair : mMeshBones)
			{
				for (const aiBone* assimpBone : pair.second)
				{
					this->checkBoneWeights(assimpScene, pair.first, assimpBone->mNode, assimpBone->mOffsetMatrix, assimpBone->mNumWeights, assimpBone->mWeights);
				}
			}
		}

		void AssimpModelImporter::checkBoneWeights(const aiScene* assimpScene, Mesh* mesh, const aiNode* assimpBone, const aiMatrix4x4& offsetMatrix, const unsigned int& weightCount, const aiVertexWeight* weights)
		{
			if (!assimpBone)
			{
				return;
			}

			auto boneFinder = mAssimp2NodeMap.find(assimpBone);
			if (mAssimp2NodeMap.end() == boneFinder)
			{
				TRACE_WARN("No bone node of bone %s", assimpBone->mName.C_Str());
				return;
			}

			WeightCollection* weightCollection = create_weight_collection(boneFinder->second, matrix_from_ai(offsetMatrix), static_cast<int>(weightCount));
			WeightData* weight = weightCollection->weights;
			const aiVertexWeight* assimpWeight = weights;
			for (uint32_t weightIndex = 0; weightIndex < weightCount; ++weightIndex, ++weight, ++assimpWeight)
			{
				weight->index = static_cast<int>(assimpWeight->mVertexId);
				weight->weight = static_cast<float>(assimpWeight->mWeight);
			}
			mesh_add_weight_collection(mesh, weightCollection);
		}
	}
}