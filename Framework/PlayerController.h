#pragma once
#include "IComponent.h"
#include "MonoBehavior.h"

class Rigidbody2DComponent;

class PlayerController : public MonoBehavior
{
public:
	void Start() override;
	void Update(float deltaTime) override;
	~PlayerController() override = default;
	void Awake() override;
	void OnDestroy() override;

	void HandleInput(float deltaTime);
	void HandleAnimation(float deltaTime);
	void HandleMovement(float deltaTime);
	void HandleBlockSpawn();

	void OnCollisionEnter2D(const GameObject& other) override;

	bool GetHasSpawned() const;

private:
	float accX = 10;
	float currSpeedX = 0;
	float maxSpeedX = 1000;
	float posX = 0, posY = 0;
	float frictionX = 800;
	float currSpeedY = 0.0f;
	float accY = 10;
	float maxSpeedY = 100;
	float frictionY = 8;
	float bounceBackDsit = 300.f;

	float moveForce = 1000.f;

	Rigidbody2DComponent* rb = nullptr;

	float inputX = 0;
	float inputY = 0;

	bool hasSpawned = false;
};

