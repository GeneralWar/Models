// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <assert.h>

#include <General.Cpp/General.hpp>

#include "../General.Models.Common/Common.hpp"

#include <fbxsdk.h>

#ifndef TRACE
#define TRACE(format, ...) Tracer::Log(LOG_LEVEL_LOG, "[General.Model.Fbx]", format, __VA_ARGS__);
#endif

#ifndef TRACE_WARN
#define TRACE_WARN(format, ...) Tracer::Log(LOG_LEVEL_WARN, "[General.Model.Fbx]", format, __VA_ARGS__);
#endif

#ifndef TRACE_ERROR
#define TRACE_ERROR(format, ...) Tracer::Log(LOG_LEVEL_ERROR, "[General.Model.Fbx]", format, __VA_ARGS__);
#endif

#endif //PCH_H
