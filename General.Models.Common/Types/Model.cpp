#include "pch.h"
#include "Model.hpp"

namespace General
{
	namespace Models
	{
		Vector3 vector3_scale(const Vector3 v, const float scaling)
		{
			Vector3 r = { };
			r.x = v.x * scaling;
			r.y = v.y * scaling;
			r.z = v.z * scaling;
			return r;
		}

		UVSet* create_uv_set(const char* name)
		{
			UVSet* set = (UVSet*)malloc(sizeof(UVSet));
			memset(set, 0, sizeof(UVSet));
			g_set_string(&set->name, name);
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

		WeightCollection* create_weight_collection(Node* bone, Matrix boneTransform, const int weightCount)
		{
			WeightCollection* instance = g_alloc_struct<WeightCollection>();
			instance->bone = bone;
			instance->boneOffset = boneTransform;
			g_resize_array(&instance->weights, &instance->weightCount, weightCount);
			return instance;
		}
		
		void weight_collection_copy_weight(WeightCollection* instance, const int templateIndex, const int newIndex)
		{
			int index = -1;
			int count = instance->weightCount;
			const WeightData* weight = instance->weights;
			for (int i = 0; i < count; ++i, ++weight)
			{
				if (weight->index == templateIndex)
				{
					index = i;
					break;
				}
			}
			if (-1 == index)
			{
				return;
			}

			g_resize_array(&instance->weights, &instance->weightCount, count + 1);
			memcpy(instance->weights + count, instance->weights + index, sizeof(WeightData));
			(instance->weights + count)->index = newIndex;
		}

		void destroy_weight_collection(WeightCollection* instance)
		{
			if (instance->weights) free(instance->weights);
			free(instance);
		}

		Mesh* create_mesh(const char* name)
		{
			Mesh* instance = (Mesh*)malloc(sizeof(Mesh));
			memset(instance, 0, sizeof(Mesh));
			g_set_string(&instance->name, name);
			return instance;
		}

		void mesh_set_vertex_count(Mesh* instance, const int count)
		{
			g_resize_array(&instance->vertices, &instance->vertexCount, count);
		}

		void mesh_set_triangle_count(Mesh* instance, const int count)
		{
			g_resize_array(&instance->triangles, &instance->triangleCount, count);
		}
		
		void mesh_set_triangles(Mesh* instance, const int count, const Triangle* triangles)
		{
			mesh_set_triangle_count(instance, count);
			memcpy(instance->triangles, triangles, sizeof(Triangle) * count);
		}
		
		void mesh_add_material(Mesh* instance, Material* material)
		{
			int index = instance->materialCount;
			g_resize_array(&instance->materials, &instance->materialCount, index + 1);
			instance->materials[index] = material;
		}

		void mesh_add_weight_collection(Mesh* instance, WeightCollection* collection)
		{
			int index = instance->weightCollectionCount;
			g_resize_array(&instance->weightCollections, &instance->weightCollectionCount, index + 1);
			instance->weightCollections[index] = collection;
		}

		void mesh_copy_weight(Mesh* instance, const int templateIndex, const int newIndex)
		{
			WeightCollection** collections = instance->weightCollections;
			for (int i = 0; i < instance->weightCollectionCount; ++i, ++collections)
			{
				weight_collection_copy_weight(*collections, templateIndex, newIndex);
			}
		}

		void destroy_mesh(Mesh* instance)
		{
			CHECK(instance, );

			if (instance->vertices) free(instance->vertices);
			if (instance->triangles) free(instance->triangles);

			for (int i = 0; i < instance->weightCollectionCount; ++i)
			{
				destroy_weight_collection(instance->weightCollections[i]);
			}
			free(instance->weightCollections);

			for (int i = 0; i < instance->materialCount; ++i)
			{
				destroy_material(instance->materials[i]);
			}
			free(instance->materials);

			if (instance->name) free(const_cast<char*>(instance->name));

			free(instance);
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
			g_set_string(&material->texture, filename);
			g_set_string(&material->uvSet, uvSet);
		}

		void destroy_material_texture(MaterialTexture* material)
		{
			if (material->texture) free(material->texture);
			if (material->uvSet) free(material->uvSet);
			free(material);
		}

		Material* create_material(const char* name)
		{
			Material* material = (Material*)malloc(sizeof(Material));
			memset(material, 0, sizeof(Material));
			g_set_string(&material->name, name);
			return material;
		}
		
		void material_set_ambient(Material* instance, const char* filename, const char* uvSet)
		{
			if (nullptr == instance->ambient)
			{
				instance->ambient = create_material_texture();
			}
			set_material_texture(instance->ambient, filename, uvSet);
		}
		
		void material_set_diffuse(Material* instance, const char* filename, const char* uvSet)
		{
			if (nullptr == instance->diffuse)
			{
				instance->diffuse = create_material_texture();
			}
			set_material_texture(instance->diffuse, filename, uvSet);		
		}
		
		void material_set_emissive(Material* instance, const char* filename, const char* uvSet)
		{
			if (nullptr == instance->emissive)
			{
				instance->emissive = create_material_texture();
			}
			set_material_texture(instance->emissive, filename, uvSet);			
		}
		
		void material_set_specular(Material* instance, const char* filename, const char* uvSet)
		{
			if (nullptr == instance->specular)
			{
				instance->specular = create_material_texture();
			}
			set_material_texture(instance->specular, filename, uvSet);
		}

		Material* find_material(Material** instance, const int materialCount, const char* materialName)
		{
			for (int i = 0; i < materialCount; ++i)
			{
				Material* material = instance[i];
				if (nullptr != material && 0 == strcmp(material->name, materialName))
				{
					return material;
				}
			}
			return nullptr;
		}

		void destroy_material(Material* instance)
		{
			if (nullptr == instance)
			{
				return;
			}

			if (instance->ambient) destroy_material_texture(instance->ambient);
			if (instance->diffuse) destroy_material_texture(instance->diffuse);
			if (instance->emissive) destroy_material_texture(instance->emissive);
			if (instance->specular) destroy_material_texture(instance->specular);

			if (instance->name) free(const_cast<char*>(instance->name));

			free(instance);
		}

		Node* create_node(const char* name)
		{
			Node* node = g_alloc_struct<Node>();
			g_set_string(&node->name, name);
			node->visible = true;
			return node;
		}

		void node_add_child(Node* instance, Node* child)
		{
			int index = instance->childCount;
			g_resize_array(&instance->children, &instance->childCount, index + 1);
			instance->children[index] = child;
			child->parent = instance;
		}

		void destroy_node(Node* instance)
		{
			if (!instance)
			{
				return;
			}

			if (instance->mesh)
			{
				destroy_mesh(instance->mesh);
			}

			for (int i = 0; i < instance->childCount; ++i)
			{
				destroy_node(instance->children[i]);
			}
			free(instance->children);

			if (instance->name) free(const_cast<char*>(instance->name));
			free(instance);
		}

		Model* create_model()
		{
			Model* instance = (Model*)malloc(sizeof(Model));
			memset(instance, 0, sizeof(Model));
			return instance;
		}

		void model_add_mesh(Model* instance, Mesh* mesh) 
		{
			int index = instance->meshCount;
			g_resize_array(&instance->meshes, &instance->meshCount, index + 1);
			instance->meshes[index] = mesh;
		}

		void model_add_material(Model* instance, Material* material)
		{
			int index = instance->materialCount;
			g_resize_array(&instance->materials, &instance->materialCount, index + 1);
			instance->materials[index] = material;
		}

		void model_add_animation(Model* instance, Animation* animation)
		{
			int index = instance->animationCount;
			g_resize_array(&instance->animations, &instance->animationCount, index + 1);
			instance->animations[index] = animation;
		}

		void destroy_model(Model* instance)
		{
			CHECK(instance, );

			if (instance->root) destroy_node(instance->root);

			if (instance->meshes) free(instance->meshes);
			if (instance->materials) free(instance->materials); // release items at destroy_mesh

			if (instance->animations)
			{
				for (int i = 0; i < instance->animationCount; ++i)
				{
					destroy_animation(instance->animations[i]);
				}
				free(instance->animations);
			}

			free(instance);
		}
	}
}