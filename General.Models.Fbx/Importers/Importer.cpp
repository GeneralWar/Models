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

			void analyze_mesh(Context* context, const FbxMesh* mesh)
			{
				Model* model = context->model;
				model_add_mesh_count(model);

				Mesh* generalMesh = model->meshes[model->meshCount - 1];
				mesh_set_name(generalMesh, mesh->GetName());

				int vertexCount = generalMesh->vertexCount = mesh->GetControlPointsCount();
				mesh_set_vertex_count(generalMesh, vertexCount);

				FbxVector4* controlPoints = mesh->GetControlPoints();
				for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
				{
					Vertex* vertex = generalMesh->vertices + vertexIndex;
					FbxVector4* point = controlPoints + vertexIndex;
					vertex->position[0] = static_cast<float>(point->mData[0]);
					vertex->position[1] = static_cast<float>(point->mData[1]);
					vertex->position[2] = static_cast<float>(point->mData[2]);
				}

				int indexCount = 0;
				int polygonCount = mesh->GetPolygonCount();
				for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					int polygonSize = mesh->GetPolygonSize(polygonIndex);
					switch (polygonSize)
					{
					case 3:
						indexCount += 3;
						break;
					default:
#ifdef DEBUG
						assert(!"unexpected vertex count");
#endif
						break;
					}
				}
				mesh_set_index_count(generalMesh, indexCount);
				for (int polygonIndex = 0, i = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					VertexIndex* triangle = generalMesh->indices + i;
					int startIndex = mesh->GetPolygonVertexIndex(polygonIndex);
					int polygonSize = mesh->GetPolygonSize(polygonIndex);
					switch (polygonSize)
					{
					case 3:
						if (startIndex > -1)
						{
							memcpy(&triangle->index0, mesh->GetPolygonVertices() + startIndex, sizeof(int) * polygonSize);
						}
						else
						{
							DebugBreak();
						}
						++i;
						break;
					default:
#ifdef DEBUG
						assert(!"unexpected vertex count");
#endif
						break;
					}
				}
			}

			void analyze_node(Context* context, FbxNode* node)
			{
				FbxMesh* mesh = node->GetMesh();
				if (nullptr != mesh)
				{
					analyze_mesh(context, mesh);
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