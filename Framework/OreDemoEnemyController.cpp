#include "OreDemoEnemyController.h"

#include <algorithm>
#include <cmath>

#include "GameObject.h"
#include "HammerContactComponent.h"
#include "Rigidbody2DComponent.h"
#include "Scene.h"

OreDemoEnemyController::OreDemoEnemyController(int inMaxHp)
	: maxHp(inMaxHp > 0 ? inMaxHp : 1), hp(maxHp)
{
}

bool OreDemoEnemyController::TakeDamage(int damage, const DirectX::XMFLOAT3&)
{
	if (damage <= 0 || IsDead()) return false;
	// Clamp without subtraction overflow, including INT_MAX contact damage.
	hp = damage >= hp ? 0 : hp - damage;
	if (IsDead()) QueueDeath();
	return true;
}

void OreDemoEnemyController::QueueDeath()
{
	if (deathPending || deathHandled) return;
	deathPending = true;
	if (!owner) return;

	if (auto* hammer = owner->GetComponent<HammerContactComponent>())
		hammer->SetHammerState(false);
	if (auto* collider = owner->GetComponent<BoxCollider2D>())
		collider->SetColliderActive(false);
	if (auto* animator = owner->GetComponent<AnimatorComponent>())
		animator->SetEnabled(false);
	if (auto* body = owner->GetComponent<Rigidbody2DComponent>())
	{
		body->SetVelocity({ 0.f, 0.f });
		body->SetAngularVel(0.f);
	}
	// Keep this script enabled: LateUpdate queues removal after physics has
	// finished dispatching contacts to the still-live enemy object.
}

void OreDemoEnemyController::FinishDeath()
{
	if (!deathPending || deathHandled || !owner) return;
	auto* scene = owner->GetContext().gameScene;
	if (!scene) return;
	deathPending = false;
	deathHandled = true;
	SetEnabled(false);
	scene->Destroy(owner);
}

void OreDemoEnemyController::OnDestroy()
{
	deathPending = false;
	deathHandled = true;
	SetEnabled(false);
}

void OreDemoEnemyController::LateUpdate(float deltaTime)
{
	if (deathPending)
	{
		FinishDeath();
		return;
	}
	if (IsDead() || !owner) return;
	const auto* hammer = owner->GetComponent<HammerContactComponent>();
	if (hammer && hammer->IsHammerState()) return;
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
		// Movement is controlled through the transform, so explicitly wake the
		// zero-gravity body to keep its trigger participating in contacts.
		if (step > 0.f)
			if (auto* body = owner->GetComponent<Rigidbody2DComponent>()) body->Wake();
		return;
	}
}
