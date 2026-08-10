//
//  UndoSystem.h
//  Editor
//

#pragma once
#include "UndoCommand.h"

#include "World/WorldSubsystem.h"
#include "Container/Pointer.h"
#include "Container/RingBuffer.h"

namespace GEditor {

class UndoSystem final : public Gleam::TickableWorldSubsystem
{
public:

	virtual void Initialize(Gleam::World* world) override;

	virtual void Tick(Gleam::World* world) override;

	void RequestUndo();

	void RequestRedo();

	void BeginTransformTransaction(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void BeginEntityTransaction(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void BeginSingletonTransaction(uint32_t typeHash);

	void EndTransaction();

	void RecordEntityCreation(Gleam::EntityHandle entity);

	void DestroyEntities(const Gleam::TArray<Gleam::EntityHandle>& entities);

	bool CanUndo() const
	{
		return mUndoStack.empty() == false;
	}

	bool CanRedo() const
	{
		return mRedoStack.empty() == false;
	}

	Gleam::TStringView GetUndoName() const;

	Gleam::TStringView GetRedoName() const;

private:

	static constexpr size_t HistoryLimit = 128;

	enum class Request
	{
		None,
		Undo,
		Redo
	};

	struct ComponentState
	{
		Gleam::EntityHandle entity = Gleam::InvalidEntity;
		uint32_t typeHash = 0;
		Gleam::TString data;
	};

	struct Transaction
	{
		Gleam::TArray<Gleam::EntityHandle> entities;
		Gleam::TArray<Gleam::Transform> transforms;
		Gleam::TArray<ComponentState> components;
		uint32_t singleton = 0;
	};

	void Push(Gleam::Scope<UndoCommand>&& command);

	Gleam::TArray<Gleam::Transform> CaptureTransforms() const;

	Gleam::TArray<ComponentState> CaptureComponents() const;

	Gleam::World* mEditWorld = nullptr;

	Transaction mTransaction;

	Gleam::FixedRingBuffer<Gleam::Scope<UndoCommand>, HistoryLimit> mUndoStack;

	Gleam::FixedRingBuffer<Gleam::Scope<UndoCommand>, HistoryLimit> mRedoStack;

	Request mRequest = Request::None;

};

} // namespace GEditor
