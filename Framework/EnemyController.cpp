#include "EnemyController.h"

#include <algorithm>
#include <cmath>

#include "GameObject.h"
#include "Scene.h"

void EnemyController::SetStatus(int hp, int attackPower, float moveSpeed)
{
	status.hp = std::max(1, hp);
	status.attackPower = std::max(0, attackPower);
	status.moveSpeed = std::fmax(0.f, moveSpeed);
}

void EnemyController::LateUpdate(float deltaTime)
{
	// 削除は衝突判定の途中ではなく、ここで予約する
	if (dead)
	{
		DestroySelf();
		return;
	}

	if (deltaTime <= 0.f || !std::isfinite(deltaTime))
	{
		return;
	}

	damageCooldownTimer = std::max(0.f, damageCooldownTimer - deltaTime);

	// ノックバック中はプレイヤーを追いかけない
	if (knockbackTimer > 0.f)
	{
		auto* transform = owner->GetTransform();
		auto position = transform->GetPosition();

		const float distance = knockbackSpeed * std::min(deltaTime, knockbackTimer);
		position.x += knockbackDirection.x * distance;
		position.y += knockbackDirection.y * distance;
		transform->SetPosition(position);

		knockbackTimer = std::max(0.f, knockbackTimer - deltaTime);
		return;
	}

	ChasePlayer(deltaTime);
}

void EnemyController::ChasePlayer(float deltaTime)
{
	auto* scene = owner->GetContext().gameScene;
	if (scene == nullptr)
	{
		return;
	}

	for (const auto& object : scene->GetObjects())
	{
		if (object->GetTag() != ObjectTag::PLAYER)
		{
			continue;
		}

		auto* transform = owner->GetTransform();
		auto position = transform->GetPosition();
		const auto target = object->GetTransform()->GetPosition();

		const float dx = target.x - position.x;
		const float dy = target.y - position.y;
		const float distance = std::sqrt(dx * dx + dy * dy);
		if (distance <= 0.001f)
		{
			return;
		}

		const float step = std::min(distance, status.moveSpeed * deltaTime);
		position.x += dx / distance * step;
		position.y += dy / distance * step;
		transform->SetPosition(position);
		return;
	}
}

bool EnemyController::TakeDamage(int damage, const DirectX::XMFLOAT3& attackPosition)
{
	if (dead || damage <= 0 || damageCooldownTimer > 0.f)
	{
		return false;
	}

	damageCooldownTimer = damageCooldown;
	status.hp = std::max(0, status.hp - damage);

	if (status.hp == 0)
	{
		dead = true;
		return true;
	}

	// 攻撃を受けた位置から離れる方向へノックバック
	const auto position = owner->GetTransform()->GetPosition();
	const float dx = position.x - attackPosition.x;
	const float dy = position.y - attackPosition.y;
	const float distance = std::sqrt(dx * dx + dy * dy);

	knockbackDirection = distance > 0.001f
		? DirectX::XMFLOAT2{ dx / distance, dy / distance }
		: DirectX::XMFLOAT2{ 1.f, 0.f };
	knockbackTimer = knockbackDuration;

	return true;
}
