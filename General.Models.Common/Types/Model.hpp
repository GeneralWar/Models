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

		struct VertexIndex
		{
			int index0;
			int index1;
			int index2;
		};

		struct Mesh
		{
			const char* name;
			int vertexCount;
			Vertex* vertices; 
			VertexIndex* indices;
			int indexCount;
		};

		struct Model
		{
			int meshCount;
			Mesh** meshes;
		};

		GENERAL_API Mesh* create_mesh();
		GENERAL_API void mesh_set_name(Mesh* mesh, const char* name);
		GENERAL_API void mesh_set_vertex_count(Mesh* mesh, const int vertexCount);
		GENERAL_API void mesh_set_index_count(Mesh* mesh, const int indexCount);
		GENERAL_API void destroy_mesh(Mesh* mesh);

		GENERAL_API Model* create_model();
		GENERAL_API void model_add_mesh_count(Model* model);
		GENERAL_API void destroy_model(Model* model);
	}
}

#endif