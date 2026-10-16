#include "PlayerController.h"

#include <algorithm>
#include <cmath>

#include "AnimatorComponent.h"
#include "EnemyController.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Rigidbody2DComponent.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	hp = maxHP;
}

void PlayerController::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);

	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - deltaTime);

	HandleInput();
	HandleMovement();
	HandleAnimation();
}

void PlayerController::HandleInput()
{
	inputX = 0.f;
	inputY = 0.f;

	if (keyboard->KeyIsPressed('D')) inputX += 1.f;
	if (keyboard->KeyIsPressed('A')) inputX -= 1.f;
	if (keyboard->KeyIsPressed('W')) inputY += 1.f;
	if (keyboard->KeyIsPressed('S')) inputY -= 1.f;
}

void PlayerController::HandleMovement()
{
	if (rb == nullptr)
	{
		return;
	}

	float dirX = inputX;
	float dirY = inputY;

	// 斜め移動が速くならないように正規化する
	const float length = std::sqrt(dirX * dirX + dirY * dirY);
	if (length > 0.f)
	{
		dirX /= length;
		dirY /= length;
	}

	rb->SetVelocity({ dirX * moveSpeed, dirY * moveSpeed });
}

void PlayerController::HandleAnimation()
{
	if (anim == nullptr)
	{
		return;
	}

	if (inputX > 0.f)
	{
		anim->SetFlipX(false);
	}
	else if (inputX < 0.f)
	{
		anim->SetFlipX(true);
	}

	const bool moving = inputX != 0.f || inputY != 0.f;
	const char* next = moving ? "CharMove" : "CharIdle";

	if (anim->GetCurrAnimName() != next)
	{
		anim->SetCurrAnimation(next);
	}
}

void PlayerController::OnCollisionEnter2D(const GameObject& other)
{
	MonoBehavior::OnCollisionEnter2D(other);

	// 無敵時間中、または敵以外との接触ではダメージを受けない
	if (invulnerabilityTimer > 0.f || other.GetTag() != ObjectTag::ENEMY)
	{
		return;
	}

	const auto* enemy = other.GetComponent<EnemyController>();
	if (enemy == nullptr || enemy->IsDead())
	{
		return;
	}

	hp = std::max(0.f, hp - static_cast<float>(enemy->GetAttackPower()));
	invulnerabilityTimer = invulnerabilityDuration;
}
