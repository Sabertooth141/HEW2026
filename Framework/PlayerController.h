#pragma once
#include "IComponent.h"
#include "MonoBehavior.h"
#include "PlayerStatus.h"

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
	PlayerStatus& GetStatus();
	bool IsLevelUpSelecting() const;

	const std::vector<PlayerStatus::StatType>& GetLevelUpChoices() const;

	void SelectLevelUpChoice(size_t index);
	void IgnoreMouseUntilRelease();

	void AddExperience(float amount);

private:
	bool ignoreMouseUntilRelease = false;
	float harpoonCooldown = 0.0f;

	std::vector<PlayerStatus::StatType> levelUpChoices;

	PlayerStatus status;

	float accX = 10;
	float currSpeedX = 0;
	float maxSpeedX = 500;
	float posX = 0, posY = 0;
	float frictionX = 2000;
	float currSpeedY = 0.0f;
	float accY = 10;
	float maxSpeedY = 500;
	float frictionY = 2000;
	float bounceBackDsit = 300.f;
	float invulnerabilityTimer = 0.0f;
	float invulnerabilityDuration = 1.0f;

	float moveForce = 1000.f;

	

	Rigidbody2DComponent* rb = nullptr;

	float inputX = 0;
	float inputY = 0;

	bool hasSpawned = false;
	bool levelUpSelecting = false;
};

