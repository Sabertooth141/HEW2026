#include "PlayerController.h"

#include "GameObject.h"
#include "Keyboard.h"
#include "Rigidbody2DComponent.h"
#include "WindowSettings.h"

void PlayerController::Start()
{
	MonoBehavior::Start();
	posX = owner->GetTransform()->GetPosition().x;
	posY = owner->GetTransform()->GetPosition().y;

	inputX = 1.f;
}

void PlayerController::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);

	HandleInput(deltaTime);
	HandleMovement(deltaTime);
	HandleAnimation(deltaTime);
	HandleBlockSpawn();
}

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
}

void PlayerController::OnDestroy()
{
	MonoBehavior::OnDestroy();
}

void PlayerController::HandleInput(float deltaTime)
{
	//inputX = 0;
	//inputY = 0;
	//if (keyboard->KeyIsPressed('D'))
	//{
	//	owner->GetComponent<AnimatorComponent>()->SetFlipX(false);
	//	inputX += 1.f;
	//}
	//if (keyboard->KeyIsPressed('A'))
	//{
	//	owner->GetComponent<AnimatorComponent>()->SetFlipX(true);
	//	inputX -= 1.f;
	//}

	//if (keyboard->KeyIsPressed('W'))
	//{
	//	inputY += 1.f;
	//}
	//if (keyboard->KeyIsPressed('S'))
	//{
	//	inputY -= 1.f;
	//}

	//
}

void PlayerController::HandleAnimation(float deltaTime)
{
	if (inputX > 0.f)
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(false);
	}
	else if (inputX < 0.f)
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(true);
	}

	DirectX::XMFLOAT2 v = rb->GetVelocity();
	const bool moving = std::abs(v.x) > 1.f;
	if (!moving && owner->GetComponent<AnimatorComponent>()->GetCurrAnimName() != "CharIdle")
	{
		owner->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharIdle");
	}
	else if (moving && owner->GetComponent<AnimatorComponent>()->GetCurrAnimName() != "CharMove")
	{
		owner->GetComponent<AnimatorComponent>()->SetCurrAnimation("CharMove");
	}
}

void PlayerController::HandleMovement(float deltaTime)
{
	DirectX::XMFLOAT2 v = rb->GetVelocity();

	const float decel = frictionX * deltaTime;

	if (inputX == 0.f)
	{
		if (v.x > 0.f)
		{
			v.x = std::max(0.f, v.x - decel);
		}
		else if (v.x < 0.f)
		{
			v.x = std::min(0.f, v.x + decel);
		}
	}

	// clamp speed
	v.x = std::clamp(v.x, -maxSpeedX, maxSpeedX);
	v.y = std::clamp(v.y, -maxSpeedX, maxSpeedX);
	rb->SetVelocity(v);

	const float halfSprite = owner->GetTransform()->GetScale().x * 0.5f;
	DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
	if (pos.x + halfSprite < -WIN_WIDTH / 2.f + bounceBackDsit)
	{
		inputX = 1.f;
	}
	else if (pos.x - halfSprite > WIN_WIDTH / 2.f - bounceBackDsit)
	{
		inputX = -1.f;
	}

	rb->AddForce({ moveForce * inputX, moveForce * inputY });
	owner->GetTransform()->SetPosition(pos);
}

void PlayerController::HandleBlockSpawn()
{
	if (keyboard->KeyIsTriggered(' '))
	{
		if (hasSpawned)
		{
			return;
		}
		hasSpawned = true;
		Instantiate("block", owner->GetTransform()->GetPosition());
	}
	if (keyboard->KeyIsReleased(' '))
	{
		hasSpawned = false;
	}
}

void PlayerController::OnCollisionEnter2D(const GameObject& other)
{
	MonoBehavior::OnCollisionEnter2D(other);
}

bool PlayerController::GetHasSpawned() const
{
	return hasSpawned;
}
