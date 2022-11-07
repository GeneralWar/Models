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

		EXPORT UVSet* create_uv_set(const char* name);
		EXPORT void uv_set_set_uv_count(UVSet* set, const int count);
		EXPORT UVSet* find_uv_set(UVSet** sets, const int setCount, const char* setName);
		EXPORT void destroy_uv_set(UVSet* set);

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

		EXPORT MaterialTexture* create_material_texture();
		EXPORT void set_material_texture(MaterialTexture* texture, const char* filename, const char* uvSet);
		EXPORT void destroy_material_texture(MaterialTexture* material);

		struct Material
		{
			char* name;
			MaterialTexture* ambient;
			MaterialTexture* diffuse;
			MaterialTexture* emissive;
			MaterialTexture* specular;
		};

		EXPORT Material* create_material(const char* name);
		EXPORT void material_set_ambient(Material* material, const char* filename, const char* uvSet);
		EXPORT void material_set_diffuse(Material* material, const char* filename, const char* uvSet);
		EXPORT void material_set_emissive(Material* material, const char* filename, const char* uvSet);
		EXPORT void material_set_specular(Material* material, const char* filename, const char* uvSet);
		EXPORT Material* find_material(Material** materials, const int materialCount, const char* materialName);
		EXPORT void destroy_material(Material* material);

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

		EXPORT Mesh* create_mesh(const char* name);
		EXPORT void mesh_set_vertex_count(Mesh* mesh, const int vertexCount);
		EXPORT void mesh_set_index_count(Mesh* mesh, const int indexCount);
		EXPORT void mesh_set_normal_count(Mesh* mesh, const int indexCount);
		EXPORT void mesh_add_material(Mesh* mesh, Material* material);
		EXPORT void mesh_add_uv_set(Mesh* mesh, UVSet* uvSet);
		EXPORT void destroy_mesh(Mesh* mesh);

		struct Node
		{
			char* name;
			bool visible;

			Mesh* mesh;

			int childCount;
			Node** children;
		};

		EXPORT Node* create_node(const char* name);
		EXPORT void node_set_child_count(Node* node, const int count);
		EXPORT void destroy_node(Node* node);

		struct Model
		{
			Node* root;
			float unit; // relative to centimeter

			int materialCount;
			Material** materials;
		};

		EXPORT Model* create_model();
		EXPORT void model_add_material_count(Model* model);
		EXPORT void destroy_model(Model* model);
	}
}

#endif