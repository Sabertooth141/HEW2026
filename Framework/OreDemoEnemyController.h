#pragma once
#include "MonoBehavior.h"

// Optional demo actor. It does not replace or extend the game's EnemyController.
class OreDemoEnemyController : public MonoBehavior
{
public:
	explicit OreDemoEnemyController(int maxHp = 200);

	int GetHP() const { return hp; }
	int GetMaxHP() const { return maxHp; }
	bool IsDead() const { return hp == 0; }
	bool TakeDamage(int damage, const DirectX::XMFLOAT3& attackPosition = {});

	float moveSpeed = 60.f;
	void LateUpdate(float deltaTime) override;
	void OnDestroy() override;

private:
	void QueueDeath();
	void FinishDeath();

	int maxHp;
	int hp;
	bool deathPending = false;
	bool deathHandled = false;
};
