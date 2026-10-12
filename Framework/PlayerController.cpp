#include "PlayerController.h"

#include "GameObject.h"
#include "Keyboard.h"
#include "Rigidbody2DComponent.h"
#include "WindowSettings.h"
#include "Mouse.h"
#include "Camera2D.h"
#include "EnemyController.h"

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

	if (invulnerabilityTimer > 0.0f)
	{
		invulnerabilityTimer -= deltaTime;

		if (invulnerabilityTimer < 0.0f)
		{
			invulnerabilityTimer = 0.0f;
		}
	}
	if (harpoonCooldown > 0.0f)
	{
		harpoonCooldown -= deltaTime;

		if (harpoonCooldown < 0.0f)
		{
			harpoonCooldown = 0.0f;
		}
	}

	HandleInput(deltaTime);
	HandleMovement(deltaTime);
	HandleAnimation(deltaTime);
	HandleBlockSpawn();
}

PlayerStatus& PlayerController::GetStatus()
{
	return status;
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
	inputX = 0;
	inputY = 0;
	if (keyboard->KeyIsPressed('D'))
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(false);
		inputX += 1.f;
	}
	if (keyboard->KeyIsPressed('A'))
	{
		owner->GetComponent<AnimatorComponent>()->SetFlipX(true);
		inputX -= 1.f;
	}

	if (keyboard->KeyIsPressed('W'))
	{
		inputY += 1.f;
	}
	if (keyboard->KeyIsPressed('S'))
	{
		inputY -= 1.f;
	}
	// 確認用
	if (inputY != 0.f)
	{
		OutputDebugStringA("inputY is working!\n");
	}


//	if (keyboard->KeyIsTriggered('L'))
	//{
		//if (!levelUpSelecting)
		//{
			//AddExperience(100.0f);
		//}
	//}

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
	const float decelY = frictionY * deltaTime;

	if (inputX == 0.f)
	{
		v.x = 0.f;
	}


	if (inputY == 0.f)
	{
		v.y = 0.f;
	}

	// clamp speed
	float moveSpeed = status.GetMoveSpeed();

	v.x = std::clamp(v.x, -moveSpeed, moveSpeed);
	v.y = std::clamp(v.y, -moveSpeed, moveSpeed);
	rb->SetVelocity(v);

	//const float halfSprite = owner->GetTransform()->GetScale().x * 0.5f;
	//DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
	//if (pos.x + halfSprite < -WIN_WIDTH / 2.f + bounceBackDsit)
	//{
	//	inputX = 1.f;
	//}
	//else if (pos.x - halfSprite > WIN_WIDTH / 2.f - bounceBackDsit)
//	{
	//	inputX = -1.f;
	//}

	v.x = inputX * moveSpeed;
	v.y = inputY * moveSpeed;

	rb->SetVelocity(v);
	//owner->GetTransform()->SetPosition(pos);
}

//void PlayerController::HandleBlockSpawn()
//{
	//if (keyboard->KeyIsTriggered(' '))
	//{
//		if (hasSpawned)
	//	{
	//		return;
	//	}
	//	hasSpawned = true;
	//	Instantiate("block", owner->GetTransform()->GetPosition());
	//}
	//if (keyboard->KeyIsReleased(' '))
	//{
	//	hasSpawned = false;
	//}
//}

void PlayerController::IgnoreMouseUntilRelease()
{
	ignoreMouseUntilRelease = true;
}

void PlayerController::HandleBlockSpawn()
{
	if (ignoreMouseUntilRelease)
	{
		if (!mouse->LeftPressed())
		{
			ignoreMouseUntilRelease = false;
		}

		return;
	}
	if (levelUpSelecting)
	{
		return;
	}
	int mouseX = mouse->GetPosX();
	int mouseY = mouse->GetPosY();

	if (mouse->LeftPressed())
	{
		if (hasSpawned)
		{
			return;
		}

		if (harpoonCooldown > 0.0f)
		{
			return;
		}

		hasSpawned = true;

		auto* harpoon =
			Instantiate("harpoon", owner->GetTransform()->GetPosition());

		auto* harpoonRb =
			harpoon->GetComponent<Rigidbody2DComponent>();

		// 以下は今までのマウス方向処理
		DirectX::XMFLOAT3 playerPos =
			owner->GetTransform()->GetPosition();
		auto cameraPos = owner->GetContext().camera.GetPosition();

		float targetX =
			static_cast<float>(mouseX) - WIN_WIDTH / 2.f + cameraPos.x;

		float targetY =
			WIN_HEIGHT / 2.f - static_cast<float>(mouseY) + cameraPos.y;

		float directionX = targetX - playerPos.x;
		float directionY = targetY - playerPos.y;

		float length =
			std::sqrt(directionX * directionX +
				directionY * directionY);

		if (length > 0.f)
		{
			directionX /= length;
			directionY /= length;
		}

		harpoonRb->SetVelocity({
			directionX * 500.f,
			directionY * 500.f
			});
		harpoonCooldown = status.GetHarpoonInterval();
	}



	while (auto event = mouse->Read())
	{
		if (event->GetType() == Mouse::Event::Type::LReleased)
		{
			hasSpawned = false;
		}
	}
}



void PlayerController::OnCollisionEnter2D(const GameObject& other)
{
	MonoBehavior::OnCollisionEnter2D(other);

	// 無敵時間中はダメージを受けない
	if (invulnerabilityTimer > 0.0f)
	{
		return;
	}

	// 敵以外との接触ではダメージを受けない
	if (other.GetTag() != ObjectTag::ENEMY)
	{
		return;
	}

	// 敵の攻撃力分、プレイヤーのHPを減らす
	auto* enemy = other.GetComponent<EnemyController>();

	if (enemy == nullptr)
	{
		return;
	}

	status.TakeDamage(
		static_cast<float>(enemy->GetAttackPower()));

	// 無敵時間を開始する
	invulnerabilityTimer = invulnerabilityDuration;
}

bool PlayerController::GetHasSpawned() const
{
	return hasSpawned;
}


bool PlayerController::IsLevelUpSelecting() const
{
	return levelUpSelecting;
}

const std::vector<PlayerStatus::StatType>& PlayerController::GetLevelUpChoices() const
{
	return levelUpChoices;
}

void PlayerController::SelectLevelUpChoice(size_t index)
{
	if (!levelUpSelecting)
	{
		return;
	}

	if (index >= levelUpChoices.size())
	{
		return;
	}

	status.ApplyLevelUp(levelUpChoices[index]);
	status.Heal(status.GetMaxHP() - status.GetHP());

	levelUpSelecting = false;

	OutputDebugStringA("Level Up choice selected.\n");
}

void PlayerController::AddExperience(float amount)
{
	int oldLevel = status.GetLevel();

	status.AddExperience(amount);

	if (status.GetLevel() > oldLevel && !levelUpSelecting)
	{
		levelUpChoices = status.GetLevelUpChoices();

		if (!levelUpChoices.empty())
		{
			levelUpSelecting = true;
			OutputDebugStringA("Level Up! Select a stat.\n");
		}
	}
}