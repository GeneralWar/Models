#ifndef GENERAL_MODELS_COMMON_ANIMATION_HPP
#define GENERAL_MODELS_COMMON_ANIMATION_HPP

#include "Model.hpp"

namespace General
{
	namespace Models
	{
#pragma pack(push, 4)
		struct AnimationCurveFrameData
		{
			union
			{
				Vector3 vector3; // for translation and scaling
				Vector4 vector4; // for rotation in quaternion
				float values[4];
			};
		};

		EXPORT void scale_animation_curve_frame_data(AnimationCurveFrameData* data, const float scaling);

		struct AnimationCurveFrame
		{
			unsigned long long time; // in milliseconds
			AnimationCurveFrameData data;
		};

		enum AnimationCurveNodeType
		{
			None,

			AnimationCurveNodeTranslation,
			AnimationCurveNodeRotation,
			AnimationCurveNodeScaling,
		};

		struct AnimationCurveNode
		{
			const Node* const target;
			const AnimationCurveNodeType type;

			const int frameCount;
			const AnimationCurveFrame* const frames;
		};

		EXPORT AnimationCurveNode* create_animation_curve_node(const Node* target, const AnimationCurveNodeType type, const int frameCount, const AnimationCurveFrame* frames);
		EXPORT void destroy_animation_curve_node(AnimationCurveNode* instance);

		struct AnimationCurve
		{
			const int nodeCount;
			const AnimationCurveNode** nodes;
		};

		EXPORT AnimationCurve* create_animation_curve();
		EXPORT void animation_curve_add_node(AnimationCurve* instance, AnimationCurveNode* node);
		EXPORT void destroy_animation_curve(AnimationCurve* instance);

		struct Animation
		{
			const char* name;
			const float fps;
			const AnimationCurve* const curve;
		};

		EXPORT Animation* create_animation(const char* name, const float fps);
		EXPORT void destroy_animation(Animation* instance);
#pragma pack(pop)
	}
}

#endif