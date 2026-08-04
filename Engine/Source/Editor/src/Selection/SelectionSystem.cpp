//
//  SelectionSystem.cpp
//  Editor
//

#include "SelectionSystem.h"

using namespace GEditor;

void SelectionSystem::SelectEntity(Gleam::EntityHandle entity)
{
	mSelectedEntity = entity;
	mSelectedSingleton = 0;
}

void SelectionSystem::SelectSingleton(uint32_t typeHash)
{
	mSelectedSingleton = typeHash;
	mSelectedEntity = Gleam::InvalidEntity;
}
