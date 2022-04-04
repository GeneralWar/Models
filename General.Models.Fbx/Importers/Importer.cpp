#include "pch.h"
#include "Importer.hpp"
#include <fbxsdk.h>
using namespace General;
using namespace fbxsdk;

namespace General
{
	namespace Models
	{
		namespace Fbx
		{
			struct Context
			{
				const char* filename;
				FbxManager* manager;
				FbxScene* scene;
				FbxImporter* importer;
				Model* model;
			};

			FbxManager* check_manager()
			{
				static FbxManager* sdk_manager = nullptr;
				if (nullptr == sdk_manager)
				{
					sdk_manager = FbxManager::Create();
					Tracer::Log("fbx version: %s", FbxManager::GetVersion());
					Tracer::SetPrefix("[General.Models.Fbx]");

					FbxIOSettings* ioSettings = FbxIOSettings::Create(sdk_manager, "IOSRoot");
					sdk_manager->SetIOSettings(ioSettings);
				}
				return sdk_manager;
			}

			bool check_status(Context* context, FbxStatus::EStatusCode* outErrorCode = nullptr)
			{
				FbxImporter* importer = context->importer;
				if (nullptr == importer)
				{
					Tracer::Error("no FbxImporter instance");
					return false;
				}
				if (!importer->IsFBX())
				{
					Tracer::Error("%s is not a fbx file", context->filename);
					return false;
				}

				const FbxStatus& status = importer->GetStatus();
				if (status.Error())
				{
					if (nullptr != outErrorCode)
					{
						*outErrorCode = status.GetCode();
					}

					FbxString error = status.GetErrorString();
					Tracer::Error(error.Buffer());
					return false;
				}

				return true;
			}

			const char* analyze_texture(Context* context, Material* material, FbxTexture* fbxTexture)
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

			void analyze_property_for_texture(Context* context, Material* material, const FbxProperty& property)
			{
				if (!property.IsValid())
				{
					return;
				}

				FbxLayeredTexture* fbxLayeredTexture = property.GetSrcObject<FbxLayeredTexture>();
				if (nullptr != fbxLayeredTexture)
				{
					int sourceCount = fbxLayeredTexture->GetSrcObjectCount();
					assert(!"Should handle multiple textures");
					return;
				}

				auto type = property.GetPropertyDataType();
				const char* filename = analyze_texture(context, material, property.GetSrcObject<FbxTexture>());
				if (nullptr == filename || 0 == strlen(filename))
				{
					return;
				}

				const char* propertyName = property.GetNameAsCStr();
				if (0 == strcmp("DiffuseTexture", propertyName))
				{
					material_set_diffuse(material, filename);
					return;
				}

				Tracer::Warn("Should handle other texture: %s", propertyName);
			}

			void enumerate_material_property_for_texture(Context* context, Material* material, /*const */FbxSurfaceMaterial* fbxMaterial)
			{
				FbxProperty property = fbxMaterial->GetFirstProperty();
				while (property.IsValid())
				{
					analyze_property_for_texture(context, material, property);
					property = fbxMaterial->GetNextProperty(property);
				}
			}

			Material* analyze_material(Context* context, /*const */FbxSurfaceMaterial* fbxMaterial)
			{
				Model* model = context->model;
				if (nullptr == fbxMaterial || nullptr == model)
				{
					return nullptr;
				}

				const char* materialName = fbxMaterial->GetName();
				Material* material = find_material(model->materials, model->materialCount, materialName);
				if (nullptr == material)
				{
					model_add_material_count(context->model);
					material = model->materials[model->materialCount - 1] = create_material(materialName);

					FbxSurfaceLambert* lambert = FbxCast<FbxSurfaceLambert>(fbxMaterial);
					if (nullptr == lambert)
					{
						FbxProperty property = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
						if (property.IsValid())
						{
							analyze_property_for_texture(context, material, property);
						}
						else
						{
							enumerate_material_property_for_texture(context, material, fbxMaterial);
						}
					}
					else 
					{
						FbxFileTexture* ambient = lambert->Ambient.GetSrcObject<FbxFileTexture>();
						if (nullptr != ambient)
						{
							material_set_ambient(material, ambient->GetFileName());
						}
						FbxFileTexture* diffuse = lambert->Diffuse.GetSrcObject<FbxFileTexture>();
						if (nullptr != diffuse)
						{
							material_set_diffuse(material, diffuse->GetFileName());
						}
						FbxFileTexture* emissive = lambert->Emissive.GetSrcObject<FbxFileTexture>();
						if (nullptr != emissive)
						{
							material_set_emissive(material, emissive->GetFileName());
						}

						FbxSurfacePhong* phong = FbxCast<FbxSurfacePhong>(fbxMaterial);
						if (nullptr != phong)
						{
							FbxFileTexture* specular = phong->Specular.GetSrcObject<FbxFileTexture>();
							if (nullptr != specular)
							{
								material_set_specular(material, specular->GetFileName());
							}
						}
					}
				}
				return material;
			}

			Mesh* analyze_mesh(Context* context, const FbxMesh* fbxMesh)
			{
				if (nullptr == fbxMesh)
				{
					return nullptr;
				}

				Model* model = context->model;
				model_add_mesh_count(model);

				Mesh* mesh = model->meshes[model->meshCount - 1];
				const char* meshName = fbxMesh->GetName();
				if (nullptr == meshName || 0 == strlen(meshName))
				{
					meshName = fbxMesh->GetNode()->GetName();
				}
				mesh_set_name(mesh, meshName);

#pragma region Vertex
				int vertexCount = fbxMesh->GetControlPointsCount();
				mesh_set_vertex_count(mesh, vertexCount);

				FbxVector4* controlPoints = fbxMesh->GetControlPoints();
				for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
				{
					Vertex* vertex = mesh->vertices + vertexIndex;
					FbxVector4* point = controlPoints + vertexIndex;
					vertex->position[0] = static_cast<float>(point->mData[0]);
					vertex->position[1] = static_cast<float>(point->mData[1]);
					vertex->position[2] = static_cast<float>(point->mData[2]);
				}
#pragma endregion

#pragma region Index
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

				mesh_set_index_count(mesh, indexCount);
				for (int polygonIndex = 0, i = 0; polygonIndex < polygonCount; ++polygonIndex, ++i)
				{
					VertexIndex* triangle = mesh->indices + i;
					int startIndex = fbxMesh->GetPolygonVertexIndex(polygonIndex);
					int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
					if (startIndex > -1)
					{
						memcpy(&triangle->index0, fbxMesh->GetPolygonVertices() + startIndex, sizeof(int) * polygonSize);
					}
					else
					{
						DebugBreak();
					}
				}
#pragma endregion

#pragma region UV
				FbxStringList uvSetNameList;
				fbxMesh->GetUVSetNames(uvSetNameList);
				for (int i = 0; i < uvSetNameList.GetCount(); ++i)
				{
					const char* uvSetName = uvSetNameList.GetStringAt(i);
					const FbxGeometryElementUV* elementUV= fbxMesh->GetElementUV(uvSetName);
					if (!elementUV)
					{
						continue;
					}

					FbxLayerElement::EMappingMode mappingMode = elementUV->GetMappingMode();
					// only support mapping mode eByPolygonVertex and eByControlPoint
					if (FbxGeometryElement::eByPolygonVertex != mappingMode && FbxGeometryElement::eByControlPoint != mappingMode)
					{
						continue;
					}

					const FbxLayerElementArrayTemplate<int>& indexArray = elementUV->GetIndexArray(); 
					const FbxLayerElementArrayTemplate<FbxVector2>& directArray = elementUV->GetDirectArray();
					//index array, where holds the index referenced to the uv data
					const bool useIndex = elementUV->GetReferenceMode() != FbxGeometryElement::eDirect;
					assert(!useIndex || indexCount == indexArray.GetCount());
					//const int indexCount = (useIndex) ? elementUV->GetIndexArray().GetCount() : 0;
					//iterating through the data by polygon
					const int polygonCount = fbxMesh->GetPolygonCount();
					if (FbxGeometryElement::eByControlPoint == mappingMode)
					{
						Tracer::Warn("TODO: ensure this condition");
						for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
						{
							// build the max index array that we need to pass into MakePoly
							const int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
							for (int vertexIndex = 0; vertexIndex < polygonSize; ++vertexIndex)
							{
								//get the index of the current vertex in control points array
								int polygonVertexIndex = fbxMesh->GetPolygonVertex(polygonIndex, vertexIndex);
								//the UV index depends on the reference mode
								int uvIndex = useIndex ? indexArray.GetAt(polygonVertexIndex) : polygonVertexIndex;
								if (uvIndex < mesh->vertexCount)
								{
									Vertex* vertex = mesh->vertices + uvIndex;
									FbxVector2 uvValue = directArray.GetAt(uvIndex);
									vertex->uv0[0] = static_cast<float>(uvValue.mData[0]);
									vertex->uv0[1] = static_cast<float>(uvValue.mData[1]);
								}
							}
						}
					}
					else if (FbxGeometryElement::eByPolygonVertex == mappingMode)
					{
						int polygonIndexCounter = 0;
						const int* vertexIndices = fbxMesh->GetPolygonVertices();
						for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
						{
							// build the max index array that we need to pass into MakePoly
							const int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
							const int vertexStartIndex = fbxMesh->GetPolygonVertexIndex(polygonIndex);
							for (int vertexIndex = 0; vertexIndex < polygonSize; ++vertexIndex)
							{
								if (polygonIndexCounter < indexCount)
								{
									//the UV index depends on the reference mode
									int uvIndex = useIndex ? indexArray.GetAt(polygonIndexCounter) : polygonIndexCounter;
									int index = vertexIndices[vertexStartIndex + vertexIndex];
									if (index < mesh->vertexCount)
									{
										Vertex* vertex = mesh->vertices + index;
										FbxVector2 uvValue = directArray.GetAt(uvIndex);
										vertex->uv0[0] = static_cast<float>(uvValue.mData[0]);
										vertex->uv0[1] = static_cast<float>(uvValue.mData[1]);
									}
									polygonIndexCounter++;
								}
							}
						}
					}
				}
#pragma endregion

				return mesh;
			}

			void analyze_node(Context* context, FbxNode* node)
			{
				FbxMesh* fbxMesh = node->GetMesh();
				if (nullptr != fbxMesh)
				{
					Mesh* mesh = analyze_mesh(context, fbxMesh);
					assert(mesh);

					int materialCount = node->GetMaterialCount();
					for (int i = 0; i < materialCount; ++i)
					{
						FbxSurfaceMaterial* fbxMaterial = node->GetMaterial(i);
						Material* material = analyze_material(context, fbxMaterial);
						if (nullptr != material)
						{
							mesh_add_material(mesh, material);
						}
					}
				}

				int childCount = node->GetChildCount();
				for (int i = 0; i < childCount; ++i)
				{
					analyze_node(context, node->GetChild(i));
				}
			}

			void import(Context* context)
			{
				FbxImporter* importer = context->importer;
				Model* model = context->model;

				FbxScene* scene = context->scene = FbxScene::Create(context->manager, "ImportScene");
				if (!importer->Import(scene))
				{
					FbxStatus::EStatusCode errorCode;
					if (!check_status(context, &errorCode))
					{
						if (errorCode == FbxStatus::ePasswordError)
						{
							// TODO: input password
						}
						return;
					}
				}
				
				FbxGeometryConverter clsConverter(context->manager);
				clsConverter.Triangulate(scene, true);

				FbxNode* root = scene->GetRootNode();
				if (nullptr == root)
				{
					Tracer::Error("no root node");
					return;
				}

				analyze_node(context, root);
			}

			ModelImporter::ModelImporter(const char* filename) : Importer(filename), mContext((Context*)malloc(sizeof(Context)))
			{
				memset(mContext, 0, sizeof(Context));
				mContext->filename = this->GetFilename();
				if (nullptr == filename || 0 == strlen(filename))
				{
					return;
				}

				FbxManager* manager = mContext->manager = check_manager();
				fbxsdk::FbxImporter* importer = mContext->importer = fbxsdk::FbxImporter::Create(manager, "Importer");
				bool status = importer->Initialize(filename, -1, manager->GetIOSettings());
				if (!status)
				{
					FbxString error = importer->GetStatus().GetErrorString();
					Tracer::Error("failed to initialize FbxImporter: %s", error.Buffer());
				}
			}

			ModelImporter::~ModelImporter()
			{
				if (mContext->scene)
				{
					mContext->scene->Destroy();
				}
				if (mContext->importer)
				{
					mContext->importer->Destroy();
				}
			}

			bool ModelImporter::IsValid()
			{
				return check_status(mContext);
			}

			Model* ModelImporter::Import()
			{
				if (!this->IsValid())
				{
					return nullptr;
				}

				Model* model = create_model();
				mContext->model = model;
				import(mContext);

				return model;
			}
		}
	}
}