#pragma once
#include "MonoBehavior.h"

struct EnemyStatus
{	
	int hp = 30;
	int attackPower = 10;
	float moveSpeed = 60.f;
	int dropExperience = 5;
};

class EnemyController : public MonoBehavior
{
public:
	void SetStatus(
		int hp,
		int attackPower,
		float moveSpeed,
		int dropExperience);

	const EnemyStatus& GetStatus() const
	{
		return status;
	}

	int GetHP() const
	{
		return status.hp;
	}
	int GetAttackPower() const 
	{ 
		return status.attackPower; 
	}
	bool IsDead() const 
	{
		return dead;
	}

	bool TakeDamage(int damage,const DirectX::XMFLOAT3& attackPosition);

	void SetExternalControl(bool enabled);
	bool IsExternallyControlled() const { return externalControl; }

	void LateUpdate(float deltaTime) override;

	float knockbackSpeed = 240.f;
	float knockbackDuration = 0.2f;

	// 振り回し攻撃の判定を有効にするか
	void SetAttackActive(bool enabled)
	{
		attackActive = enabled;
	}

	bool IsAttackActive() const
	{
		return attackActive && !dead;
	}

	// 接触し続けたときの被弾の間隔
	float damageCooldown = 0.3f;

	void OnCollisionEnter2D(const GameObject& other) override;
	void OnCollisionStay2D(const GameObject& other) override;

private:
	void FinishDeath();

	EnemyStatus status;

	bool dead = false;
	bool deathHandled = false;
	bool externalControl = false;

	void ReceiveCollisionDamage(const GameObject& other);

	bool attackActive = false;
	float damageCooldownTimer = 0.f;

	float knockbackTimer = 0.f;
	DirectX::XMFLOAT2 knockbackDirection = { 0.f, 0.f };
	DirectX::XMFLOAT3 deathPosition = { 0.f, 0.f, 1.f };
};
