//
//  RawVectorLifecycle.h
//  Editor
//

#pragma once
#include "Core/TypeLifecycle.h"

namespace GEditor {

class RawVectorLifecycle final : public Gleam::VectorLifecycle
{
public:

	explicit RawVectorLifecycle(const Gleam::TypeLifecycle& element);

	virtual void Construct(void* obj) const override;

	virtual void Destruct(void* obj) const override;

	virtual void Copy(void* obj, const void* source) const override;

	virtual void Move(void* obj, void* source) const override;

	virtual size_t GetElementSize() const override;

	virtual void* Data(void* obj) const override;

	virtual void Reset(void* obj, size_t count) const override;

private:

	void DestructElements(void* begin, void* end) const;

	const Gleam::TypeLifecycle& mElement;
};

} // namespace GEditor
