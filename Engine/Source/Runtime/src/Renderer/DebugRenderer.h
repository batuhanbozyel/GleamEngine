#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Renderer.h"
#include "ShaderTypes.h"
#include "CommandBuffer.h"

namespace Gleam {

enum class PrimitiveTopology;

struct DebugLine
{
    DebugVertex start;
    DebugVertex end;
};

struct DebugTriangle
{
    DebugVertex vertex1;
    DebugVertex vertex2;
    DebugVertex vertex3;
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
    
    void RenderPrimitive(uint32_t primitiveCount, uint32_t bufferOffset, PrimitiveTopology topology, bool depthTest) const;
    
	TArray<DebugLine> mLines;
	TArray<DebugLine> mDepthLines;

	TArray<DebugTriangle> mTrianlges;
	TArray<DebugTriangle> mDepthTrianlges;

	CommandBuffer mCommandBuffer;
	GraphicsShader mDebugProgram;
    
    TArray<DebugVertex> mStagingBuffer;
	VertexBuffer<DebugVertex> mVertexBuffer;

};

} // namespace Gleam
