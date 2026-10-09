#include "PhysicsCooker.h"
#include "Box3DPhysicsCooker.h"

using namespace GEditor;

Gleam::Scope<PhysicsCooker> PhysicsCooker::Create()
{
	switch (Gleam::AssetUtils::PhysicsBackend())
	{
		case Gleam::AssetBackend::Box3D: return Gleam::CreateScope<Box3DPhysicsCooker>();
		default: return nullptr;
	}
}
