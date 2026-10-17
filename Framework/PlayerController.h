#pragma once
#include "MonoBehavior.h"

class Rigidbody2DComponent;
class AnimatorComponent;

// サンプル用の基本的なプレイヤー操作
// WASDで上下左右に移動し、敵に触れるとHPが減る
class PlayerController : public MonoBehavior
{
public:
	~PlayerController() override = default;

	void Awake() override;
	void Update(float deltaTime) override;

	void OnCollisionEnter2D(const GameObject& other) override;

	float GetHP() const { return hp; }
	float GetMaxHP() const { return maxHP; }
	bool IsDead() const { return hp <= 0.f; }

	float moveSpeed = 300.f;
	float maxHP = 100.f;
	float invulnerabilityDuration = 1.f;

private:
	void HandleInput();
	void HandleMovement();
	void HandleAnimation();
	void HandleHUD();

	Rigidbody2DComponent* rb = nullptr;
	AnimatorComponent* anim = nullptr;

	float inputX = 0.f;
	float inputY = 0.f;

	float hp = 100.f;
	float invulnerabilityTimer = 0.f;

	float playTimer = 0.f;
};
