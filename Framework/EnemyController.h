#pragma once
#include "MonoBehavior.h"

class EnemyController : public MonoBehavior
{
public:
	float moveSpeed = 60.f;
	void LateUpdate(float deltaTime) override;
};
