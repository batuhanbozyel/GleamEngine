#pragma once
#include "Mesh.h"
#include "Camera.h"
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
    
    void DrawMesh(const Mesh* mesh, Color32 color, bool depthTest = true);
    
    void DrawSkeletalMesh(const SkeletalMesh* mesh, Color32 color, bool depthTest = true);
    
    void DrawBoundingBox(const BoundingBox& boundingBox, Color32 color, bool depthTest = true);
    
    void DrawBoundingBox(const BoundingBox& boundingBox, const Matrix4& transform, Color32 color, bool depthTest = true);
    
    void UpdateView(Camera& camera)
    {
        mViewMatrix = camera.GetViewMatrix();
        mProjectionMatrix = camera.GetProjectionMatrix();
    }

private:
    
    void RenderPrimitive(uint32_t primitiveCount, uint32_t bufferOffset, PrimitiveTopology topology, bool depthTest) const;
    
    void RenderMesh(const MeshBuffer& meshBuffer, Color32 color, bool depthTest) const;
    
    void RenderSkeletalMesh(const MeshBuffer& meshBuffer, const TArray<SubmeshDescriptor>& submeshDescriptors, Color32 color, bool depthTest) const;
    
    Matrix4 mViewMatrix;
    Matrix4 mProjectionMatrix;
    
	TArray<DebugLine> mLines;
	TArray<DebugLine> mDepthLines;

	TArray<DebugTriangle> mTrianlges;
	TArray<DebugTriangle> mDepthTrianlges;
    
    TArray<std::pair<const Mesh*, Color32>> mMeshes;
    TArray<std::pair<const Mesh*, Color32>> mDepthMeshes;
    
    TArray<std::pair<const SkeletalMesh*, Color32>> mSkeletalMeshes;
    TArray<std::pair<const SkeletalMesh*, Color32>> mDepthSkeletalMeshes;

	CommandBuffer mCommandBuffer;
	GraphicsShader mDebugProgram;
    GraphicsShader mDebugMeshProgram;
    
    TArray<DebugVertex> mStagingBuffer;
	VertexBuffer<DebugVertex> mVertexBuffer;

};

} // namespace Gleam
