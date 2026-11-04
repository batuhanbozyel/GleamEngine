#include "gpch.h"
#include "SceneView.h"
#include "RenderSurface.h"
#include "RenderTexture.h"

#include "World/World.h"
#include "World/Components/Camera.h"

namespace Gleam {

Rect SceneView::GetRect() const
{
	const auto output = target ? target->GetDescriptor().size : mSurface->GetSize();
	const auto& rect = world->GetEntityManager().GetComponent<Camera>(camera).viewportRect;
	const float minX = Math::Round(Math::Clamp(rect.offset.x, 0.0f, 1.0f) * output.width);
	const float minY = Math::Round(Math::Clamp(rect.offset.y, 0.0f, 1.0f) * output.height);
	const float maxX = Math::Round(Math::Clamp(rect.offset.x + rect.size.width, 0.0f, 1.0f) * output.width);
	const float maxY = Math::Round(Math::Clamp(rect.offset.y + rect.size.height, 0.0f, 1.0f) * output.height);
	return Rect(Size(maxX - minX, maxY - minY), Float2(minX, minY));
}

Size SceneView::GetResolution() const
{
	return GetRect().size;
}

} // namespace Gleam
