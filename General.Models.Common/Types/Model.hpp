#ifndef GENERAL_MODELS_COMMON_MODEL_HPP
#define GENERAL_MODELS_COMMON_MODEL_HPP

namespace General
{
	namespace Models
	{
		struct Vertex
		{
			float position[3];
		};

		struct UV
		{
			int vertexIndex;
			float value[2];
		};

		struct Normal
		{
			int vertexIndex;
			float value[3];
		};

		struct UVSet
		{
			char* name;
			int uvCount;
			UV* uvArray;
		};

		GENERAL_API UVSet* create_uv_set(const char* name);
		GENERAL_API void uv_set_set_uv_count(UVSet* set, const int count);
		GENERAL_API UVSet* find_uv_set(UVSet** sets, const int setCount, const char* setName);
		GENERAL_API void destroy_uv_set(UVSet* set);

		struct VertexIndex
		{
			int index0;
			int index1;
			int index2;
		};
		
		struct MaterialTexture
		{
			char* texture;
			char* uvSet;
			bool useDefaultUVSet;
		};

		GENERAL_API MaterialTexture* create_material_texture();
		GENERAL_API void set_material_texture(MaterialTexture* texture, const char* filename, const char* uvSet);
		GENERAL_API void destroy_material_texture(MaterialTexture* material);


		struct Material
		{
			char* name;
			MaterialTexture* ambient;
			MaterialTexture* diffuse;
			MaterialTexture* emissive;
			MaterialTexture* specular;
		};

		GENERAL_API Material* create_material(const char* name);
		GENERAL_API void material_set_ambient(Material* material, const char* filename, const char* uvSet);
		GENERAL_API void material_set_diffuse(Material* material, const char* filename, const char* uvSet);
		GENERAL_API void material_set_emissive(Material* material, const char* filename, const char* uvSet);
		GENERAL_API void material_set_specular(Material* material, const char* filename, const char* uvSet);
		GENERAL_API Material* find_material(Material** materials, const int materialCount, const char* materialName);
		GENERAL_API void destroy_material(Material* material);

		struct Mesh
		{
			char* name;

			int vertexCount;
			Vertex* vertices; 

			int indexCount;
			VertexIndex* indices;

			int materialCount;
			Material** materials;

			int uvSetCount;
			UVSet** uvSets;

			int normalCount;
			Normal* normals;
		};

		GENERAL_API Mesh* create_mesh(const char* name);
		GENERAL_API void mesh_set_vertex_count(Mesh* mesh, const int vertexCount);
		GENERAL_API void mesh_set_index_count(Mesh* mesh, const int indexCount);
		GENERAL_API void mesh_set_normal_count(Mesh* mesh, const int indexCount);
		GENERAL_API void mesh_add_material(Mesh* mesh, Material* material);
		GENERAL_API void mesh_add_uv_set(Mesh* mesh, UVSet* uvSet);
		GENERAL_API void destroy_mesh(Mesh* mesh);

		struct Node
		{
			char* name;
			bool visible;

			Mesh* mesh;

			int childCount;
			Node** children;
		};

		GENERAL_API Node* create_node(const char* name);
		GENERAL_API void node_set_child_count(Node* node, const int count);
		GENERAL_API void destroy_node(Node* node);

		struct Model
		{
			Node* root;
			float unit; // relative to centimeter

			int materialCount;
			Material** materials;
		};

		GENERAL_API Model* create_model();
		GENERAL_API void model_add_material_count(Model* model);
		GENERAL_API void destroy_model(Model* model);
	}
}

#endif