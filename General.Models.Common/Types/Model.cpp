#include "pch.h"
#include "Model.hpp"

namespace General
{
	namespace Models
	{
		Mesh* create_mesh()
		{
			Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
			memset(mesh, 0, sizeof(Mesh));
			return mesh;
		}

		void mesh_set_name(Mesh* mesh, const char* name)
		{
			if (mesh->name)
			{
				free(const_cast<char*>(mesh->name));
			}
			mesh->name = nullptr;

			if (nullptr == name)
			{
				return;
			}

			mesh->name = (char*)malloc(strlen(name) + 1);
			strcpy(const_cast<char*>(mesh->name), name);
		}

		void mesh_set_vertex_count(Mesh* mesh, const int vertexCount)
		{
			const int vertexCount0 = mesh->vertexCount;
			Vertex* vertices0 = mesh->vertices;
			mesh->vertices = (Vertex*)malloc(sizeof(Vertex) * vertexCount);
			mesh->vertexCount = vertexCount;
			if (vertices0 && mesh->vertices)
			{
				memcpy(mesh->vertices, vertices0, sizeof(Vertex) * g_min(vertexCount0, vertexCount));
				free(vertices0);
			}
			memset(mesh->vertices + vertexCount0, 0, sizeof(Vertex) * (vertexCount - vertexCount0));
		}

		void mesh_set_index_count(Mesh* mesh, const int indexCount)
		{
			const int indexCount0 = mesh->indexCount;
			VertexIndex* indices0 = mesh->indices;
			mesh->indices = (VertexIndex*)malloc(sizeof(VertexIndex) * indexCount);
			mesh->indexCount = indexCount;
			if (indices0 && mesh->indices)
			{
				memcpy(mesh->indices, indices0, sizeof(VertexIndex) * g_min(indexCount0, indexCount));
				free(indices0);
			}
			memset(mesh->indices + indexCount0, 0, sizeof(VertexIndex) * (indexCount - indexCount0));
		}

		void destroy_mesh(Mesh* mesh)
		{
			if (nullptr == mesh)
			{
				return;
			}

			if (mesh->vertices)
			{
				free(mesh->vertices);
			}
			if (mesh->indices)
			{
				free(mesh->indices);
			}

			free(mesh);
		}

		Model* create_model()
		{
			Model* model = (Model*)malloc(sizeof(Model));
			memset(model, 0, sizeof(Model));
			return model;
		}

		void model_add_mesh_count(Model* model)
		{
			int meshCount = model->meshCount;
			Mesh** meshes = model->meshes;
			size_t newSize = sizeof(Mesh*) * ++model->meshCount;
			model->meshes = (Mesh**)malloc(newSize);
			memset(model->meshes, 0, newSize);
			memcpy(model->meshes, meshes, sizeof(Mesh*) * meshCount);
			model->meshes[meshCount] = create_mesh();
		}

		void destroy_model(Model* model)
		{
			for (int i = 0; i < model->meshCount; ++i)
			{
				destroy_mesh(model->meshes[i]);
			}
			free(model);
		}
	}
}