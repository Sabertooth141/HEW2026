#pragma once
#include <DirectXMath.h>

#include "MonoBehavior.h"

struct EnemyStatus
{
	int hp = 30;
	int attackPower = 10;
	float moveSpeed = 60.f;
};

// サンプル用の基本的な敵
// プレイヤーを追いかけ、ダメージを受けるとノックバックし、HP0で消える
class EnemyController : public MonoBehavior
{
public:
	void SetStatus(int hp, int attackPower, float moveSpeed);

	const EnemyStatus& GetStatus() const { return status; }
	int GetHP() const { return status.hp; }
	int GetAttackPower() const { return status.attackPower; }
	bool IsDead() const { return dead; }

	// ダメージを受けたらtrueを返す（無敵時間中・死亡済みはfalse）
	bool TakeDamage(int damage, const DirectX::XMFLOAT3& attackPosition);

	void LateUpdate(float deltaTime) override;

	float knockbackSpeed = 240.f;
	float knockbackDuration = 0.2f;

	// 連続で被弾しないための間隔
	float damageCooldown = 0.3f;

private:
	void ChasePlayer(float deltaTime);

	EnemyStatus status;

	bool dead = false;
	float damageCooldownTimer = 0.f;

	float knockbackTimer = 0.f;
	DirectX::XMFLOAT2 knockbackDirection = { 0.f, 0.f };
};
