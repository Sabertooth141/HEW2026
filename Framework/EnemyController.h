#pragma once
#include "MonoBehavior.h"

class EnemyController : public MonoBehavior
{
public:
	float moveSpeed = 60.f;
	void LateUpdate(float deltaTime) override;
	virtual void TakeDamage(float damage);
	float hp = 3.0f;
};
