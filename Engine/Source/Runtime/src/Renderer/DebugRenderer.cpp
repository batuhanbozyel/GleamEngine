#include "gpch.h"
#include "DebugRenderer.h"
#include "ShaderLibrary.h"

using namespace Gleam;

DebugRenderer::DebugRenderer()
	: mVertexBuffer(100)
{
	mDebugProgram.vertexShader = ShaderLibrary::CreateShader("debugVertexShader", ShaderStage::Vertex);
	mDebugProgram.fragmentShader = ShaderLibrary::CreateShader("debugFragmentShader", ShaderStage::Fragment);
}

DebugRenderer::~DebugRenderer()
{
    
}

void DebugRenderer::Render()
{

	// reset render queue
	mLines.clear();
	mDepthLines.clear();
	mTrianlges.clear();
	mDepthTrianlges.clear();
}

void DebugRenderer::DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest)
{
	DebugLine line;
	line.start = start;
	line.end = end;
	line.color = color;

	if (depthTest)
	{
		mDepthLines.emplace_back(std::move(line));
	}
	else
	{
		mLines.emplace_back(std::move(line));
	}
}

void DebugRenderer::DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest)
{
	DebugTriangle triangle;
	triangle.vertex1 = v1;
	triangle.vertex2 = v2;
	triangle.vertex3 = v3;
	triangle.color = color;

	if (depthTest)
	{
		mDepthTrianlges.emplace_back(std::move(triangle));
	}
	else
	{
		mTrianlges.emplace_back(std::move(triangle));
	}
}

void DebugRenderer::DrawQuad(const Vector3& center, float width, float height, Color32 color, bool depthTest)
{
	float halftWidth = width / 2.0f;
	float halfHeight = height / 2.0f;

	Vector3 v0{ center.x - halftWidth, center.y, center.z - halfHeight };
	Vector3 v1{ center.x + halftWidth, center.y, center.z - halfHeight };
	Vector3 v2{ center.x + halftWidth, center.y, center.z + halfHeight };
	Vector3 v3{ center.x - halftWidth, center.y, center.z + halfHeight };

	DrawLine(v0, v1, color, depthTest);
	DrawLine(v1, v2, color, depthTest);
	DrawLine(v2, v3, color, depthTest);
	DrawLine(v3, v0, color, depthTest);
}
