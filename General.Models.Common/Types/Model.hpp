#ifndef GENERAL_MODELS_COMMON_MODEL_HPP
#define GENERAL_MODELS_COMMON_MODEL_HPP

namespace General
{
	namespace Models
	{
		struct Vertex
		{
			float position[3];
			float uv0[2];
		};

		struct VertexIndex
		{
			int index0;
			int index1;
			int index2;
		};

		struct Material
		{
			const char* name;
			const char* ambient;
			const char* diffuse;
			const char* emissive;
			const char* specular;
		};

		struct Mesh
		{
			const char* name;

			int vertexCount;
			Vertex* vertices; 

			int indexCount;
			VertexIndex* indices;

			int materialCount;
			Material** materials;
		};

		struct Model
		{
			int meshCount;
			Mesh** meshes;

			int materialCount;
			Material** materials;
		};

		GENERAL_API Mesh* create_mesh();
		GENERAL_API void mesh_set_name(Mesh* mesh, const char* name);
		GENERAL_API void mesh_set_vertex_count(Mesh* mesh, const int vertexCount);
		GENERAL_API void mesh_set_index_count(Mesh* mesh, const int indexCount);
		GENERAL_API void mesh_add_material(Mesh* mesh, Material* material);
		GENERAL_API void destroy_mesh(Mesh* mesh);

		GENERAL_API Material* create_material(const char* name);
		GENERAL_API void material_set_ambient(Material* material, const char* filename);
		GENERAL_API void material_set_diffuse(Material* material, const char* filename);
		GENERAL_API void material_set_emissive(Material* material, const char* filename);
		GENERAL_API void material_set_specular(Material* material, const char* filename);
		GENERAL_API Material* find_material(Material** materials, const int materialCount, const char* materialName);
		GENERAL_API void destroy_material(Material* material);

		GENERAL_API Model* create_model();
		GENERAL_API void model_add_mesh_count(Model* model);
		GENERAL_API void model_add_material_count(Model* model);
		GENERAL_API void destroy_model(Model* model);
	}
}

#endif