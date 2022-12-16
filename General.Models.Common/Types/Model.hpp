#ifndef GENERAL_MODELS_COMMON_MODEL_HPP
#define GENERAL_MODELS_COMMON_MODEL_HPP

namespace General
{
	namespace Models
	{
		/****************************************************************
		* Axis direction: +x == right, +y == up, +z == into screen
		* ***************************************************************/

		struct Node;
		struct Animation;

		struct Vector2
		{
			union
			{
				struct
				{
					float x;
					float y;
				};
				float values[2];
			};
		};

		struct Vector3
		{
			union
			{
				struct
				{
					float x;
					float y;
					float z;
				};
				float values[3];
			};
		};

		struct Vector4
		{
			union
			{
				struct
				{
					float x;
					float y;
					float z;
					float w;
				};
				float values[4];
			};
		};

		EXPORT Vector3 vector3_scale(const Vector3 v, const float scaling);

		struct Transform
		{
			Vector3 translation;
			Vector3 rotation; // euler angles, in degrees
			Vector3 scaling;
		};

		struct Matrix // row matrix
		{
			union
			{
				struct
				{
					float row0[4];
					float row1[4];
					float row2[4];
					float row3[4];
				};
				float values[16];
			};
		};

		struct Vertex
		{
			Vector3 position;
			Vector3 normal;
			Vector2 uv[4];
		};

		struct Triangle
		{
			union
			{
				struct
				{
					int index0;
					int index1;
					int index2;
				};
				int indices[3];
			};
		};

		struct UV
		{
			int vertexIndex;
			Vector2 uv;
		};

		struct Normal
		{
			int vertexIndex;
			Vector3 normal;
		};

		struct UVSet
		{
			char* name;
			int uvCount;
			UV* uvArray; // 与Index数组对应
		};

		EXPORT UVSet* create_uv_set(const char* name);
		EXPORT void uv_set_set_uv_count(UVSet* set, const int count);
		EXPORT UVSet* find_uv_set(UVSet** sets, const int setCount, const char* setName);
		EXPORT void destroy_uv_set(UVSet* set);
		
		struct MaterialTexture
		{
			char* texture;
			char* uvSet;
			bool useDefaultUVSet;
		};

		EXPORT MaterialTexture* create_material_texture();
		EXPORT void set_material_texture(MaterialTexture* instance, const char* filename, const char* uvSet);
		EXPORT void destroy_material_texture(MaterialTexture* instance);

		struct Material
		{
			char* name;
			MaterialTexture* ambient;
			MaterialTexture* diffuse;
			MaterialTexture* emissive;
			MaterialTexture* specular;
		};

		EXPORT Material* create_material(const char* name);
		EXPORT void material_set_ambient(Material* instance, const char* filename, const char* uvSet);
		EXPORT void material_set_diffuse(Material* instance, const char* filename, const char* uvSet);
		EXPORT void material_set_emissive(Material* instance, const char* filename, const char* uvSet);
		EXPORT void material_set_specular(Material* instance, const char* filename, const char* uvSet);
		EXPORT Material* find_material(Material** instance, const int materialCount, const char* materialName);
		EXPORT void destroy_material(Material* instance);

		struct WeightData
		{
			int index;
			float weight;
		};

		struct WeightCollection
		{
			Node* bone;
			Matrix boneOffset;

			int weightCount;
			WeightData* weights;
		};

		EXPORT WeightCollection* create_weight_collection(Node* bone, Matrix boneTransform, const int weightCount);
		EXPORT void weight_collection_copy_weight(WeightCollection* instance, const int templateIndex, const int newIndex);
		EXPORT void destroy_weight_collection(WeightCollection* instance);

		struct Mesh
		{
			char* name;

			int vertexCount;
			Vertex* vertices; 

			int triangleCount;
			Triangle* triangles;

			int materialCount;
			Material** materials;

			int weightCollectionCount;
			WeightCollection** weightCollections;
		};

		EXPORT Mesh* create_mesh(const char* name);
		EXPORT void mesh_set_vertex_count(Mesh* instance, const int count);
		EXPORT void mesh_set_triangle_count(Mesh* instance, const int count);
		EXPORT void mesh_set_triangles(Mesh* instance, const int count, const Triangle* triangles);
		EXPORT void mesh_add_material(Mesh* instance, Material* material);
		EXPORT void mesh_add_weight_collection(Mesh* instance, WeightCollection* collection);
		EXPORT void mesh_copy_weight(Mesh* instance, const int templateIndex, const int newIndex);
		EXPORT void destroy_mesh(Mesh* instance);

		struct Node
		{
			char* name;
			bool visible;

			Mesh* mesh;

			Node* parent;

			int childCount;
			Node** children;

			Vector3 localPosition;
			Vector3 localRotation; // in degrees
			Vector3 localScaling;
		};

		EXPORT Node* create_node(const char* name);
		EXPORT void node_add_child(Node* instance, Node* child);
		EXPORT void destroy_node(Node* instance);

		struct Model
		{
			Node* root;

			int meshCount;
			Mesh** meshes;

			int materialCount;
			Material** materials;

			int animationCount;
			Animation** animations;
		};

		EXPORT Model* create_model();
		EXPORT void model_add_mesh(Model* instance, Mesh* mesh);
		EXPORT void model_add_material(Model* instance, Material* material);
		EXPORT void model_add_animation(Model* instance, Animation* animation);
		EXPORT void destroy_model(Model* instance);
	}
}

#endif