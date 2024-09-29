#include "gpch.h"
#include "ScriptingSystem.h"

using namespace Gleam;

void ScriptingSystem::Initialize(Application* app)
{
	RegisterMetaComponent<Camera>();
}

void ScriptingSystem::Shutdown()
{

}