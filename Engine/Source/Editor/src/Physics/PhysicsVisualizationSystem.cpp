//
//  PhysicsVisualizationSystem.cpp
//  Editor
//

#include "PhysicsVisualizationSystem.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

#include "Math/Float4x4.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/DebugRenderer.h"

#include "World/EntityManager.h"
#include "World/Components/RigidBody.h"

using namespace GEditor;

namespace PhysicsVisualizationUtils {

static constexpr uint32_t kCircleSegments = 24;

static constexpr Gleam::Color32 kStaticColor = Gleam::Color32(90, 210, 120, 255);
static constexpr Gleam::Color32 kKinematicColor = Gleam::Color32(90, 170, 240, 255);
static constexpr Gleam::Color32 kDynamicColor = Gleam::Color32(240, 200, 90, 255);
static constexpr Gleam::Color32 kTriggerColor = Gleam::Color32(220, 120, 220, 255);

static Gleam::Color32 ResolveColor(const Gleam::RigidBody& rigidBody, bool isTrigger)
{
	if (isTrigger)
	{
		return kTriggerColor;
	}

	switch (rigidBody.type)
	{
		case Gleam::RigidBodyType::Static: return kStaticColor;
		case Gleam::RigidBodyType::Kinematic: return kKinematicColor;
		default: return kDynamicColor;
	}
}

static void DrawArc(Gleam::DebugRenderer* renderer,
					const Gleam::Float4x4& transform,
					const Gleam::Float3& axisU,
					const Gleam::Float3& axisV,
					float radius,
					float startAngle,
					float endAngle,
					Gleam::Color32 color,
					bool depthTest)
{
	const float step = (endAngle - startAngle) / kCircleSegments;

	Gleam::Float3 previous = transform * (axisU * radius);
	for (uint32_t i = 1; i <= kCircleSegments; ++i)
	{
		const float angle = startAngle + step * i;
		const Gleam::Float3 local = axisU * (Gleam::Math::Cos(angle) * radius) + axisV * (Gleam::Math::Sin(angle) * radius);
		const Gleam::Float3 current = transform * local;

		renderer->DrawLine(previous, current, color, depthTest);
		previous = current;
	}
}

static void DrawWireSphere(Gleam::DebugRenderer* renderer, const Gleam::Float4x4& transform, float radius, Gleam::Color32 color, bool depthTest)
{
	DrawArc(renderer, transform, Gleam::Float3::right, Gleam::Float3::up, radius, 0.0f, Gleam::Math::TWO_PI, color, depthTest);
	DrawArc(renderer, transform, Gleam::Float3::right, Gleam::Float3::forward, radius, 0.0f, Gleam::Math::TWO_PI, color, depthTest);
	DrawArc(renderer, transform, Gleam::Float3::up, Gleam::Float3::forward, radius, 0.0f, Gleam::Math::TWO_PI, color, depthTest);
}

static void DrawWireCapsule(Gleam::DebugRenderer* renderer,
							const Gleam::Float4x4& transform,
							float radius,
							float halfSegment,
							Gleam::Color32 color,
							bool depthTest)
{
	const Gleam::Float3 top{ 0.0f, halfSegment, 0.0f };
	const Gleam::Float3 bottom{ 0.0f, -halfSegment, 0.0f };

	const auto topTransform = transform * Gleam::Float4x4::Translate(top);
	const auto bottomTransform = transform * Gleam::Float4x4::Translate(bottom);

	DrawArc(renderer, topTransform, Gleam::Float3::right, Gleam::Float3::forward, radius, 0.0f, Gleam::Math::TWO_PI, color, depthTest);
	DrawArc(renderer, bottomTransform, Gleam::Float3::right, Gleam::Float3::forward, radius, 0.0f, Gleam::Math::TWO_PI, color, depthTest);

	DrawArc(renderer, topTransform, Gleam::Float3::right, Gleam::Float3::up, radius, 0.0f, Gleam::Math::PI, color, depthTest);
	DrawArc(renderer, topTransform, Gleam::Float3::forward, Gleam::Float3::up, radius, 0.0f, Gleam::Math::PI, color, depthTest);

	DrawArc(renderer, bottomTransform, Gleam::Float3::right, Gleam::Float3::up, radius, Gleam::Math::PI, Gleam::Math::TWO_PI, color, depthTest);
	DrawArc(renderer, bottomTransform, Gleam::Float3::forward, Gleam::Float3::up, radius, Gleam::Math::PI, Gleam::Math::TWO_PI, color, depthTest);

	const Gleam::Float3 offsets[] =
	{
		Gleam::Float3::right * radius,
		Gleam::Float3::right * -radius,
		Gleam::Float3::forward * radius,
		Gleam::Float3::forward * -radius
	};

	for (const auto& offset : offsets)
	{
		renderer->DrawLine(transform * (top + offset), transform * (bottom + offset), color, depthTest);
	}
}

} // namespace PhysicsVisualizationUtils

bool PhysicsVisualizationSystem::IsVisible(const Gleam::RigidBody& rigidBody) const
{
	switch (rigidBody.type)
	{
		case Gleam::RigidBodyType::Static: return mSettings.flags.Has(Gleam::PhysicsVisualizationFlag::StaticBodies);
		case Gleam::RigidBodyType::Kinematic: return mSettings.flags.Has(Gleam::PhysicsVisualizationFlag::KinematicBodies);
		default: return mSettings.flags.Has(Gleam::PhysicsVisualizationFlag::DynamicBodies);
	}
}

void PhysicsVisualizationSystem::OnUpdate(Gleam::EntityManager& entityManager)
{
	const bool drawColliders = mSettings.flags.Has(Gleam::PhysicsVisualizationFlag::Colliders);
	const bool drawTriggers = mSettings.flags.Has(Gleam::PhysicsVisualizationFlag::Triggers);
	if (drawColliders == false && drawTriggers == false)
	{
		return;
	}

	static auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	auto pipeline = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default);
	if (pipeline == nullptr || pipeline->HasRenderer<Gleam::DebugRenderer>() == false)
	{
		return;
	}

	auto renderer = pipeline->GetRenderer<Gleam::DebugRenderer>();
	const bool depthTest = mSettings.depthTest;

	entityManager.ForEach<Gleam::Entity, Gleam::RigidBody>([&](const Gleam::Entity& entity, const Gleam::RigidBody& rigidBody)
	{
		if (entity.IsActive() == false || IsVisible(rigidBody) == false)
		{
			return;
		}

		const auto bodyTransform = Gleam::Float4x4::TRS(entity.GetWorldPosition(), entity.GetWorldRotation(), entity.GetWorldScale());

		const auto shouldDraw = [&](bool isTrigger)
		{
			return isTrigger ? drawTriggers : drawColliders;
		};

		for (const auto& collider : rigidBody.colliders.boxes)
		{
			if (shouldDraw(collider.isTrigger) == false)
			{
				continue;
			}

			const auto transform = bodyTransform * Gleam::Float4x4::TRS(collider.center, collider.rotation, 1.0f);
			const auto halfSize = collider.size * 0.5f;
			renderer->DrawBoundingBox(Gleam::BoundingBox(-halfSize, halfSize), transform, PhysicsVisualizationUtils::ResolveColor(rigidBody, collider.isTrigger), depthTest);
		}

		for (const auto& collider : rigidBody.colliders.spheres)
		{
			if (shouldDraw(collider.isTrigger) == false)
			{
				continue;
			}

			const auto transform = bodyTransform * Gleam::Float4x4::Translate(collider.center);
			PhysicsVisualizationUtils::DrawWireSphere(renderer, transform, collider.radius, PhysicsVisualizationUtils::ResolveColor(rigidBody, collider.isTrigger), depthTest);
		}

		for (const auto& collider : rigidBody.colliders.capsules)
		{
			if (shouldDraw(collider.isTrigger) == false)
			{
				continue;
			}

			// Matches the segment the physics world builds, so the gizmo lines up with the simulated shape
			const float halfSegment = Gleam::Math::Max(0.0f, collider.height * 0.5f - collider.radius);
			const auto transform = bodyTransform * Gleam::Float4x4::TRS(collider.center, collider.rotation, 1.0f);
			PhysicsVisualizationUtils::DrawWireCapsule(renderer, transform, collider.radius, halfSegment, PhysicsVisualizationUtils::ResolveColor(rigidBody, collider.isTrigger), depthTest);
		}
	});
}
