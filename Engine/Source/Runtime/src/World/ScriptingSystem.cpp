#include "gpch.h"
#include "ScriptingSystem.h"

using namespace Gleam;

void ScriptingSystem::Initialize(Engine* engine)
{
	RegisterMetaComponent<Camera>();
}

void ScriptingSystem::Shutdown()
{

}