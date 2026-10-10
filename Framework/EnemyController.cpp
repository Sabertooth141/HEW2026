#include "EnemyController.h"

#include <algorithm>
#include <cmath>

#include "GameObject.h"
#include "Scene.h"

void EnemyController::LateUpdate(float deltaTime)
{
	if (deltaTime <= 0.f || !std::isfinite(deltaTime)) return;
	auto* scene = owner->GetContext().gameScene;
	if (!scene) return;

	for (const auto& object : scene->GetObjects())
	{
		if (object->GetTag() != ObjectTag::PLAYER) continue;

		const auto target = object->GetTransform()->GetPosition();
		auto position = owner->GetTransform()->GetPosition();
		const float dx = target.x - position.x;
		const float dy = target.y - position.y;
		const float distance = std::sqrt(dx * dx + dy * dy);
		if (distance <= 0.001f) return;

		const float step = std::min(distance, std::fmax(0.f, moveSpeed) * deltaTime);
		position.x += dx / distance * step;
		position.y += dy / distance * step;
		owner->GetTransform()->SetPosition(position);
		return;
	}
}
