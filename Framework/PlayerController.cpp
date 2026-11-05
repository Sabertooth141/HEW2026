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
#include "BoxCollider2D.h"
#include "RetryTransitionComponent.h"
#include "CameraController.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	hp = maxHP;
	playTimer = owner->GetContext().globalContext.playTime;

	//最後に触れたチェックポイントを取得
	lastCheckpointPosition = owner->GetTransform()->GetPosition();

	auto* playerCollider = owner->GetComponent<BoxCollider2D>();
	if (playerCollider != nullptr)
	{
		auto aabb = playerCollider->GetWorldAABB();//プレイヤーの当たり判定の四隅の位置

		previousPlayerBottom = aabb.min.y;//プレイヤーの足元のY座標
		previousPlayerLeft = aabb.min.x;//プレイヤーの左端のX座標
		previousPlayerRight = aabb.max.x;//プレイヤーの右端のX座標
	}
	//加速
	speedBoost = owner->GetComponent<SpeedBoostComponent>();
}

void PlayerController::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);

	// 死亡中はSpaceキーを押すまで待機
	if (isDead)
	{
		if (keyboard->KeyIsTriggered(' '))
		{
			Respawn();
		}

		return;
	}
	//プレイヤーの足元を記録
	auto* playerCollider = owner->GetComponent<BoxCollider2D>();
	if (playerCollider != nullptr)
	{
		auto aabb = playerCollider->GetWorldAABB();

		previousPlayerBottom = aabb.min.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}
	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - deltaTime);
	auto* retryTransition =
		owner->GetComponent<RetryTransitionComponent>();

	//トランジションが終わったタイミングで復帰
	if (retryTransition != nullptr && retryTransition->ConsumeRespawnRequest())
	{
		RespawnAtCheckPoint();
	}

	//死亡中はspace入力を待つ
	if (IsDead())
	{
		if (speedBoost != nullptr)
		{
			speedBoost->ResetBoost();
		}

		if (rb != nullptr)
		{
			rb->SetVelocity({ 0.0f,0.0f });
		}

		if (retryTransition != nullptr &&
			!retryTransition->IsRunning() &&
			keyboard != nullptr &&
			keyboard->KeyIsTriggered(' '))
		{
			retryTransition->Begin();
		}

		HandleHUD();
		return;
	}

	//復帰処理でトランジション（暗転）中はプレイヤーの操作を受け付けない
	if (retryTransition != nullptr && retryTransition->IsRunning())
	{
		if (rb != nullptr)
		{
			rb->SetVelocity({ 0.0f,0.0f });
		}

		HandleHUD();
		return;
	}




	HandleInput();
	HandleMovement();
	HandleAnimation();
	HandleHUD();
	HandleJump();

	playTimer += deltaTime;
	
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

const float currentMoveSpeed = speedBoost != nullptr
    ? speedBoost->GetMoveSpeed(moveSpeed)
    : moveSpeed;

rb->SetVelocity({
    dirX * currentMoveSpeed,
    rb->GetVelocity().y
});
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

	// 針に上から落ちてきた場合だけ死亡
	if (other.GetTag() == ObjectTag::SPIKE)
	{
		auto* playerCollider = owner->GetComponent<BoxCollider2D>();
		auto* spikeCollider = other.GetComponent<BoxCollider2D>();

		if (playerCollider != nullptr && spikeCollider != nullptr && rb != nullptr)
		{
			//プレイヤーとトゲの現在の当たり判定の範囲
			const auto spikeAABB = spikeCollider->GetWorldAABB();
			const auto playerAABB = playerCollider->GetWorldAABB();

			const float spikeTop = spikeAABB.max.y; // 針の一番上のY座標を取得

			const float playerBottom = playerAABB.min.y;//プレイヤーの足元のY座標の取得

			const bool horizontalOverlap =
				previousPlayerRight > spikeAABB.min.x &&
				previousPlayerLeft < spikeAABB.max.x;
			// 上から落下して接触したか
			const bool landedOnSpike =
				rb->GetVelocity().y < 0.f &&//下方向の落下
				previousPlayerBottom >= spikeTop - 1.f &&//前フレームでは針より上にいたか
				playerBottom < spikeTop;

			if (horizontalOverlap && landedOnSpike)
			{
				isDead = true;
				rb->SetVelocity({ 0.f, 0.f });
			}
		}
		return;
	}

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
	const GameObject* checkpoint)
{
	lastCheckpointPosition = position;
	
	if (speedBoost != nullptr)
	{
		speedBoost->TryAddStack(checkpoint);
	}
}

void PlayerController::RespawnAtCheckPoint()
{
	owner->GetTransform()->SetPosition(lastCheckpointPosition);

	auto* cameraConroller = owner->GetComponent<CameraController>();

	if (cameraConroller != nullptr)
	{
		cameraConroller->SnapToTarget();
	}

	if (rb != nullptr)
	{
		rb->SetVelocity({ 0.0f,0.0f });
	}

	hp = maxHP;
	invulnerabilityTimer = invulnerabilityDuration;
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


void PlayerController::Respawn()
{
	owner->GetTransform()->SetPosition(lastCheckpointPosition);

	if (rb != nullptr)
	{
		rb->SetVelocity({ 0.f, 0.f });
		rb->SetAngularVel(0.f);
		rb->Wake();
	}
	isGrounded = false;
	isDead = false;
	auto* playerCollider = owner->GetComponent<BoxCollider2D>();
	if (playerCollider != nullptr)
	{
		auto aabb = playerCollider->GetWorldAABB();

		previousPlayerBottom = aabb.min.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}
}