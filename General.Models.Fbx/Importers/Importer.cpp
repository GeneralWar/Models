#include "pch.h"
#include "Importer.hpp"
#include <fbxsdk.h>
using namespace General;
using namespace fbxsdk;

namespace General
{
	namespace Models
	{
#ifndef FBX_DEFAULT_UV_SET_NAME
#define FBX_DEFAULT_UV_SET_NAME "default"
#endif

		static FbxManager* check_manager()
		{
			static FbxManager* sdk_manager = nullptr;
			if (nullptr == sdk_manager)
			{
				sdk_manager = FbxManager::Create();
				TRACE("FBX SDK version: %s", FbxManager::GetVersion());

				FbxIOSettings* ioSettings = FbxIOSettings::Create(sdk_manager, "IOSRoot");
				sdk_manager->SetIOSettings(ioSettings);
			}
			return sdk_manager;
		}

		Vector3 vector3_from_fbx(const FbxDouble3& property)
		{
			Vector3 v = { };
			v.x = static_cast<float>(property.mData[0]);
			v.y = static_cast<float>(property.mData[1]);
			v.z = static_cast<float>(property.mData[2]);
			return v;
		}

		Vector2 vector2_from_fbx(const FbxDouble2& property)
		{
			Vector2 v = { };
			v.x = static_cast<float>(property.mData[0]);
			v.y = static_cast<float>(property.mData[1]);
			return v;
		}

		Vector3 vector3_from_fbx(const FbxDouble3& property, const float& scaleFactor)
		{
			Vector3 v = { };
			v.x = static_cast<float>(property.mData[0]) * scaleFactor;
			v.y = static_cast<float>(property.mData[1]) * scaleFactor;
			v.z = static_cast<float>(property.mData[2]) * scaleFactor;
			return v;
		}

		Vector4 vector4_from_fbx_quaternion(const FbxQuaternion& quaternion)
		{
			Vector4 v = { };
			v.x = static_cast<float>(quaternion.mData[0]);
			v.y = static_cast<float>(quaternion.mData[1]);
			v.z = static_cast<float>(quaternion.mData[2]);
			v.w = static_cast<float>(quaternion.mData[3]);
			return v;
		}

		Transform transform_from_fbx(FbxAMatrix matrix)
		{
			Transform transform = { };
			transform.translation = vector3_from_fbx(matrix.GetT());
			transform.rotation = vector3_from_fbx(matrix.GetR());
			transform.scaling = vector3_from_fbx(matrix.GetS());
			return transform;
		}

		static double limit(const double& value, const bool& minLimit, const double& min, const bool& maxLimit, const double& max)
		{
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
			return g_clamp(value, minLimit ? min : value, maxLimit ? max : value);
		}

		FbxAMatrix compute_geometry_matrix(const fbxsdk::FbxNode* fbxNode)
		{
			FbxAMatrix geometry;
			geometry.SetT(FbxVector4(fbxNode->GeometricTranslation.Get()));
			geometry.SetR(FbxVector4(fbxNode->GeometricRotation.Get()));
			geometry.SetS(FbxVector4(fbxNode->GeometricScaling.Get()));
			//return node->GetAnimationEvaluator()->GetNodeGlobalTransform(node) * geometry;
			return geometry;
		}

		FbxAMatrix compute_node_total_matrix(fbxsdk::FbxNode* fbxNode)
		{
			return fbxNode->GetAnimationEvaluator()->GetNodeGlobalTransform(fbxNode) * compute_geometry_matrix(fbxNode);
		}

		FbxAMatrix check_node_final_transform(const FbxNode* fbxNode)
		{
			// https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_nodes_computing_transformation_matrix_html
			// T * Roff * Rp * Rpre * R * Rpost -1 * Rp -1 * Soff * Sp * S * Sp -1
			FbxAMatrix translation; translation.SetIdentity(); translation.SetT(fbxNode->LclTranslation.Get());
			FbxAMatrix rotationOffset; rotationOffset.SetIdentity(); rotationOffset.SetR(fbxNode->RotationOffset.Get());
			FbxAMatrix rotationPivot; rotationPivot.SetIdentity(); rotationPivot.SetR(fbxNode->RotationPivot.Get());
			FbxAMatrix preRotation; preRotation.SetIdentity(); preRotation.SetR(fbxNode->PreRotation.Get());
			FbxAMatrix rotation; rotation.SetIdentity();  rotation.SetR(fbxNode->LclRotation.Get());
			FbxAMatrix postRotation; postRotation.SetIdentity(); postRotation.SetR(fbxNode->PostRotation.Get());
			FbxAMatrix rotationPivotInverse = rotationPivot.Inverse();
			FbxAMatrix postRotationInverse = postRotation.Inverse();
			FbxAMatrix scalingOffset; scalingOffset.SetIdentity();// scalingOffset.SetS(fbxNode->ScalingOffset.Get()); // lead to zero
			FbxAMatrix scalingPivot; scalingPivot.SetIdentity();  //scalingPivot.SetS(fbxNode->ScalingPivot.Get()); // lead to zero
			FbxAMatrix scaling; scaling.SetIdentity(); scaling.SetS(fbxNode->LclScaling.Get());
			FbxAMatrix scalingPivotInverse = scalingPivot.Inverse();
			return translation * rotationOffset * rotationPivot * preRotation * rotation * postRotationInverse * rotationPivotInverse * scalingOffset * scalingPivot * scaling * scalingPivotInverse;
		}

		Vector3 rotation_from_fbx(const FbxLimits& limits, const FbxDouble3& preProperty, const FbxDouble3& property, const FbxDouble3& postProperty)
		{
			Vector3 v = { };
			FbxDouble3 minLimit = limits.GetMin();
			FbxDouble3 maxLimit = limits.GetMax();
			v.x = static_cast<float>(limit(preProperty.mData[0] + property.mData[0] + postProperty.mData[0], limits.GetMinXActive(), minLimit[0], limits.GetMaxXActive(), maxLimit[0]));
			v.y = static_cast<float>(limit(preProperty.mData[1] + property.mData[1] + postProperty.mData[1], limits.GetMinYActive(), minLimit[1], limits.GetMaxYActive(), maxLimit[1]));
			v.z = static_cast<float>(limit(preProperty.mData[2] + property.mData[2] + postProperty.mData[2], limits.GetMinZActive(), minLimit[2], limits.GetMaxZActive(), maxLimit[2]));
			return v;
		}

		Vector3 vector3_from_fbx(const FbxLimits& limits, const FbxDouble3& property)
		{
			Vector3 v = { };
			FbxDouble3 minLimit = limits.GetMin();
			FbxDouble3 maxLimit = limits.GetMax();
			v.x = static_cast<float>(limit(property.mData[0], limits.GetMinXActive(), minLimit[0], limits.GetMaxXActive(), maxLimit[0]));
			v.y = static_cast<float>(limit(property.mData[1], limits.GetMinYActive(), minLimit[1], limits.GetMaxYActive(), maxLimit[1]));
			v.z = static_cast<float>(limit(property.mData[2], limits.GetMinZActive(), minLimit[2], limits.GetMaxZActive(), maxLimit[2]));
			return v;
		}

		Vector3 vector3_from_fbx(const FbxLimits& limits, const FbxDouble3& property, const float& scaleFactor)
		{
			Vector3 v = { };
			FbxDouble3 minLimit = limits.GetMin();
			FbxDouble3 maxLimit = limits.GetMax();
			v.x = static_cast<float>(limit(property.mData[0], limits.GetMinXActive(), minLimit[0], limits.GetMaxXActive(), maxLimit[0])) * scaleFactor;
			v.y = static_cast<float>(limit(property.mData[1], limits.GetMinYActive(), minLimit[1], limits.GetMaxYActive(), maxLimit[1])) * scaleFactor;
			v.z = static_cast<float>(limit(property.mData[2], limits.GetMinZActive(), minLimit[2], limits.GetMaxZActive(), maxLimit[2])) * scaleFactor;
			return v;
		}

		FbxAMatrix scale_matrix(const FbxAMatrix& matrix, const float& scale)
		{
			FbxAMatrix scaled;
			FbxDouble4* targetBuffer = scaled.Buffer();
			FbxDouble4* sourceBuffer = scaled.Buffer();
			for (int i = 0; i < 4; ++i, ++sourceBuffer, ++targetBuffer)
			{
				targetBuffer->mData[0] = sourceBuffer->mData[0] * scale;
				targetBuffer->mData[1] = sourceBuffer->mData[1] * scale;
				targetBuffer->mData[2] = sourceBuffer->mData[2] * scale;
				targetBuffer->mData[3] = sourceBuffer->mData[3] * scale;
			}
			return scaled;
		}

		void offset_matrix(FbxAMatrix& matrix, const FbxAMatrix& offset)
		{
			FbxDouble4* sourceBuffer = matrix.Buffer();
			const FbxDouble4* offsetBuffer = offset.Buffer();
			for (int i = 0; i < 4; ++i, ++sourceBuffer, ++offsetBuffer)
			{
				sourceBuffer->mData[0] += offsetBuffer->mData[0];
				sourceBuffer->mData[1] += offsetBuffer->mData[1];
				sourceBuffer->mData[2] += offsetBuffer->mData[2];
				sourceBuffer->mData[3] += offsetBuffer->mData[3];
			}
		}

		Node* create_node_from_fbx(FbxNode* fbxNode, const float& scaleFactor)
		{
			Node* node = create_node(fbxNode->GetName());
			//FbxDouble3 rotation1 = fbxNode->PreRotation.Get();
			//FbxDouble3 rotation2 = fbxNode->LclRotation.Get();
			//FbxDouble3 rotation3 = fbxNode->PostRotation.Get();
			//FbxDouble3 rotation4 = fbxNode->GeometricRotation.Get();
			//FbxDouble3 rotation5 = fbxNode->RotationOffset.Get();
			//FbxDouble3 rotation6 = fbxNode->RotationPivot.Get();
			//FbxAMatrix transform = check_node_final_transform(fbxNode);
			//FbxDouble3 rotation7 = transform.GetR();
			//FbxDouble3 rotation8 = fbxNode->EvaluateLocalRotation();
			node->localPosition = vector3_from_fbx(fbxNode->GetTranslationLimits(), fbxNode->LclTranslation, scaleFactor);
			node->localRotation = rotation_from_fbx(fbxNode->GetRotationLimits(), fbxNode->PreRotation, fbxNode->LclRotation, fbxNode->RotationActive ? fbxNode->PostRotation.Get() : FbxDouble3());
			node->localScaling = vector3_from_fbx(fbxNode->GetScalingLimits(), fbxNode->LclScaling);
			//node->localPosition = vector3_from_fbx(fbxNode->GetTranslationLimits(), fbxNode->GeometricTranslation, context->scaleFactor);
			//node->localRotation = vector3_from_fbx(fbxNode->GetRotationLimits(), fbxNode->GeometricRotation);
			//node->localScaling = vector3_from_fbx(fbxNode->GetScalingLimits(), fbxNode->GeometricScaling);
			//node->localPosition = vector3_from_fbx(fbxNode->GetTranslationLimits(), fbxNode->LclTranslation, context->scaleFactor);
			//node->localRotation = vector3_from_fbx(fbxNode->GetRotationLimits(), fbxNode->LclRotation);
			//node->localScaling = vector3_from_fbx(fbxNode->GetScalingLimits(), fbxNode->LclScaling);
			//node->localPosition = vector3_from_fbx(fbxNode->GetTranslationLimits(), transform.GetT(), context->scaleFactor);
			//node->localRotation = vector3_from_fbx(fbxNode->GetRotationLimits(), transform.GetR());
			//node->localScaling = vector3_from_fbx(fbxNode->GetScalingLimits(), transform.GetS());
			return node;
		}

		FbxAMatrix compute_bone_matrix(FbxCluster* cluster, FbxMesh* mesh)
		{
			FbxAMatrix matrix;
			matrix.SetIdentity();
			CHECK(cluster && mesh, matrix);
			CHECK_INSTANCE(FbxNode*, linkNode, cluster->GetLink(), matrix);

			FbxAMatrix transformMatrix;
			FbxAMatrix transformLinkMatrix;
			cluster->GetTransformMatrix(transformMatrix); // The transformation of the mesh at binding time 
			cluster->GetTransformLinkMatrix(transformLinkMatrix); // The transformation of the cluster(joint) at binding time from joint space to world space 
			FbxAMatrix transformRelativeMatrix = transformLinkMatrix.Inverse() * transformMatrix;

			FbxAMatrix meshGlobalMatrix = compute_node_total_matrix(mesh->GetNode());
			FbxAMatrix linkPoseMatrix = linkNode->GetAnimationEvaluator()->GetNodeGlobalTransform(linkNode, 0);
			FbxAMatrix boneRelativeInverse = meshGlobalMatrix.Inverse() * linkPoseMatrix;
			//matrix = matrix * boneRelativeInverse;
			matrix = matrix * transformRelativeMatrix;
			matrix = matrix * compute_geometry_matrix(linkNode);
			return matrix;
		}

		FbxSystemUnit check_preferred_unit(const UnitLevel& level)
		{
			switch (level)
			{
			case UNIT_LEVEL_MILLIMETER:
				return FbxSystemUnit::mm;
			case UNIT_LEVEL_DECIMITER:
				return FbxSystemUnit::dm;
			case UNIT_LEVEL_METER:
				return FbxSystemUnit::m;
			default:
				return FbxSystemUnit::cm;
			}
		}

		static UVSet* uvset_from_fbx(const FbxMesh* fbxMesh, const FbxGeometryElementUV* elementUV)
		{
			FbxLayerElement::EMappingMode mappingMode = elementUV->GetMappingMode();
			// only support mapping mode eByPolygonVertex and eByControlPoint
			if (FbxGeometryElement::eByPolygonVertex != mappingMode && FbxGeometryElement::eByControlPoint != mappingMode)
			{
				return nullptr;
			}

			const int polygonCount = fbxMesh->GetPolygonCount();
			const int vertexCount = fbxMesh->GetPolygonVertexCount();
			const int* indices = fbxMesh->GetPolygonVertices();

			UVSet* uvSet = create_uv_set(elementUV->GetName());
			uv_set_set_uv_count(uvSet, polygonCount * 3); // must calls FbxGeometryConverter::Triangulate first

			const FbxLayerElementArrayTemplate<int>& indexArray = elementUV->GetIndexArray();
			const FbxLayerElementArrayTemplate<FbxVector2>& directArray = elementUV->GetDirectArray();
			//index array, where holds the index referenced to the uv data
			const bool useIndex = elementUV->GetReferenceMode() != FbxGeometryElement::eDirect;
			assert(!useIndex || uvSet->uvCount == indexArray.GetCount());
			//const int indexCount = (useIndex) ? elementUV->GetIndexArray().GetCount() : 0;
			//iterating through the data by polygon
			if (FbxGeometryElement::eByControlPoint == mappingMode)
			{
				UV* uv = uvSet->uvArray;
				assert(!"TODO: ensure this condition");
				for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					const int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
					for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; ++polygonVertexIndex, ++uv)
					{
						int vertexIndex = fbxMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);
						int uvIndex = useIndex ? indexArray.GetAt(vertexIndex) : vertexIndex;
						FbxVector2 uvValue = directArray.GetAt(uvIndex);
						uv->vertexIndex = *(indices + polygonIndex * 3 + polygonVertexIndex);
						uv->uv.x = static_cast<float>(uvValue.mData[0]);
						uv->uv.y = static_cast<float>(uvValue.mData[1]);
					}
				}
			}
			else if (FbxGeometryElement::eByPolygonVertex == mappingMode)
			{
				int polygonIndexCounter = 0;
				const int* vertexIndices = fbxMesh->GetPolygonVertices();
				for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					const int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
					const int vertexStartIndex = fbxMesh->GetPolygonVertexIndex(polygonIndex);
					for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; ++polygonVertexIndex)
					{
						int uvIndex = useIndex ? indexArray.GetAt(polygonIndexCounter) : polygonIndexCounter;
						UV* uv = uvSet->uvArray + polygonIndexCounter;
						FbxVector2 uvValue = directArray.GetAt(uvIndex);
						uv->vertexIndex = *(indices + polygonIndex * 3 + polygonVertexIndex);
						uv->uv.x = static_cast<float>(uvValue.mData[0]);
						uv->uv.y = static_cast<float>(uvValue.mData[1]);
						polygonIndexCounter++;
					}
				}
			}
			return uvSet;
		}

		FbxModelImporter::FbxModelImporter(const ImportParams& params) : Importer(params), mManager(), mImporter() { }

		FbxModelImporter::~FbxModelImporter()
		{
			if (mImporter)
			{
				mImporter->Destroy();
				mImporter = nullptr;
			}
			if (mManager)
			{
				mManager->Destroy();
				mManager = nullptr;
			}
		}

		bool FbxModelImporter::checkImportStatus()
		{
			FbxImporter* importer = mImporter;
			if (!importer)
			{
				const char* error = "no " STRING(fbxsdk::FbxImpoter) "!";
				TRACE_ERROR(error);
				this->setError(error);
				return false;
			}

			FbxStatus status = importer->GetStatus();
			if (status.Error())
			{
				FbxString error = status.GetErrorString();
				TRACE_ERROR(error.Buffer());
				this->setError(error.Buffer());
				return false;
			}
			if (!importer->IsFBX())
			{
				char error[128];
				sprintf(error, "%s is not a fbx file", this->GetFilename().c_str());
				TRACE_ERROR(error);
				this->setError(error);
				return false;
			}
			return true;
		}

		bool FbxModelImporter::internalImport(Model* model)
		{
			FbxManager* manager = mManager = check_manager();
			FbxImporter* importer = mImporter = FbxImporter::Create(manager, "Importer");

			const std::string& filename = this->GetFilename();
			bool status = importer->Initialize(filename.c_str(), -1, manager->GetIOSettings());
			if (!status)
			{
				this->checkImportStatus();
				return false;
			}

			fbxsdk::FbxScene* scene = FbxScene::Create(mManager, "ImportScene");
			this->import(scene);
			scene->Destroy();

			return true;
		}

		void FbxModelImporter::import(FbxScene* scene)
		{
			Model* model = mModel;
			FbxManager* manager = mManager;
			FbxImporter* importer = mImporter;

			if (!importer->Import(scene))
			{
				if (!this->checkImportStatus())
				{
					return;
				}
				assert(!"unexpected condition");
			}

			// https://help.autodesk.com/view/FBX/2020/ENU/?guid=FBX_Developer_Help_nodes_and_scene_graph_fbx_scenes_scene_axis_and_unit_conversion_html
			// Objects in the FBX SDK are always created in the right handed, Y-Up axis system. The scene's axis system may need to be converted to suit your application's needs. Consult the FbxAxisSystem class documentation for more information.
			fbxsdk::FbxAxisSystem::DirectX.ConvertScene(scene);

			FbxGeometryConverter converter(manager);
			converter.Triangulate(scene, true);

			FbxSystemUnit preferredUnit = check_preferred_unit(this->GetUnitLevel());
			//if (preferredUnit != scene->GetGlobalSettings().GetSystemUnit())
			//{
			//	FbxSystemUnit::ConversionOptions options = { };
			//	options.mConvertClusters = true;
			//	options.mConvertLimits = true;
			//	preferredUnit.ConvertScene(scene, options);
			//}
			mScaleFactor = static_cast<float>(1.0 / preferredUnit.GetScaleFactor());

			FbxNode* root = scene->GetRootNode();
			if (nullptr == root)
			{
				TRACE_ERROR("no root node");
				return;
			}

			this->checkAnimations(scene);

			this->checkNode(mModel->root = create_node_from_fbx(root, mScaleFactor), root);
			g_set_string(&model->root->name, std::filesystem::path(this->GetFilename()).replace_extension().filename().string().c_str());

			this->checkSkinWeights();

			this->postProcess();
		}

		void FbxModelImporter::checkNode(Node* node, FbxNode* fbxNode)
		{
			node->visible = fbxNode->GetVisibility();

			FbxMesh* fbxMesh = fbxNode->GetMesh();
			if (nullptr != fbxMesh)
			{
				Mesh* mesh = node->mesh = this->checkMesh(fbxMesh);
				assert(mesh);

				model_add_mesh(mModel, mesh);

				int materialCount = fbxNode->GetMaterialCount();
				for (int i = 0; i < materialCount; ++i)
				{
					FbxSurfaceMaterial* fbxMaterial = fbxNode->GetMaterial(i);
					Material* material = this->checkMaterial(fbxMaterial);
					if (nullptr != material)
					{
						mesh_add_material(mesh, material);
					}
				}
			}

			this->checkNodeAnimationFrames(fbxNode, node);

			int childCount = fbxNode->GetChildCount();
			for (int i = 0; i < childCount; ++i)
			{
				FbxNode* fbxChildNode = fbxNode->GetChild(i);
				Node* childNode = create_node_from_fbx(fbxChildNode, mScaleFactor);
				this->checkNode(childNode, fbxChildNode);
				node_add_child(node, childNode);
			}

			mFbx2NodeMap[fbxNode] = node;
		}

		static bool check_uv_different(const Vertex* vertex, const Vector2& uv)
		{
			return .0f != vertex->uv[0].x && vertex->uv[0].x != uv.x && .0f != vertex->uv[0].y && vertex->uv[0].y != uv.y;
		}

		static bool check_normal_different(const Vertex* vertex, const Normal* normal)
		{
			return .0f != vertex->normal.x && vertex->normal.x != normal->normal.x && .0f != vertex->normal.y && vertex->normal.y != normal->normal.y && .0f != vertex->normal.z && vertex->normal.z != normal->normal.z;
		}

		Mesh* FbxModelImporter::checkMesh(FbxMesh* fbxMesh)
		{
			if (nullptr == fbxMesh)
			{
				return nullptr;
			}

			fbxMesh->RemoveBadPolygons();

			const char* meshName = fbxMesh->GetName();
			if (nullptr == meshName || 0 == strlen(meshName))
			{
				meshName = fbxMesh->GetNode()->GetName();
			}

			Mesh* mesh = create_mesh(meshName);
			this->checkMeshVertices(fbxMesh, mesh);
			this->checkMeshIndices(fbxMesh, mesh);
			this->checkMeshUVs(fbxMesh, mesh);
			mMesh2FbxMap[mesh] = fbxMesh;
			return mesh;
		}

		/// <returns>vertex count</returns>
		int FbxModelImporter::checkMeshVertices(const FbxMesh* fbxMesh, Mesh* mesh)
		{
			int vertexCount = fbxMesh->GetControlPointsCount();
			mesh_set_vertex_count(mesh, vertexCount);

			FbxAMatrix matrix = compute_geometry_matrix(fbxMesh->GetNode());
			//FbxAMatrix matrix = check_node_final_transform(fbxMesh->GetNode()).Inverse();

			//FbxAMatrix matrix;
			//FbxNode* fbxNode = fbxMesh->GetNode();
			//matrix.SetT(fbxNode->GeometricTranslation.Get());
			//matrix.SetR(fbxNode->GeometricRotation.Get());
			//matrix.SetS(fbxNode->GeometricScaling.Get());
			FbxVector4* point = fbxMesh->GetControlPoints();
			for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex, ++point)
			{
				Vertex* vertex = mesh->vertices + vertexIndex;
				//vertex->position = vector3_from_fbx(*point, context->scaleFactor); 
				vertex->position = vector3_from_fbx(matrix.MultT(FbxVector4(point->mData[0], point->mData[1], point->mData[2], 1.0)), mScaleFactor);
			}
			return vertexCount;
		}

		/// <returns>index count</returns>
		int FbxModelImporter::checkMeshIndices(const FbxMesh* fbxMesh, Mesh* mesh)
		{
			int indexCount = 0;
			int polygonCount = fbxMesh->GetPolygonCount();
			for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
			{
				int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
#ifdef DEBUG
				assert(3 == polygonSize && "unexpected vertex count");
#endif
				indexCount += polygonSize;
			}

			FbxVector4 fbxNormal;
			mMeshNormals[mesh].resize(indexCount);
			Normal* normals = mMeshNormals[mesh].data();
			mesh_set_triangle_count(mesh, polygonCount);
			FbxAMatrix matrix = compute_geometry_matrix(fbxMesh->GetNode()).Inverse().Transpose();
			for (int polygonIndex = 0, i = 0; polygonIndex < polygonCount; ++polygonIndex, ++i)
			{
				Triangle* triangle = mesh->triangles + i;
				int startIndex = fbxMesh->GetPolygonVertexIndex(polygonIndex);
				int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
				if (startIndex > -1)
				{
					memcpy(&triangle->index0, fbxMesh->GetPolygonVertices() + startIndex, sizeof(int) * polygonSize);
					for (int i = 0, n = startIndex; i < polygonSize; ++i, ++n)
					{
						if (fbxMesh->GetPolygonVertexNormal(polygonIndex, i, fbxNormal))
						{
							(normals + n)->normal = vector3_from_fbx(matrix.MultT(FbxVector4(fbxNormal.mData[0], fbxNormal.mData[1], fbxNormal.mData[2], .0)));
						}
					}
				}
				else
				{
					DebugBreak();
				}
			}
			return indexCount;
		}

		void FbxModelImporter::checkMeshUVs(const FbxMesh* fbxMesh, Mesh* mesh)
		{
			FbxStringList uvSetNameList;
			fbxMesh->GetUVSetNames(uvSetNameList);
			int uvSetCount = uvSetNameList.GetCount();
			for (int i = 0; i < uvSetCount; ++i)
			{
				const char* uvSetName = uvSetNameList.GetStringAt(i);
				const FbxGeometryElementUV* elementUV = fbxMesh->GetElementUV(uvSetName);
				if (!elementUV)
				{
					continue;
				}

				mFbxUVSets[uvSetName] = elementUV;
			}
		}

		Material* FbxModelImporter::checkMaterial(/*const */FbxSurfaceMaterial* fbxMaterial)
		{
			Model* model = mModel;
			if (nullptr == fbxMaterial || nullptr == model)
			{
				return nullptr;
			}

			const char* materialName = fbxMaterial->GetName();
			Material* material = find_material(model->materials, model->materialCount, materialName);
			if (nullptr == material)
			{
				material = create_material(materialName);
				model_add_material(model, material);

				FbxSurfaceLambert* lambert = FbxCast<FbxSurfaceLambert>(fbxMaterial);
				if (nullptr == lambert)
				{
					assert(!"ensure this condition");
					FbxProperty property = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
					if (property.IsValid())
					{
						this->checkPropertyForTexture(material, &property);
					}
					else
					{
						this->enumerateMaterialPropertyForTexture(material, fbxMaterial);
					}
				}
				else
				{
					FbxFileTexture* ambient = lambert->Ambient.GetSrcObject<FbxFileTexture>();
					if (nullptr != ambient)
					{
						material_set_ambient(material, ambient->GetFileName(), ambient->UVSet.Get());
						material->ambient->useDefaultUVSet = 0 == strcmp(material->ambient->uvSet, FBX_DEFAULT_UV_SET_NAME);
					}
					FbxFileTexture* diffuse = lambert->Diffuse.GetSrcObject<FbxFileTexture>();
					if (nullptr != diffuse)
					{
						material_set_diffuse(material, diffuse->GetFileName(), diffuse->UVSet.Get());
						material->diffuse->useDefaultUVSet = 0 == strcmp(material->diffuse->uvSet, FBX_DEFAULT_UV_SET_NAME);
					}
					FbxFileTexture* emissive = lambert->Emissive.GetSrcObject<FbxFileTexture>();
					if (nullptr != emissive)
					{
						material_set_emissive(material, emissive->GetFileName(), emissive->UVSet.Get());
						material->emissive->useDefaultUVSet = 0 == strcmp(material->emissive->uvSet, FBX_DEFAULT_UV_SET_NAME);
					}

					FbxSurfacePhong* phong = FbxCast<FbxSurfacePhong>(fbxMaterial);
					if (nullptr != phong)
					{
						FbxFileTexture* specular = phong->Specular.GetSrcObject<FbxFileTexture>();
						if (nullptr != specular)
						{
							material_set_specular(material, specular->GetFileName(), specular->UVSet.Get());
							material->specular->useDefaultUVSet = 0 == strcmp(material->specular->uvSet, FBX_DEFAULT_UV_SET_NAME);
						}
					}
				}
			}
			return material;
		}

		void FbxModelImporter::enumerateMaterialPropertyForTexture(Material* material, /*const */FbxSurfaceMaterial* fbxMaterial)
		{
			FbxProperty property = fbxMaterial->GetFirstProperty();
			while (property.IsValid())
			{
				this->checkPropertyForTexture(material, &property);
				property = fbxMaterial->GetNextProperty(property);
			}
		}

		void FbxModelImporter::checkPropertyForTexture(Material* material, const FbxProperty* property)
		{
			const char* propertyName = property->GetNameAsCStr();
			if (!property->IsValid())
			{
				return;
			}

			FbxLayeredTexture* fbxLayeredTexture = property->GetSrcObject<FbxLayeredTexture>();
			if (nullptr != fbxLayeredTexture)
			{
				int sourceCount = fbxLayeredTexture->GetSrcObjectCount();
				assert(!"Should handle multiple textures");
				return;
			}

			FbxTexture* fbxTexture = property->GetSrcObject<FbxTexture>();
			const char* filename = this->checkTexture(material, fbxTexture);
			if (nullptr == filename || 0 == strlen(filename))
			{
				return;
			}

			if (0 == strcmp("DiffuseTexture", propertyName))
			{
				material_set_diffuse(material, filename, fbxTexture->UVSet.Get());
				material->diffuse->useDefaultUVSet = 0 == strcmp(material->diffuse->uvSet, FBX_DEFAULT_UV_SET_NAME);
				return;
			}

			TRACE_WARN("Should handle other texture: %s", propertyName);
		}

		const char* FbxModelImporter::checkTexture(Material* material, FbxTexture* fbxTexture)
		{
			if (nullptr == fbxTexture)
			{
				return nullptr;
			}

			FbxFileTexture* fbxFileTexture = FbxCast<FbxFileTexture>(fbxTexture);
			if (nullptr == fbxFileTexture)
			{
				assert(!"Should handle this");
				return nullptr;
			}

			return fbxFileTexture->GetRelativeFileName();
		}

		void FbxModelImporter::checkAnimations(FbxScene* scene)
		{
			CHECK_INSTANCE(Model*, model, mModel, );

			int animationStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
			if (animationStackCount > 0)
			{
				//context->animationEvaluator = scene->GetAnimationEvaluator();
				for (int stackIndex = 0; stackIndex < animationStackCount; ++stackIndex)
				{
					FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(stackIndex);
					if (!stack)
					{
						continue;
					}

					mAnimationStacks.push_back(stack);

					const int layerCount = stack->GetMemberCount<FbxAnimLayer>();
					for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
					{
						FbxAnimLayer* layer = stack->GetMember<FbxAnimLayer>(layerIndex);
						mAnimationLayers.push_back(layer);

						const char* layerName = layer->GetName();
						Animation* animation = create_animation(layerName);
						mAnimations.push_back(animation);
						model_add_animation(model, animation);

						/*const int nodeCount = layer->GetMemberCount<FbxAnimCurveNode>();
						for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
						{
							FbxAnimCurveNode* node = layer->GetMember<FbxAnimCurveNode>(nodeIndex);
							const char* curveNodeName = node->GetName();
							if (!node->IsAnimated(true))
							{
								continue;
							}

							assert(1 == node->GetDstPropertyCount() && "unexpected condition");
							FbxProperty property = node->GetDstProperty(0);
							FbxNode* ownerNode = FbxCast<FbxNode>(property.GetFbxObject());
							const char* nodeName = ownerNode->GetName();
							printf(STRING(FbxAnimCurveNode) " %s for %s\n", curveNodeName, nodeName);

							const unsigned int channelCount = node->GetChannelsCount();
							for (unsigned int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
							{
								unsigned int curveCount = node->GetCurveCount(channelIndex);
								for (unsigned int curveIndex = 0; curveIndex < curveCount; ++curveIndex)
								{
									FbxAnimCurve* curve = node->GetCurve(curveIndex);
									const char* curveName = curve->GetName();
									const int keyCount = curve->KeyGetCount();
									for (int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
									{
										FbxAnimCurveKey key = curve->KeyGet(keyIndex);
										const long long int timeMilliSeconds = key.GetTime().GetMilliSeconds();
										const float value = key.GetValue();
										const FbxAnimCurveDef::EInterpolationType interpolationType = key.GetInterpolation();
										const FbxAnimCurveDef::ETangentMode tangentMode = key.GetTangentMode();
										const FbxAnimCurveDef::ETangentVisibility tangentVisibility = key.GetTangentVisibility();
										TRACE("");
									}
								}
								const float v0 = node->GetChannelValue<float>(0u, .0f);
								const float v1 = node->GetChannelValue<float>(1u, .0f);
								const float v2 = node->GetChannelValue<float>(2u, .0f);
							}
						}*/
					}
				}
			}
		}

		void offset_rotation_curve(AnimationCurveNode* curveNode, const FbxLimits& limits, const FbxDouble3& preProperty, const FbxDouble3& postProperty)
		{
			Vector3 v = { };
			FbxDouble3 minLimit = limits.GetMin();
			FbxDouble3 maxLimit = limits.GetMax();
			AnimationCurveFrame* frame = const_cast<AnimationCurveFrame*>(curveNode->frames);
			for (int frameIndex = 0; frameIndex < curveNode->frameCount; ++frameIndex, ++frame)
			{
				frame->data.vector3.x = static_cast<float>(limit(preProperty.mData[0] + frame->data.vector3.x + postProperty.mData[0], limits.GetMinXActive(), minLimit[0], limits.GetMaxXActive(), maxLimit[0]));
				frame->data.vector3.y = static_cast<float>(limit(preProperty.mData[1] + frame->data.vector3.y + postProperty.mData[1], limits.GetMinYActive(), minLimit[1], limits.GetMaxYActive(), maxLimit[1]));
				frame->data.vector3.z = static_cast<float>(limit(preProperty.mData[2] + frame->data.vector3.z + postProperty.mData[2], limits.GetMinZActive(), minLimit[2], limits.GetMaxZActive(), maxLimit[2]));
			}
		}

		void offset_animation_curve_node(AnimationCurveNode* curveNode, const FbxDouble3& offset)
		{
			AnimationCurveFrame* frame = const_cast<AnimationCurveFrame*>(curveNode->frames);
			for (int frameIndex = 0; frameIndex < curveNode->frameCount; ++frameIndex, ++frame)
			{
				frame->data.vector3.x = static_cast<float>(frame->data.vector3.x + offset.mData[0]);
				frame->data.vector3.y = static_cast<float>(frame->data.vector3.y + offset.mData[1]);
				frame->data.vector3.z = static_cast<float>(frame->data.vector3.z + offset.mData[2]);
			}
		}

		void post_process_animation_rotation_curve_node(AnimationCurveNode* curveNode)
		{
			Vector4 lastQuaternion = { };
			AnimationCurveFrame* frame = const_cast<AnimationCurveFrame*>(curveNode->frames);
			for (int frameIndex = 0; frameIndex < curveNode->frameCount; ++frameIndex, ++frame)
			{
				FbxAMatrix matrix;
				matrix.SetR(FbxVector4(frame->data.vector4.x, frame->data.vector4.y, frame->data.vector4.z, 0));
				FbxQuaternion quaternion = matrix.MultQ(FbxQuaternion());
				// take shortest path by checking the inner product (Copy from Assimp v5.2.5 FBXConverter.cpp:3530)
				// http://www.3dkingdoms.com/weekly/weekly.php?a=36
				if (quaternion.mData[0] * lastQuaternion.x + quaternion.mData[1] * lastQuaternion.y + quaternion.mData[2] * lastQuaternion.z + quaternion.mData[3] * lastQuaternion.w < 0) {
					quaternion.Conjugate();
					quaternion.mData[3] = -quaternion.mData[3];
				}
				lastQuaternion = frame->data.vector4 = vector4_from_fbx_quaternion(quaternion);
			}
		}

		template <typename TimeAndValue> static void fill_time_and_value(const FbxAnimCurve* curve, std::vector<TimeAndValue>& values, std::set<FbxTime>& timeSet, const float& scaleFactor)
		{
			int count = curve->KeyGetCount();

			values.resize(count);
			TimeAndValue* timeValue = values.data();

			for (int keyIndex = 0; keyIndex < count; ++keyIndex, ++timeValue)
			{
				timeValue->time = curve->KeyGetTime(keyIndex);
				timeValue->value = curve->KeyGetValue(keyIndex) * scaleFactor;
				timeSet.insert(timeValue->time);
			}
		}

		template <typename TimeAndValue> static inline float lerp(const FbxTime& time, const TimeAndValue* data1, const TimeAndValue* data2)
		{
			double percent = (time.Get() - data1->time.Get()) * 1.0 / (data2->time.Get() - data1->time.Get());
			return static_cast<float>(data1->value + (data2->value - data1->value) * percent);
		}

		template <typename TimeAndValue> static float check_frame_value(const FbxTime& currentTime, const TimeAndValue* data, bool* moveToNext)
		{
			*moveToNext = false;

			if (currentTime == data->time)
			{
				*moveToNext = true;
				return data->value;
			}

			if (data->time < currentTime)
			{
				const TimeAndValue* nextData = data + 1; // might lead to overflow
				*moveToNext = nextData->time >= currentTime;
				return lerp(currentTime, data, nextData);
			}
			else
			{
				const TimeAndValue* prevData = data - 1; // might lead to overflow
				return lerp(currentTime, prevData, data);
			}
		}

		AnimationCurveNode* analyze_animation_node_frames(FbxAnimCurve* curveX, FbxAnimCurve* curveY, FbxAnimCurve* curveZ, FbxNode* fbxNode, const Node* target, const AnimationCurveNodeType& curveType, const float& scaleFactor)
		{
			assert((!!curveX) == (!!curveY) && (!!curveX) == (!!curveZ)); // 要么都有，要么都没有
			CHECK(curveX || curveY || curveZ, nullptr);

			struct TimeAndValue
			{
				FbxTime time;
				float value;
			};

			std::set<FbxTime> timeSet;

			FbxTime time;
			std::vector<TimeAndValue> xValues;
			fill_time_and_value(curveX, xValues, timeSet, scaleFactor);

			std::vector<TimeAndValue> yValues;
			fill_time_and_value(curveY, yValues, timeSet, scaleFactor);

			std::vector<TimeAndValue> zValues;
			fill_time_and_value(curveZ, zValues, timeSet, scaleFactor);

			std::vector<FbxTime> times;
			std::copy(timeSet.begin(), timeSet.end(), std::back_inserter(times));
			std::sort(times.begin(), times.end());

			TimeAndValue *xValue = xValues.data(), *yValue = yValues.data(), *zValue = zValues.data();
			std::vector<AnimationCurveFrame> frames(times.size());
			AnimationCurveFrame* frame = frames.data();
			FbxTime currentTime;
			bool moveToNext;
			for (std::vector<FbxTime>::iterator iterator = times.begin(); times.end() != iterator; ++iterator, ++frame)
			{
				currentTime = *iterator;
				frame->data.vector4.x = check_frame_value(currentTime, xValue, &moveToNext);
				if (moveToNext) ++xValue;
				frame->data.vector4.y = check_frame_value(currentTime, yValue, &moveToNext);
				if (moveToNext) ++yValue;
				frame->data.vector4.z = check_frame_value(currentTime, zValue, &moveToNext);
				if (moveToNext) ++zValue;

				frame->time = currentTime.GetMilliSeconds();
			}
			assert(xValue == xValues.data() + xValues.size() && yValue == yValues.data() + yValues.size() && zValue == zValues.data() + zValues.size());
			return create_animation_curve_node(target, curveType, static_cast<int>(frames.size()), frames.data());
		}

		void FbxModelImporter::checkNodeAnimationFrames(FbxNode* fbxNode, Node* node)
		{
			CHECK(fbxNode, );
			CHECK_INSTANCE(Model*, model, mModel, );
			assert(mAnimationLayers.size() == mAnimations.size());

			for (size_t i = 0; i < mAnimationLayers.size(); ++i)
			{
				Animation* animation = mAnimations[i];
				AnimationCurve* curve = const_cast<AnimationCurve*>(animation->curve);
				FbxAnimLayer* layer = mAnimationLayers[i];
				AnimationCurveNode* curveNode = analyze_animation_node_frames(fbxNode->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X, false), fbxNode->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y, false), fbxNode->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z, false), fbxNode, node, AnimationCurveNodeTranslation, mScaleFactor);
				if (curveNode)
				{
					animation_curve_add_node(curve, curveNode);
				}
				curveNode = analyze_animation_node_frames(fbxNode->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X, false), fbxNode->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y, false), fbxNode->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z, false), fbxNode, node, AnimationCurveNodeRotation, 1.0f);
				if (curveNode)
				{
					offset_rotation_curve(curveNode, fbxNode->GetRotationLimits(), fbxNode->PreRotation, fbxNode->RotationActive ? fbxNode->PostRotation : FbxDouble3());
					//offset_animation_curve_node(curveNode, fbxNode->RotationOffset);
					//offset_animation_curve_node(curveNode, fbxNode->RotationPivot);
					//offset_animation_curve_node(curveNode, fbxNode->PreRotation);
					//if (fbxNode->RotationActive)
					//{
					//	offset_animation_curve_node(curveNode, fbxNode->PostRotation);
					//}
					
					post_process_animation_rotation_curve_node(curveNode);/*
					const AnimationCurveFrame* frame = curveNode->frames;
					for (int frameIndex = 0; frameIndex < curveNode->frameCount; ++frameIndex, ++frame)
					{
						FbxQuaternion q;
						Vector4 quaternion;
						q.ComposeSphericalXYZ(FbxVector4(frame->data.vector3.x, frame->data.vector3.y, frame->data.vector3.z));
						quaternion.x = static_cast<float>(q.mData[0]);
						quaternion.y = static_cast<float>(q.mData[1]);
						quaternion.z = static_cast<float>(q.mData[2]);
						quaternion.w = static_cast<float>(q.mData[3]);

						if (0 == strcmp("LeftArm", node->name))
						{
							DebugBreak();
							int n = 0;
						}
					}*/
					animation_curve_add_node(curve, curveNode);
				}
				curveNode = analyze_animation_node_frames(fbxNode->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X, false), fbxNode->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y, false), fbxNode->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z, false), fbxNode, node, AnimationCurveNodeScaling, 1.0f);
				if (curveNode)
				{
					animation_curve_add_node(curve, curveNode);
				}
			}
		}

		void FbxModelImporter::checkSkinWeights()
		{
			for (const auto& pair : mMesh2FbxMap)
			{
				Mesh* mesh = pair.first;
				FbxMesh* fbxMesh = pair.second;

				//std::vector<FbxAMatrix> transformMatrices(mesh->vertexCount);
				//memset(transformMatrices.data(), 0, sizeof(FbxAMatrix) * transformMatrices.size());

				int deformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
				for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex)
				{
					FbxSkin* fbxSkin = FbxCast<FbxSkin>(fbxMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
					if (!fbxSkin)
					{
						continue;
					}

					int clusterCount = fbxSkin->GetClusterCount();
					for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
					{
						FbxCluster* cluster = fbxSkin->GetCluster(clusterIndex);
						FbxCluster::ELinkMode linkMode = cluster->GetLinkMode();
						FbxNode* fbxLinkNode = cluster->GetLink();
						if (!fbxLinkNode)
						{
							TRACE_WARN("No link node with cluster %s", cluster->GetName());
							continue;
						}

						const char* boneName = fbxLinkNode->GetName();
						auto nodeFinder = mFbx2NodeMap.find(fbxLinkNode);
						if (mFbx2NodeMap.end() == nodeFinder)
						{
							TRACE_ERROR("No imported node corresponded with link node %s", fbxLinkNode->GetName());
							continue;
						}

						int indexCount = cluster->GetControlPointIndicesCount();
						FbxAMatrix boneOffsetMatrix = compute_bone_matrix(cluster, fbxMesh);
						WeightCollection* weightCollection = create_weight_collection(nodeFinder->second, transform_from_fbx(boneOffsetMatrix), indexCount);

						FbxSubDeformer::EType subType = cluster->GetSubDeformerType();
						assert(FbxSubDeformer::EType::eCluster == subType && "should handle other types");

						int* indices = cluster->GetControlPointIndices();
						double* weights = cluster->GetControlPointWeights();
						WeightData* weight = weightCollection->weights;
						FbxAMatrix boneOffsetMatrixInverse = boneOffsetMatrix.Inverse();
						for (int index = 0; index < indexCount; ++index, ++indices, ++weights, ++weight)
						{
							weight->index = *indices;
							weight->weight = static_cast<float>(*weights);

							//offset_matrix(transformMatrices[weight->index], scale_matrix(boneOffsetMatrixInverse, weight->weight));
						}
						mesh_add_weight_collection(mesh, weightCollection);
					}
				}

				//Vertex* vertex = mesh->vertices;
				//FbxAMatrix* transformMatrix = transformMatrices.data();
				//for (int i = 0; i < mesh->vertexCount; ++i, ++vertex, ++transformMatrix)
				//{
				//	vertex->position = vector3_from_fbx(transformMatrix->MultT(FbxVector4(vertex->position.x, vertex->position.y, vertex->position.z, 1.0f)));
				//}

				// check total weight is 1.0f
				std::vector<float> totalWeights(mesh->vertexCount);
				for (int collectionIndex = 0; collectionIndex < mesh->weightCollectionCount; ++collectionIndex)
				{
					const WeightCollection* collection = mesh->weightCollections[collectionIndex];
					for (int weightIndex = 0; weightIndex < collection->weightCount; ++weightIndex)
					{
						const WeightData* weightData = collection->weights + weightIndex;
						float* value = totalWeights.data() + weightData->index;
						*value += weightData->weight;
					}
				}
				for (size_t vertexIndex = 0; vertexIndex < totalWeights.size(); ++vertexIndex)
				{
					if (totalWeights[vertexIndex] && 1.0f != totalWeights[vertexIndex])
					{
						printf("Invalid vertex weight %1.9f for mesh %s index %llu\n", totalWeights[vertexIndex], mesh->name, vertexIndex);
					}
				}
			}
		}

		void FbxModelImporter::postProcess()
		{
			CHECK_INSTANCE(Model*, model, mModel, );

			for (int meshIndex = 0; meshIndex < model->meshCount; ++meshIndex)
			{
				Mesh* mesh = model->meshes[meshIndex];

				const auto meshFinder = mMesh2FbxMap.find(mesh);
				if (mMesh2FbxMap.end() == meshFinder)
				{
					continue;
				}

				const auto normalFinder = mMeshNormals.find(mesh);
				if (mMeshNormals.end() == normalFinder)
				{
					continue;
				}

				const FbxMesh* fbxMesh = meshFinder->second;
				for (int materialIndex = 0; materialIndex < mesh->materialCount; ++materialIndex)
				{
					Material* material = mesh->materials[materialIndex];
					if (material->diffuse)
					{
						const FbxGeometryElementUV* elementUV = nullptr;
						if (material->diffuse->useDefaultUVSet)
						{
							// this confuses, the name of the uvset referenced by FbxTexture.UVSet is "default", but there might not be a uvset with this name
							elementUV = fbxMesh->GetElementUV(0, FbxLayerElement::EType::eTextureDiffuse);
						}
						else
						{
							const auto fbxFinder = mFbxUVSets.find(material->diffuse->uvSet);
							if (mFbxUVSets.end() != fbxFinder)
							{
								elementUV = fbxFinder->second;
							}
						}
						if (!elementUV)
						{
							continue;
						}

						UVSet* uvSet = uvset_from_fbx(fbxMesh, elementUV);
						if (!uvSet)
						{
							continue;
						}

						std::vector<Triangle> triangles(mesh->triangleCount);
						memcpy(triangles.data(), mesh->triangles, sizeof(Triangle) * triangles.size());

						std::vector<Vertex> vertices(mesh->vertexCount);
						memcpy(vertices.data(), mesh->vertices, sizeof(Vertex) * mesh->vertexCount);

						//this->checkVertices(mesh, setFinder->second, triangles, vertices);
						//this->checkVertices(mesh, normalFinder->second, triangles, vertices);
						this->checkVertices(mesh, uvSet, normalFinder->second, triangles, vertices);

						// 一定要更新顶点，因为在checkVertices中会更新UV和法线
						free(mesh->vertices);
						mesh->vertexCount = static_cast<int>(vertices.size());
						mesh->vertices = g_copy_array(vertices.data(), vertices.size());

						free(mesh->triangles);
						mesh->triangles = g_copy_array(triangles.data(), triangles.size());

						destroy_uv_set(uvSet);
					}
				}

				//const std::vector<UVSet*>& uvSets = uvSetFinder->second;
				//if (uvSets.size())
				//{
				//	for (size_t i = 0; i < uvSets.size(); ++i)
				//	{
				//		std::vector<Triangle> triangles(mesh->triangleCount);
				//		memcpy(triangles.data(), mesh->triangles, sizeof(Triangle) * triangles.size());

				//		std::vector<Vertex> vertices(mesh->vertexCount);
				//		memcpy(vertices.data(), mesh->vertices, sizeof(Vertex) * mesh->vertexCount);


				//		if (vertices.size() != mesh->vertexCount)
				//		{
				//			free(mesh->vertices);
				//			mesh->vertexCount = static_cast<int>(vertices.size());
				//			mesh->vertices = g_copy_array(vertices.data(), vertices.size());

				//			free(mesh->triangles);
				//			mesh->triangles = g_copy_array(triangles.data(), triangles.size());
				//		}
				//	}
				//}
			}
		}

		static Vertex* copy_new_vertex(std::vector<Vertex>& vertices, const size_t& sourceIndex)
		{
			vertices.push_back(vertices[sourceIndex]);
			return vertices.data() + (vertices.size() - 1llu);
		}

		template <typename VertexReference> static Vertex* copy_new_vertex(Mesh* mesh, std::vector<VertexReference>& references, VertexReference*& vertexReference, int& referenceIndex, const int& triangleIndex, std::vector<Vertex>& vertices)
		{
			references.push_back(*vertexReference);
			vertexReference = references.data() + referenceIndex; // push_back in previous step might cause memory reallocation
			referenceIndex = static_cast<int>(references.size() - 1);
			VertexReference* newReference = references.data() + referenceIndex;
			newReference->vertexIndex = referenceIndex;
			vertexReference->nextIndex = referenceIndex;
			vertexReference = newReference;

			Vertex* vertex = copy_new_vertex(vertices, triangleIndex);
			mesh_copy_weight(mesh, triangleIndex, referenceIndex);
			return vertex;
		}

		void FbxModelImporter::checkVertices(Mesh* mesh, const UVSet* uvSet, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices)
		{
			struct VertexReference
			{
				int vertexIndex;
				int nextIndex; // 当前顶点的下一个复制品
			};
			VertexReference defaultReference = { };
			defaultReference.vertexIndex = -1;
			std::vector<VertexReference> references(vertices.size(), defaultReference);

			// TODO: handle multiple uv sets
			int index = 0;
			UV* uv = nullptr;
			Vertex* vertex = nullptr;
			Triangle* triangle = triangles.data();
			for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex, ++triangle)
			{
				for (int i = 0, vertexIndex, referenceIndex; i < 3; ++i, ++index)
				{
					referenceIndex = vertexIndex = triangle->indices[i];
					uv = uvSet->uvArray + index;
					assert(uv->vertexIndex == vertexIndex);

					vertex = vertices.data() + uv->vertexIndex;

					VertexReference* vertexReference = references.data() + referenceIndex;
					if (-1 == vertexReference->vertexIndex) // index为-1的是原始顶点，从未被赋值过，因此仅赋值
					{
						vertexReference->vertexIndex = vertexIndex;
						memcpy(vertex->uv[0].values, uv->uv.values, sizeof(Vector2));
						continue;
					}

					while (check_uv_different(vertex, uv->uv)) // 如果UV或法线与记录值不一样（可能是同一个顶点在不同的三角面上的状态），创建一个新的顶点，并且更新索引
					{
						if (vertexReference->nextIndex)
						{
							vertexReference = references.data() + vertexReference->nextIndex;
							vertex = vertices.data() + vertexReference->vertexIndex;
							continue;
						}

						vertex = copy_new_vertex(mesh, references, vertexReference, referenceIndex, triangle->indices[i], vertices);
						memcpy(vertex->uv[0].values, uv->uv.values, sizeof(Vector2));
					}
					triangle->indices[i] = static_cast<int>(vertex - vertices.data()); // 因为可能在上面的while中被更新过，所以最后一定要更新顶点索引
				}
			}
		}

		void FbxModelImporter::checkVertices(Mesh* mesh, const std::vector<Normal> normals, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices)
		{
			assert(normals.size() == triangles.size() * 3);

			struct VertexReference
			{
				int vertexIndex;
				int nextIndex; // 当前顶点的下一个复制品
			};
			VertexReference defaultReference = { };
			defaultReference.vertexIndex = -1;
			std::vector<VertexReference> references(vertices.size(), defaultReference);

			// TODO: handle multiple uv sets
			int index = 0;
			Vertex* vertex = nullptr;
			const Normal* normal = nullptr;
			Triangle* triangle = triangles.data();
			for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex, ++triangle)
			{
				for (int i = 0, vertexIndex, referenceIndex; i < 3; ++i, ++index)
				{
					referenceIndex = vertexIndex = triangle->indices[i];

					normal = normals.data() + index;
					vertex = vertices.data() + vertexIndex;

					VertexReference* vertexReference = references.data() + referenceIndex;
					if (-1 == vertexReference->vertexIndex) // index为-1的是原始顶点，从未被赋值过，因此仅赋值
					{
						vertexReference->vertexIndex = vertexIndex;
						memcpy(vertex->normal.values, normal->normal.values, sizeof(Vector3));
						continue;
					}

					while (check_normal_different(vertex, normal)) // 如果UV或法线与记录值不一样（可能是同一个顶点在不同的三角面上的状态），创建一个新的顶点，并且更新索引
					{
						if (vertexReference->nextIndex)
						{
							vertexReference = references.data() + vertexReference->nextIndex;
							vertex = vertices.data() + vertexReference->vertexIndex;
							continue;
						}

						vertex = copy_new_vertex(mesh, references, vertexReference, referenceIndex, triangle->indices[i], vertices);
						memcpy(vertex->normal.values, normal->normal.values, sizeof(Vector3));
					}
					triangle->indices[i] = static_cast<int>(vertex - vertices.data()); // 因为可能在上面的while中被更新过，所以最后一定要更新顶点索引
				}
			}
		}

		void FbxModelImporter::checkVertices(Mesh* mesh, const UVSet* uvSet, const std::vector<Normal> normals, std::vector<Triangle>& triangles, std::vector<Vertex>& vertices)
		{
			assert(uvSet->uvCount == triangles.size() * 3 && uvSet->uvCount == normals.size());

			struct VertexReference
			{
				int vertexIndex;
				int nextIndex; // 当前顶点的下一个复制品
			};
			VertexReference defaultReference = { };
			defaultReference.vertexIndex = -1;
			std::vector<VertexReference> references(vertices.size(), defaultReference);

			// TODO: handle multiple uv sets
			int index = 0;
			UV* uv = nullptr;
			Vertex* vertex = nullptr;
			const Normal* normal = nullptr;
			Triangle* triangle = triangles.data();
			for (size_t triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex, ++triangle)
			{
				for (int i = 0, vertexIndex, referenceIndex; i < 3; ++i, ++index)
				{
					referenceIndex = vertexIndex = triangle->indices[i];
					uv = uvSet->uvArray + index;
					assert(uv->vertexIndex == vertexIndex);

					normal = normals.data() + index;
					vertex = vertices.data() + uv->vertexIndex;

					VertexReference* vertexReference = references.data() + referenceIndex;
					if (-1 == vertexReference->vertexIndex) // index为-1的是原始顶点，从未被赋值过，因此仅赋值
					{
						vertexReference->vertexIndex = vertexIndex;
						memcpy(vertex->normal.values, normal->normal.values, sizeof(Vector3));
						memcpy(vertex->uv[0].values, uv->uv.values, sizeof(Vector2));
						continue;
					}

					while (check_uv_different(vertex, uv->uv) || check_normal_different(vertex, normal)) // 如果UV或法线与记录值不一样（可能是同一个顶点在不同的三角面上的状态），创建一个新的顶点，并且更新索引
					{
						if (vertexReference->nextIndex)
						{
							vertexReference = references.data() + vertexReference->nextIndex;
							vertex = vertices.data() + vertexReference->vertexIndex;
							continue;
						}

						vertex = copy_new_vertex(mesh, references, vertexReference, referenceIndex, triangle->indices[i], vertices);
						memcpy(vertex->normal.values, normal->normal.values, sizeof(Vector3));
						memcpy(vertex->uv[0].values, uv->uv.values, sizeof(Vector2));
					}
					triangle->indices[i] = static_cast<int>(vertex - vertices.data()); // 因为可能在上面的while中被更新过，所以最后一定要更新顶点索引
				}
			}
		}
	}
}