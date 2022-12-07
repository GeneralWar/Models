// General.Models.ConsoleTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _CRT_SECURE_NO_WARNINGS
#include <General.Cpp/General.hpp>
#include <General.Models.Fbx/Fbx.hpp>
#include <General.Models.Assimp/Assimp.hpp>
#include <General.Models.Common/Common.hpp>
using namespace General::Models;

void printHierarchy(Node* node, int level)
{
	for (int i = 0; i < level; ++i)
	{
		printf("-");
	}
	printf("%s, local: (%06f, %06f, %06f), (%06f, %06f, %06f), (%06f, %06f, %06f)\n", node->name, node->localPosition.x, node->localPosition.y, node->localPosition.z, node->localRotation.x, node->localRotation.y, node->localRotation.z, node->localScaling.x, node->localScaling.y, node->localScaling.z);

	for (int i = 0; i < node->childCount; ++i)
	{
		printHierarchy(node->children[i], level + 1);
	}
}

void import_test(const char* filename, const Model* (*load_model_function)(const ImportParams* params))
{
	ImportParams params = { };
	params.unitLevel = UNIT_LEVEL_METER;
	params.filename = filename;
	//const Model* model = load_model_from_fbx(&params);
	const Model* model = load_model_function(&params);
	if (!model)
	{
		printf("load model %s failed\n", params.filename);
		return;
	}

	int vertexCount = 0, triangleCount = 0, indexCount = 0;
	int weightCollectionCount = 0, weightDataCount = 0;
	for (int i = 0; i < model->meshCount; ++i)
	{
		const Mesh* mesh = model->meshes[i];
		vertexCount += mesh->vertexCount;
		triangleCount += mesh->triangleCount;
		indexCount += mesh->triangleCount * 3;
		for (int ci = 0; ci < mesh->weightCollectionCount; ++ci)
		{
			++weightCollectionCount;
			weightDataCount += mesh->weightCollections[ci]->weightCount;
		}
	}

	printf("Load model %s\n", params.filename);
	printf("There are %d meshes, %d vertices, %d triangles and %d indices in total\n", model->meshCount, vertexCount, triangleCount, indexCount);

	int animationCount = model->animationCount, animationNodeCount = 0, frameCount = 0;
	if (animationCount)
	{
		for (int animationIndex = 0; animationIndex < model->animationCount; ++animationIndex)
		{
			Animation* animation = model->animations[animationIndex];
			if (animation && animation->curve)
			{
				const AnimationCurve* curve = animation->curve;
				if (curve->nodeCount && curve->nodes)
				{
					animationNodeCount += curve->nodeCount;
					for (int nodeIndex = 0; nodeIndex < curve->nodeCount; ++nodeIndex)
					{
						frameCount += curve->nodes[nodeIndex]->frameCount;
					}
				}
			}
		}
	}

	printf("There are %d animation(s), %d animation node(s) and %d frame(s) in total\n", animationCount, animationNodeCount, frameCount);
	printf("There are %d weight collections and %d weight data records\n", weightCollectionCount, weightDataCount);

	printHierarchy(model->root, 0);

	destroy_model(const_cast<Model*>(model));
}

const char* check_animation_node_type_name(const AnimationCurveNodeType& type)
{
	switch (type)
	{
	case AnimationCurveNodeType::AnimationCurveNodeTranslation: return "Translation";
	case AnimationCurveNodeType::AnimationCurveNodeRotation: return "Rotation";
	case AnimationCurveNodeType::AnimationCurveNodeScaling: return "Scaling";
	default: return "(undefined type)";
	}
}

void save_animation(const Animation* animation, const char* filename)
{
	FILE* file = fopen(filename, "w");

	fprintf(file, "%s:\n", filename);
	fprintf(file, "%s:\n", animation->name);

	const AnimationCurve* curve = animation->curve;
	for (int nodeIndex = 0; nodeIndex < curve->nodeCount; ++nodeIndex)
	{
		const AnimationCurveNode* curveNode = curve->nodes[nodeIndex];
		if (strcmp("LeftArm", curveNode->target->name) || AnimationCurveNodeType::AnimationCurveNodeRotation != curveNode->type)
		{
			continue;
		}

		fprintf(file, "\tNode %s (%s):\n", curveNode->target->name, check_animation_node_type_name(curveNode->type));

		const AnimationCurveFrame* frame = curveNode->frames;
		for (int frameIndex = 0; frameIndex < curveNode->frameCount; ++frameIndex, ++frame)
		{
			fprintf(file, "\t\tFrame %03d: ", frameIndex);
			switch (curveNode->type)
			{
			case AnimationCurveNodeType::AnimationCurveNodeRotation:
				fprintf(file, "%06f, %06f, %06f, %06f\n", frame->data.vector4.x, frame->data.vector4.y, frame->data.vector4.z, frame->data.vector4.w);
				break;
			default:
				fprintf(file, "%06f, %06f, %06f\n", frame->data.vector3.x, frame->data.vector3.y, frame->data.vector3.z);
				break;
			}
		}
	}

	fclose(file);
}

void save_hierarchy(FILE* file, const Node* node, const std::string& prefix)
{
	fprintf(file, "%s%s: translation: %06f, %06f, %06f rotation: %06f, %06f, %06f scaling: %06f, %06f, %06f\n", prefix.c_str(), node->name,
		node->localPosition.x, node->localPosition.y, node->localPosition.z,
		node->localRotation.x, node->localRotation.y, node->localRotation.z,
		node->localScaling.x, node->localScaling.y, node->localScaling.z);

	for (int childIndex = 0; childIndex < node->childCount; ++childIndex)
	{
		save_hierarchy(file, node->children[childIndex], prefix + "\t");
	}
}

void save_hierarchy(const Node* root, const char* filename)
{
	FILE* file = fopen(filename, "w");

	fprintf(file, "%s:\n", filename);

	save_hierarchy(file, root, "");

	fclose(file);
}

void save_weights(const Model* model, const char* filename)
{
	FILE* file = fopen(filename, "w");

	fprintf(file, "%s:\n", filename);

	for (int meshIndex = 0; meshIndex < model->meshCount; ++meshIndex)
	{
		const Mesh* mesh = model->meshes[meshIndex];
		fprintf(file, "\tMesh %s:\n", mesh->name);
		for (int collectionIndex = 0; collectionIndex < mesh->weightCollectionCount; ++collectionIndex)
		{
			const WeightCollection* collection = mesh->weightCollections[collectionIndex];
			fprintf(file, "\t\tBone %s:\n", collection->bone->name);

			const WeightData* weight = collection->weights;
			for (int weightIndex = 0; weightIndex < collection->weightCount; ++weightIndex, ++weight)
			{
				fprintf(file, "\t\t\tVertex: %d, Weight: %06f\n", weight->index, weight->weight);
			}
		}
	}

	fclose(file);
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//const char* filename = "E:\\Projects\\CrossEngine\\Private\\Projects\\Cross\\Assets\\Models\\Stone_Frog\\Stone_Frog.fbx";
	//const char* filename = "E:\\Projects\\Samples\\LearnOpenGL\\resources\\objects\\vampire\\dancing_vampire.dae";
	const char* filename = "E:\\Projects\\CrossEngine\\Private\\Projects\\Cross\\Assets\\Models\\Vampire\\Vampire.fbx";
	//import_test(filename, load_model_from_assimp);
	//import_test(filename, load_model_from_fbx);

	ImportParams params = { };
	params.unitLevel = UNIT_LEVEL_METER;
	params.filename = filename;

	const Model* model1 = load_model_from_assimp(&params);
	//save_animation(model1->animations[0], "Animation1.txt");
	//save_hierarchy(model1->root, "Hierarchy1.txt");
	//save_weights(model1, "Weights1.txt");
	destroy_model(const_cast<Model*>(model1));

	const Model* model2 = load_model_from_fbx(&params);
	//save_animation(model2->animations[0], "Animation2.txt");
	//save_hierarchy(model2->root, "Hierarchy2.txt");
	//save_weights(model2, "Weights2.txt");
	destroy_model(const_cast<Model*>(model2));

	system("pause");
	return 0;
}