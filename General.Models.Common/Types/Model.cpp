#include "pch.h"
#include "Model.hpp"

namespace General
{
	namespace Models
	{
		void set_string(char** buffer, const char* name)
		{
			if (*buffer)
			{
				free(*buffer);
			}
			*buffer = nullptr;

			if (nullptr == name)
			{
				return;
			}

			*buffer = (char*)malloc(strlen(name) + 1);
			strcpy(*buffer, name);
		}

		void add_material_count(Material*** materials, int* materialCount)
		{
			int materialCount0 = *materialCount;
			Material** materials0 = *materials;
			size_t newSize = sizeof(Material*) * (++(*materialCount));
			*materials = (Material**)malloc(newSize);
			memset(*materials, 0, newSize);
			memcpy(*materials, materials0, sizeof(Material*) * materialCount0);
		}

		Mesh* create_mesh()
		{
			Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
			memset(mesh, 0, sizeof(Mesh));
			return mesh;
		}

		void mesh_set_name(Mesh* mesh, const char* name)
		{
			set_string(const_cast<char**>(&mesh->name), name);
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
		
		void mesh_add_material(Mesh* mesh, Material* material)
		{
			add_material_count(&mesh->materials, &mesh->materialCount);
			mesh->materials[mesh->materialCount - 1] = material;
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

		Material* create_material(const char* name)
		{
			Material* material = (Material*)malloc(sizeof(Material));
			memset(material, 0, sizeof(Material));
			if (nullptr != name)
			{
				material->name = (char*)malloc(strlen(name) + 1);
				strcpy(const_cast<char*>(material->name), name);
			}
			return material;
		}
		
		void material_set_ambient(Material* material, const char* filename)
		{
			set_string(const_cast<char**>(&material->ambient), filename);
		}
		
		void material_set_diffuse(Material* material, const char* filename)
		{
			set_string(const_cast<char**>(&material->diffuse), filename);
		}
		
		void material_set_emissive(Material* material, const char* filename)
		{
			set_string(const_cast<char**>(&material->emissive), filename);
		}
		
		void material_set_specular(Material* material, const char* filename)
		{
			set_string(const_cast<char**>(&material->specular), filename);
		}

		Material* find_material(Material** materials, const int materialCount, const char* materialName)
		{
			for (int i = 0; i < materialCount; ++i)
			{
				Material* material = materials[i];
				if (nullptr != material && 0 == strcmp(material->name, materialName))
				{
					return material;
				}
			}
			return nullptr;
		}

		void destroy_material(Material* material)
		{
			if (nullptr == material)
			{
				return;
			}

			free(material);
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

		void model_add_material_count(Model* model)
		{
			add_material_count(&model->materials, &model->materialCount);
		}

		void destroy_model(Model* model)
		{
			for (int i = 0; i < model->meshCount; ++i)
			{
				destroy_mesh(model->meshes[i]);
			}
			for (int i = 0; i < model->materialCount; ++i)
			{
				destroy_material(model->materials[i]);
			}
			free(model);
		}
	}
}