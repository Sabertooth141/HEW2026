#include "PlayerController.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "AnimatorComponent.h"
#include "EnemyController.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Rigidbody2DComponent.h"
#include "SceneManager.h"
#include "TextLabel.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	hp = maxHP;
	playTimer = owner->GetContext().globalContext.playTime;

	lastCheckpointPosition = owner->GetTransform()->GetPosition();
}

void PlayerController::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);

	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - deltaTime);

	HandleInput();
	HandleMovement();
	HandleAnimation();
	HandleHUD();
	HandleJump();

	playTimer += deltaTime;
	if (IsDead())
	{
		owner->GetContext().globalContext.playTime = playTimer;
		owner->GetContext().sceneManager->RequestChange("Result");
		DestroySelf();
	}
}

void PlayerController::HandleInput()
{
	inputX = 0.f;
	inputY = 0.f;

	if (keyboard->KeyIsPressed('D')) inputX += 1.f;
	if (keyboard->KeyIsPressed('A')) inputX -= 1.f;
	
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

	rb->SetVelocity({ dirX * moveSpeed, rb->GetVelocity().y });
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

void PlayerController::HandleHUD()
{
	std::string text = std::format("Player HP: {:.1f}", GetHP());

	owner->GetComponent<TextLabel>()->text = text;
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

	// 地面に接触したら接地状態にする
	if (other.GetTag() == ObjectTag::GROUND)
	{
		if (rb != nullptr && rb->GetVelocity().y <= 0.f)
		{
			isGrounded = true;
		}
	}
}


//プレイヤーが最後に触ったチェックポイント
void PlayerController::SetCheckpoint(
	const DirectX::XMFLOAT3& position,
	bool addStack)
{
	lastCheckpointPosition = position;
}

void PlayerController::HandleJump()
{
	if (rb == nullptr)
	{
		return;
	}

	// 地面にいるときだけジャンプできる
	if (isGrounded && keyboard->KeyIsTriggered(' '))
	{
		DirectX::XMFLOAT2 velocity = rb->GetVelocity();
		velocity.y = jumpPower;

		rb->SetVelocity(velocity);
		isGrounded = false;
	}
}

void PlayerController::OnCollisionStay2D(const GameObject& other)
{
	if (other.GetTag() == ObjectTag::GROUND &&
		rb != nullptr &&
		rb->GetVelocity().y <= 0.f)
	{
		isGrounded = true;
	}
}

void PlayerController::OnCollisionExit2D(const GameObject& other)
{
	if (other.GetTag() == ObjectTag::GROUND)
	{
		isGrounded = false;
	}
}

