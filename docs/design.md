# Engine Design

## Table of Contents
* [Entity Component System (ECS)](#entity-component-system-ecs)
* [Renderer Hardware Interface (RHI)](#renderer-hardware-interface-rhi)

## Entity Component System (ECS)

Gleam Engine leverages a pure Entity Component System (ECS) architecture for managing game entities and their data. This approach prioritizes data-oriented design and avoids traditional object-oriented inheritance hierarchies.

**Core Concepts:**
* **Entity:** A lightweight representation of a game object in the world. Entities themselves hold no data, but act as identifiers for components.
* **Component:** A data structure that defines an aspect or property of an entity. Components can hold various types of data, like transform, mesh, or behavior.
* **System:** A class responsible for processing groups of entities that share specific components. Systems iterate through entities with matching components and update them based on game logic.
* **EntityManager:** A manager class that provides methods for creating and destroying entities, adding, removing, and accessing components attached to them.
* **World:** The central hub that manages the ECS world. You can add systems to the world using the `AddSystem` call.

**Key Interfaces:**
* **ComponentSystem:** (interface)
    * `OnCreate(EntityManager& entityManager)`
    * `OnUpdate(EntityManager& entityManager)`
    * `OnFixedUpdate(EntityManager& entityManager)`
    * `OnDestroy(EntityManager& entityManager)`
* **EntityManager:** (interface)
    * **Entity Creation:**
        * `CreateEntity()`
        * `CreateEntity(Types&& ... components)`
    * **Entity Management:**
        * `DestroyEntity(Entity entity)`
        * `DestroyEntity(const TArray<Entity>& entities)`
    * **Component Management:**
        * `SetSingletonComponent(Args&&... args)`
        * `GetSingletonComponent<T>()`
        * `AddComponent(Entity entity, Args&&... args)`
        * `SetComponent(Entity entity, Args&&... args)`
        * `RemoveComponent(Entity entity)`
        * `HasComponent(Entity entity)`
        * `GetComponent(Entity entity)`

## Renderer Hardware Interface (RHI)
Gleam Engine prioritizes performance and avoids unnecessary abstraction layers to achieve this. It accomplishes this through a unique approach to its Renderer Hardware Interface (RHI).

**Command Buffers and Compile-Time Decisions:**

* Gleam Engine abstracts native API calls under the `CommandBuffer` class. These functions are not virtual, but have compile-time decided implementations based on the target platform's graphics API (DirectX 12 on Windows, Metal on Mac). This approach eliminates runtime overhead compared to traditional RHI designs that rely on virtual function calls.

**Render Graphs for High-Level Abstraction:**

For higher-level rendering tasks, Gleam Engine utilizes render graphs. These graphs allow you to define the order and dependencies between various rendering passes within your scene.

**Implementing a Renderer with Render Graphs:**

1. **Inherit from IRenderer:** To add a pass to the render graph, you first create a class that inherits from the `IRenderer` interface.
2. **Implement Interface Methods:** The `IRenderer` interface typically defines methods like:
    * `OnCreate(Gleam::GraphicsDevice* device)`: Used to create persistent resources such as shaders.
    * `OnDestroy(Gleam::GraphicsDevice* device)`: Handles cleanup for the renderer's resources.
    * `AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)`: The core method where you define how your renderer contributes to the render graph by adding rendering passes.

**Example: Post-Processing Stack Renderer**

This example demonstrates how a `PostProcessStack` renderer might be implemented using render graphs:

```c++
void PostProcessStack::OnCreate(GraphicsDevice* device)
{
  mFullscreenTriangleVertexShader = device->CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
  mTonemappingFragmentShader = device->CreateShader("tonemappingFragmentShader", ShaderStage::Fragment);
}

void PostProcessStack::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
  // Define data shared between passes
  struct PostProcessData
  {
    TextureHandle colorTarget;
    TextureHandle sceneTarget;
  };

  graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", &: RenderGraphBuilder& builder, PostProcessData& passData
  {
    // Access data from the blackboard
    auto& sceneData = blackboard.Get<SceneRenderingData>();
    const auto& worldData = blackboard.Get<WorldRenderingData>();

    // Define pass outputs and dependencies
    passData.colorTarget = builder.UseColorBuffer(sceneData.backbuffer);
    passData.sceneTarget = builder.ReadTexture(worldData.colorTarget);

    // Update the backbuffer for the next pass
    sceneData.backbuffer = passData.colorTarget;
  },
  this: const CommandBuffer* cmd, const PostProcessData& passData
  {
    // Set up uniforms and pipeline state
    TonemapUniforms uniforms;
    uniforms.sceneRT = passData.sceneTarget;

    PipelineStateDescriptor pipelineDesc;
    cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mTonemappingFragmentShader);
    cmd->SetPushConstant(uniforms);
    cmd->Draw(3);
  });
}