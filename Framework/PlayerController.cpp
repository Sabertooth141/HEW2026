#include "PlayerController.h"

#include <algorithm>
#include <cmath>
#include <format>

#include "AnimatorComponent.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Rigidbody2DComponent.h"
#include "SceneManager.h"
#include "TextLabel.h"
#include "BoxCollider2D.h"
#include "Collider2D.h"
#include "RetryTransitionComponent.h"
#include "CameraController.h"


void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	if (rb != nullptr)
	{
		normalGravity = rb->GetGravity();
	}
	hp = maxHP;
	playTimer = owner->GetContext().globalContext.playTime;

	//最後に触れたチェックポイントを取得
	lastCheckpointPosition = owner->GetTransform()->GetPosition();

	auto* playerCollider = owner->GetComponent<BoxCollider2D>();//プレイヤーの当たり判定の取得
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
		owner->GetComponent<RetryTransitionComponent>();//リトライ演出を管理するコンポーネントを取得

	//トランジションが終わったタイミングで復帰
	if (retryTransition != nullptr && retryTransition->ConsumeRespawnRequest())
	{
		RespawnAtCheckPoint();
	}

	//死亡中はspace入力を待つ
	if (IsDead())
	{
		if (speedBoost != nullptr)//加速リセット
		{
			speedBoost->ResetBoost();
		}

		if (rb != nullptr)//プレイヤーが動かないように
		{
			rb->SetVelocity({ 0.0f,0.0f });
		}

		if (retryTransition != nullptr &&//復活
			!retryTransition->IsRunning() &&
			keyboard != nullptr &&
			keyboard->KeyIsTriggered(' '))
		{
			retryTransition->Begin();
		}

		return;
	}

	//復帰処理でトランジション（暗転）中はプレイヤーの操作を受け付けない
	if (retryTransition != nullptr && retryTransition->IsRunning())
	{
		if (rb != nullptr)
		{
			rb->SetVelocity({ 0.0f,0.0f });
		}

		return;
	}




	HandleInput();//キー入力取得
	HandleMovement(deltaTime);//地面や壁の状態で移動方法を決める
	HandleAnimation();
	HandleJump();

	playTimer += deltaTime;
	
}

void PlayerController::HandleInput()//キー入力
{
	inputX = 0.f;
	inputY = 0.f;

	if (keyboard->KeyIsPressed('D')) inputX += 1.f;
	if (keyboard->KeyIsPressed('A')) inputX -= 1.f;
	
}

void PlayerController::HandleMovement(float deltaTime)
{
	if (rb == nullptr)
	{
		return;
	}

	// 天井に接触しているか確認
	bool isTouchingCeiling = false;
	bool isTouchingWall = false;//壁との接触

	DirectX::XMFLOAT2 wallNormal = { 0.f, 0.f };//壁の法線

	auto* playerCollider = owner->GetComponent<BoxCollider2D>();

	if (playerCollider == nullptr)
	{
		return;
	}

	const AABB playerAABB = playerCollider->GetWorldAABB();

	for (const auto& contact : groundContacts)//接触中の地形を調べる
	{
		const DirectX::XMFLOAT2 normal = contact.second;//接触面の法線

		if (normal.y < -0.5f)//天井接触
		{
			isTouchingCeiling = true;
		}

		if (std::abs(normal.x) > 0.5f)//壁に接しているか
		{
			isTouchingWall = true;
			wallNormal = normal;

			const GameObject* wall = contact.first;//壁の上橋到達で、足場でいどう
			const auto* wallCollider =
				wall->GetComponent<BoxCollider2D>();

			if (wallCollider != nullptr && inputX < 0.f)
			{
				const AABB wallAABB = wallCollider->GetWorldAABB();

				// プレイヤーの足元が壁の上端に到達した
				if (playerAABB.min.y >= wallAABB.max.y - 2.f)
				{
					isWallTopTransitioning = true;
				}
			}
		}

		// 上側の足場に乗ったか確認
		if (normal.y > 0.5f)
		{
			const GameObject* ground = contact.first;
			const auto* groundCollider =
				ground->GetComponent<BoxCollider2D>();

			if (groundCollider != nullptr)
			{
				const AABB groundAABB =
					groundCollider->GetWorldAABB();

				const float distance =
					std::abs(playerAABB.min.y - groundAABB.max.y);//プレイヤーの足元と足場の上面との距離を計算

				if (distance < 8.f)
				{
					isWallTopTransitioning = false;
				}
			}
		}
	}

	// 天井に接触した瞬間だけノックバック
	if (isTouchingCeiling && !wasTouchingCeiling)
	{
		ceilingKnockbackTimer = 1.5f;//ノックバックの継続時間

		rb->SetVelocity({
			wallNormal.x * 250.f,
			0.f
			});//壁の法線方向にノックバック
	}

	wasTouchingCeiling = isTouchingCeiling;

	// ノックバック時間を更新
	if (ceilingKnockbackTimer > 0.f)
	{
		ceilingKnockbackTimer =
			std::max(0.f, ceilingKnockbackTimer - deltaTime);
	}

	// 壁から離れる方向に入力しているか
	const bool isMovingAwayFromWall =
		isTouchingWall &&
		inputX != 0.f &&
		inputX * wallNormal.x > 0.f;

	const bool useSurfaceMovement =//壁のぼりができる状態で地面か壁にぶつかると地形に沿ったいどうができる
		speedBoost != nullptr &&
		speedBoost->HasAbility(
			SpeedBoostComponent::BoostAbility::WallClimb) &&
		(
			isWallTopTransitioning ||
			(
				isTouchingGround &&
				inputX != 0.f &&
				!isTouchingCeiling &&
				!isMovingAwayFromWall
				)
			);

	if (useSurfaceMovement)//移動モードの切り替え
	{
		movementMode = MovementMode::Loop;
		rb->SetGravity(0.f);
	}
	else
	{
		movementMode = MovementMode::Normal;
		rb->SetGravity(normalGravity);
	}

	if (ceilingKnockbackTimer > 0.f)
	{
		// ノックバック中は通常移動で速度を上書きしない
		return;
	}

	switch (movementMode)
	{
	case MovementMode::Normal://通常移動
		HandleNormalMovement();
		break;

	case MovementMode::Loop://地形に沿った移動
		HandleLoopMovement(deltaTime);
		break;
	}
}

void PlayerController::HandleLoopMovement(float deltaTime)
{
	if (rb == nullptr)
	{
		return;
	}

	// 壁の上端から足場へ移動中
	if (isWallTopTransitioning)
	{
		const float currentSpeed =
			speedBoost->GetMoveSpeed(moveSpeed);//加速スタックで上がった今の移動速度

		rb->SetVelocity({
			inputX * currentSpeed,
			0.f
			});

		return;
	}

	if (!isTouchingGround)//地形にぶつかってないと地形に沿った移動はしない
	{
		return;
	}

	// 入力がない場合は移動を停止する
	if (inputX == 0.f)
	{
		rb->SetVelocity({ 0.f, 0.f });
		return;
	}

	// 現在の進行方向に適した地形の法線を選ぶ
	const DirectX::XMFLOAT2 normal = SelectSurfaceNormal();



	if (normal.x == 0.f && normal.y == 0.f)
	{
		return;
	}

	// 法線に対して直角な、地形に沿った移動方向
	DirectX::XMFLOAT2 tangent = {
		normal.y,
		-normal.x
	};

	// 左入力なら進行方向を反転
	if (inputX < 0.f)
	{
		tangent.x = -tangent.x;
		tangent.y = -tangent.y;
	}

	// 加速中の移動速度
	const float currentSpeed = speedBoost->GetMoveSpeed(moveSpeed);

	// 地形に沿って移動
	rb->SetVelocity({
		tangent.x * currentSpeed,
		tangent.y * currentSpeed
		});
}


DirectX::XMFLOAT2 PlayerController::SelectSurfaceNormal() const
{
	if (groundContacts.empty() || inputX == 0.f)//接触中の地形がないか左右の入力もない場合は終了
	{
		return { 0.f, 0.f };
	}

	auto* playerCollider = owner->GetComponent<BoxCollider2D>();

	if (playerCollider == nullptr)
	{
		return { 0.f, 0.f };
	}

	const AABB playerAABB = playerCollider->GetWorldAABB();//現在のプレイヤーの当たり判定の範囲を取得

	// 進行方向にある壁を探す
	for (const auto& contact : groundContacts)
	{
		const GameObject* ground = contact.first;// 接触している地形のオブジェクトを取得
		const DirectX::XMFLOAT2 normal = contact.second;// 接触している地形の法線を取得

		if (ground == nullptr || std::abs(normal.x) < 0.5f)
		{
			continue;
		}

		// 壁から離れる方向の入力なら選ばない
		if (normal.x * inputX >= 0.f)
		{
			continue;
		}

		const auto* groundCollider =
			ground->GetComponent<BoxCollider2D>();

		if (groundCollider == nullptr)
		{
			continue;
		}

		const AABB groundAABB = groundCollider->GetWorldAABB();

		// プレイヤーがまだ壁の高さの範囲内にいるか
		const bool overlapsWallHeight =
			playerAABB.min.y < groundAABB.max.y - 1.f &&
			playerAABB.max.y > groundAABB.min.y + 1.f;

		if (overlapsWallHeight)
		{
			return normal;
		}
	}

	// 壁の上端を越えたら、上面を選ぶ
	DirectX::XMFLOAT2 bestNormal = { 0.f, 0.f };
	float bestDistance = 1000000.f;

	for (const auto& contact : groundContacts)
	{
		const GameObject* ground = contact.first;
		const DirectX::XMFLOAT2 normal = contact.second;

		if (ground == nullptr || normal.y < 0.5f)
		{
			continue;
		}

		const auto* groundCollider =
			ground->GetComponent<BoxCollider2D>();

		if (groundCollider == nullptr)
		{
			continue;
		}

		const AABB groundAABB = groundCollider->GetWorldAABB();

		// プレイヤーの足元と地形上面の距離
		const float distance =
			std::abs(playerAABB.min.y - groundAABB.max.y);

		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestNormal = normal;
		}
	}

	return bestNormal;
}

void PlayerController::HandleNormalMovement()
{
	if (rb == nullptr)
	{
		return;
	}

	float dirX = inputX;
	float dirY = inputY;

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



void PlayerController::OnCollisionEnter2D(const GameObject& other)
{

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

}

	
void PlayerController::OnCollisionEnter2D(
	const GameObject& other,
	const CollisionManifold& manifold)
{
	OnCollisionEnter2D(other);

	if (other.GetTag() == ObjectTag::GROUND)
	{
		DirectX::XMFLOAT2 normal = {
			-manifold.normal.x,
			-manifold.normal.y
		};

		groundContacts[&other] = normal;
		collisionNormal = normal;
		isTouchingGround = true;
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
		rb->SetVelocity({ 0.0f, 0.0f });
		rb->SetAngularVel(0.f);
		rb->Wake();
	}

	isDead = false;
	isGrounded = false;

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
	MonoBehavior::OnCollisionStay2D(other);
}
void PlayerController::OnCollisionStay2D(
	const GameObject& other,
	const CollisionManifold& manifold)
{
	// 床に接触している場合だけ接地状態にする
	if (other.GetTag() == ObjectTag::GROUND)
	{
		DirectX::XMFLOAT2 normal = {
			-manifold.normal.x,
			-manifold.normal.y
		};

		isGrounded = normal.y > 0.5f;
	}

	if (other.GetTag() == ObjectTag::GROUND)
	{
		DirectX::XMFLOAT2 normal = {
			-manifold.normal.x,
			-manifold.normal.y
		};

		groundContacts[&other] = normal;
		collisionNormal = normal;
		isTouchingGround = true;
		isGrounded = normal.y > 0.5f;
	}
}
void PlayerController::OnCollisionExit2D(const GameObject& other)
{
	if (other.GetTag() == ObjectTag::GROUND)
	{
		groundContacts.erase(&other);

		isTouchingGround = !groundContacts.empty();

		if (isTouchingGround)
		{
			collisionNormal = groundContacts.begin()->second;
		}
		else
		{
			collisionNormal = { 0.f, 0.f };
			isGrounded = false;
		}
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

