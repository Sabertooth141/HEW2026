#pragma once

#include "MonoBehavior.h"

class HarpoonController :public MonoBehavior
{
public:
	int GetHammerDamage() const;
	void Awake() override;
	void Update(float deltaTime) override;
	void OnCollisionEnter2D(const GameObject& other) override;
	bool IsStuck() const { return isStuck; }
	GameObject* GetStuckObject() const { return stuckObject; }
	bool IsHammerObject(const GameObject* object) const
	{
		return isStuck && stuckObject == object;
	}

private:
	bool isStuck = false;
	GameObject* stuckObject = nullptr;
	bool isOreTarget = false;
	float swingAngle = 0.0f;
	float swingRadius = 0.0f;
};


