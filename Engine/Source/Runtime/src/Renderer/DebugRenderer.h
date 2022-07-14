#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderTypes.h"
#include "CommandBuffer.h"

namespace Gleam {

struct DebugLine
{
	Vector3 start;
	Vector3 end;
	Color32 color;
};

struct DebugTriangle
{
	Vector3 vertex1;
	Vector3 vertex2;
	Vector3 vertex3;
	Color32 color;
};

class DebugRenderer final : public Renderer
{
public:

	DebugRenderer();

	~DebugRenderer();

	virtual void Render() override;

	void DrawLine(const Vector3& start, const Vector3& end, Color32 color, bool depthTest = true);

	void DrawTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3, Color32 color, bool depthTest = true);

	void DrawQuad(const Vector3& center, float width, float height, Color32 color, bool depthTest = true);

private:

	TArray<DebugLine> mLines;
	TArray<DebugLine> mDepthLines;

	TArray<DebugTriangle> mTrianlges;
	TArray<DebugTriangle> mDepthTrianlges;

	CommandBuffer mCommandBuffer;
	GraphicsShader mDebugProgram;
	VertexBuffer<DebugVertex> mVertexBuffer;

};

} // namespace Gleam
