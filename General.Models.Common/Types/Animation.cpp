#include "pch.h"
#include "Animation.hpp"
#include <immintrin.h>

namespace General
{
	namespace Models
	{
		void scale_animation_curve_frame_data(AnimationCurveFrameData* data, const float scale)
		{
			__m128 result = _mm_mul_ps(_mm_load_ps(data->values), _mm_load1_ps(&scale));
			memcpy(data->values, &result, sizeof(float) * 4);
		}

		AnimationCurveNode* create_animation_curve_node(const Node* target, const AnimationCurveNodeType type, const int frameCount, const AnimationCurveFrame* frames)
		{
			AnimationCurveNode* instance = g_alloc_struct<AnimationCurveNode>();
			*const_cast<AnimationCurveNodeType*>(&instance->type) = type;
			*const_cast<const Node**>(&instance->target) = target;
			*const_cast<int*>(&instance->frameCount) = frameCount;
			*const_cast<const AnimationCurveFrame**>(&instance->frames) = g_copy_array(frames, frameCount);
			return instance;
		}

		void destroy_animation_curve_node(AnimationCurveNode* instance)
		{
			if (instance->frames) free(const_cast<AnimationCurveFrame*>(instance->frames));
			g_free_struct(instance);
		}

		AnimationCurve* create_animation_curve()
		{
			return g_alloc_struct<AnimationCurve>();
		}

		void animation_curve_add_node(AnimationCurve* instance, AnimationCurveNode* node)
		{
			CHECK(node, );

			int index = instance->nodeCount;
			g_resize_array(const_cast<AnimationCurveNode***>(&instance->nodes), const_cast<int*>(&instance->nodeCount), index + 1);
			*const_cast<AnimationCurveNode**>(instance->nodes + index) = node;
		}

		void destroy_animation_curve(AnimationCurve* instance)
		{
			if (instance->nodes)
			{
				AnimationCurveNode** node = const_cast<AnimationCurveNode**>(instance->nodes);
				for (int i = 0; i < instance->nodeCount; ++i, ++node)
				{
					destroy_animation_curve_node(*node);
				}
				free(const_cast<AnimationCurveNode**>(instance->nodes));
			}
			free(instance);
		}

		Animation* create_animation(const char* name)
		{
			Animation* instance = g_alloc_struct<Animation>();
			instance->name = g_copy_string(name);
			*const_cast<AnimationCurve**>(&instance->curve) = create_animation_curve();
			return instance;
		}

		void destroy_animation(Animation* instance)
		{
			if (instance->curve) destroy_animation_curve(const_cast<AnimationCurve*>(instance->curve));
			if (instance->name) free(const_cast<char*>(instance->name));
			g_free_struct(instance);
		}
	}
}