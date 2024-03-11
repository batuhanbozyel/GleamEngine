# Gleam Engine
Gleam Engine is a high quality modern 3D game engine written in C++ targeting desktop platforms (Windows and Mac). Designed with modularity and data-oriented principles in mind.

## Table of Contents
* [Getting started](#getting-started)
    * [Windows](#windows)
    * [Mac](#mac)
* [Workspace directory](#workspace-directory)
* [Naming Convention](#naming-convention)
* [Math Conventions](#math-conventions)
* [Engine Design](#engine-design)

# Getting started
Follow the instructions below to compile and run the engine from source.

## Windows
* Install Visual Studio 2022
* Install Git with LFS
* Install CMake
* Clone repo recursively (with LFS)
* Run **Win-GenerateProjects.bat**
* Open `build/GleamEngine.sln`
* Compile Editor project (hit F7 or CTRL+Shift+B)
* Run Editor (hit F5 key)

## Mac
* Install XCode 15 or newer
* Install Git with LFS
* Install CMake
* Install [Metal Shader Converter 1.1](https://download.developer.apple.com/Developer_Tools/Metal_shader_converter_1.1/Metal_Shader_Converter_1.1.pkg)
* Clone repo recursively (with LFS)
* Run **Mac-GenerateProjects.command**
* Open `build/GleamEngine.xcodeproj`
* Switch target to Editor (CMake sets ALL_BUILD as target by default on XCode)
* Compile Editor project (hit Cmd+B)
* Run Editor (hit Cmd+R key)

## Workspace directory

- **Assets/** - Engine-ready asset files used by the engine and editor
  - **Shaders/** - Precompiled shader binaries
- **Engine/** - Engine files
  - **Source/** - Engine source
    - **Runtime/** - Runtime source code
    - **Editor/** - Editor soruce code
    - **UnitTest/** - Unit tests
  - **ThirdParty/** - Third-party libraries
- **Tools/** - Engine tools source code and binaries

## Naming Convention
Here's a table outlining the recommended naming conventions for Gleam Engine:

| Element                   | Convention                        | Example                                      |
|----------------------------|-----------------------------------|-------------------------------------------------|
| Namespaces                 | PascalCase                       | `Gleam::IRenderer`                      |
| Types (structs, unions)     | PascalCase                       | `RenderPassDescriptor`, `RendererConfig`                |
| Classes                     | PascalCase                       | `GraphicsObject`, `Material`                         |
| Enums                       | PascalCase                       | `TextureFormat`, `ProjectionType`                     |
| Functions                   | PascalCase                       | `CreateMaterial`, `AddSystem`             |
| Methods (inside classes)    | PascalCase                       | `GetComponent`, `SetFieldOfView`                     |
| Members       | mPascalCase                      | `mPosition`, `mMaterial`                           |
| Variables (local/member)   | camelCase                        | `passData`, `stagingBuffer`                      |
| Arguments (function/method) | camelCase                        | `entryPoint`, `entityManager`                 |
| Constants                   | SCREAMING_SNAKE_CASE             | `CBV_SRV_HEAP_SIZE`, `PUSH_CONSTANT_SIZE`                     |
| Macros                      | SCREAMING_SNAKE_CASE             | `PLATFORM_WINDOWS`, `ENABLE_ASSERTS`                  |

**Additional Notes:**
* Follow consistent casing throughout your code for readability and maintainability.
* Adhere to these conventions as a general guideline, but use your judgement for specific cases where clarity might be improved by deviating slightly.
* **Globals are not allowed:** Instead, consider using singletons, dependency injection, or configuration files to manage data that needs to be accessed from various parts of your code.

## Math Conventions
Gleam Engine adopts the following conventions for mathematical operations:

* **Coordinate System:**
    * **Right-handed:** Positive rotations follow the right-hand rule.
    * **X-right:** The positive X-axis points to the right of the screen in the viewport.
    * **Z-forward:** The positive Z-axis points forward (out of the screen) in the viewport.
    * **Y-up:** The positive Y-axis points up in the viewport.
* **Matrix Ordering:**
    * **Row-major:** Matrices are stored in row-major order, where elements are accessed by row and then column.

## Engine Design
For detailed information on the Gleam Engine's design choices for various parts (e.g. Render Graph, Entity Component System), please refer to the engine design document [here](docs/design.md).