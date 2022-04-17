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

		UVSet* create_uv_set(const char* name)
		{
			UVSet* set = (UVSet*)malloc(sizeof(UVSet));
			memset(set, 0, sizeof(UVSet));
			set_string(&set->name, name);
			return set;
		}

		void uv_set_set_uv_count(UVSet* set, const int count)
		{
			g_resize_array(&set->uvArray, &set->uvCount, count);
		}

		UVSet* find_uv_set(UVSet** sets, const int setCount, const char* setName)
		{
			if (nullptr != setName)
			{
				for (int i = 0; i < setCount; ++i)
				{
					if (sets[i] && 0 == strcmp(sets[i]->name, setName))
					{
						return sets[i];
					}
				}
			}
			return nullptr;
		}

		void destroy_uv_set(UVSet* set)
		{
			if (set->name) free(set->name);
			if (set->uvArray) free(set->uvArray);
			free(set);
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

		Mesh* create_mesh(const char* name)
		{
			Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
			memset(mesh, 0, sizeof(Mesh));
			set_string(&mesh->name, name);
			return mesh;
		}

		void mesh_set_vertex_count(Mesh* mesh, const int count)
		{
			g_resize_array(&mesh->vertices, &mesh->vertexCount, count);
		}

		void mesh_set_index_count(Mesh* mesh, const int count)
		{
			g_resize_array(&mesh->indices, &mesh->indexCount, count);
		}
		
		void mesh_add_material(Mesh* mesh, Material* material)
		{
			add_material_count(&mesh->materials, &mesh->materialCount);
			mesh->materials[mesh->materialCount - 1] = material;
		}
		
		void mesh_add_uv_set(Mesh* mesh, UVSet* uvSet)
		{
			g_resize_array(reinterpret_cast<void**>(&mesh->uvSets), &mesh->uvSetCount, sizeof(UVSet*), mesh->uvSetCount + 1);
			mesh->uvSets[mesh->uvSetCount - 1] = uvSet;
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
			// should not destroy materials because model create them
			for (int i = 0; i < mesh->uvSetCount; ++i)
			{
				destroy_uv_set(mesh->uvSets[i]);
			}

			free(const_cast<char*>(mesh->name));
			free(mesh);
		}

		MaterialTexture* create_material_texture()
		{
			MaterialTexture* material = (MaterialTexture*)malloc(sizeof(MaterialTexture));
			memset(material, 0, sizeof(MaterialTexture));
			return material;
		}

		void set_material_texture(MaterialTexture* material, const char* filename, const char* uvSet)
		{
			CHECK(material, );
			set_string(&material->texture, filename);
			set_string(&material->uvSet, uvSet);
		}

		void destroy_material_texture(MaterialTexture* material)
		{
			if (material->texture) free(material->texture);
			if (material->uvSet) free(material->uvSet);
		}

		Material* create_material(const char* name)
		{
			Material* material = (Material*)malloc(sizeof(Material));
			memset(material, 0, sizeof(Material));
			set_string(&material->name, name);
			return material;
		}
		
		void material_set_ambient(Material* material, const char* filename, const char* uvSet)
		{
			if (nullptr == material->ambient)
			{
				material->ambient = create_material_texture();
			}
			set_material_texture(material->ambient, filename, uvSet);
		}
		
		void material_set_diffuse(Material* material, const char* filename, const char* uvSet)
		{
			if (nullptr == material->diffuse)
			{
				material->diffuse = create_material_texture();
			}
			set_material_texture(material->diffuse, filename, uvSet);		
		}
		
		void material_set_emissive(Material* material, const char* filename, const char* uvSet)
		{
			if (nullptr == material->emissive)
			{
				material->emissive = create_material_texture();
			}
			set_material_texture(material->emissive, filename, uvSet);			
		}
		
		void material_set_specular(Material* material, const char* filename, const char* uvSet)
		{
			if (nullptr == material->specular)
			{
				material->specular = create_material_texture();
			}
			set_material_texture(material->specular, filename, uvSet);		
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

			if (material->ambient) destroy_material_texture(material->ambient);
			if (material->diffuse) destroy_material_texture(material->diffuse);
			if (material->emissive) destroy_material_texture(material->emissive);
			if (material->specular) destroy_material_texture(material->specular);

			free(const_cast<char*>(material->name));
			free(material);
		}

		Node* create_node(const char* name)
		{
			Node* node = (Node*)malloc(sizeof(Node));
			memset(node, 0, sizeof(Node));
			set_string(&node->name, name);
			return node;
		}

		void node_set_child_count(Node* node, const int count)
		{
			g_resize_array(reinterpret_cast<void**>(&node->children), &node->childCount, sizeof(Node*), count);
		}

		void destroy_node(Node* node)
		{
			if (!node)
			{
				return;
			}

			if (node->mesh)
			{
				destroy_mesh(node->mesh);
			}
			for (int i = 0; i < node->childCount; ++i)
			{
				destroy_node(node->children[i]);
			}
			free(const_cast<char*>(node->name));
			free(node);
		}

		Model* create_model()
		{
			Model* model = (Model*)malloc(sizeof(Model));
			memset(model, 0, sizeof(Model));
			return model;
		}

		void model_add_material_count(Model* model)
		{
			add_material_count(&model->materials, &model->materialCount);
		}

		void destroy_model(Model* model)
		{
			if (model->root)
			{
				destroy_node(model->root);
			}

			for (int i = 0; i < model->materialCount; ++i)
			{
				destroy_material(model->materials[i]);
			}
			free(model);
		}
	}
}