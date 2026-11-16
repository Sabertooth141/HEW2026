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
#include "BreakableObjectComponent.h"

#include "PlayerAudioComponent.h"
#include "CheckpointComponent.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	playerAudio = owner->GetComponent<PlayerAudioComponent>();
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


	// 天井にぶつかった後、地面に着地するまで操作を禁止する
	if (isCeilingKnockback)
	{
		// 地面に着地したら操作禁止を解除
		if (isGrounded)
		{
			isCeilingKnockback = false;
		}
		else
		{
			// 落下中は操作・ジャンプを受け付けない
			HandleAnimation();

			if (playerAudio != nullptr)
			{
				playerAudio->UpdateWalk(deltaTime, false);
			}

			playTimer += deltaTime;
			return;
		}
	}

	HandleInput();//キー入力取得
	HandleDive();
	HandleMovement(deltaTime);//地面や壁の状態で移動方法を決める

	// 今のフレームで天井にぶつかった場合も、ジャンプを処理しない
	if (isCeilingKnockback)
	{
		HandleAnimation();

		if (playerAudio != nullptr)
		{
			playerAudio->UpdateWalk(deltaTime, false);
		}

		playTimer += deltaTime;
		return;
	}

	HandleJump(deltaTime);
	HandleAnimation();




	// 接地中に横方向へ実際に移動している場合だけ、歩行音を鳴らす
	if (playerAudio != nullptr && rb != nullptr)
	{
		const bool isWalking =
			isGrounded &&
			std::abs(inputX) > 0.5f &&
			std::abs(rb->GetVelocity().x) > 0.1f;

		playerAudio->UpdateWalk(deltaTime, isWalking);
	}

	playTimer += deltaTime;
	
}

void PlayerController::HandleInput()//キー入力
{
	inputX = 0.f;
	inputY = 0.f;

	if (keyboard->KeyIsPressed('D')) inputX += 1.f;
	if (keyboard->KeyIsPressed('A')) inputX -= 1.f;
	
}

// 空中でSキーを押すと急降下する
void PlayerController::HandleDive()
{
	if (rb == nullptr || keyboard == nullptr || speedBoost == nullptr)
	{
		return;
	}

	if (!speedBoost->HasAbility(SpeedBoostComponent::BoostAbility::Dive))
	{
		return;
	}

	// 地面や壁に接触している間は急降下できない
	if (isTouchingGround || isGrounded)
	{
		return;
	}

	if (keyboard->KeyIsTriggered('S'))
	{
		isDiving = true;

		DirectX::XMFLOAT2 velocity = rb->GetVelocity();

		// 下方向へ強い速度を与える
		velocity.y = -moveSpeed * 2.f;

		rb->SetVelocity(velocity);
	}
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

	// 壁上端への移動中に足場との接触がなくなったら通常移動に戻す
	if (isWallTopTransitioning && !isTouchingGround)
	{
		isWallTopTransitioning = false;
		hasWallTop = false;
	}

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
			if (wallCollider != nullptr)
			{
				const AABB wallAABB = wallCollider->GetWorldAABB();

				// 壁の上端位置を記憶する
				wallTopY = wallAABB.max.y;
				hasWallTop = true;

				// 壁の位置から、上端を越えるときの移動方向を記憶する
				wallTopDirection = wallNormal.x > 0.f ? -1.f : 1.f;

				// 壁の上端に到達したら横移動へ切り替える
				if (inputX == wallTopDirection &&
					playerAABB.min.y >= wallTopY - 2.f)
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
					hasWallTop = false;
				}
			}
		}
	}
	// 壁との接触が切れても、記憶した上端位置で判定する
	if (hasWallTop &&
		inputX == wallTopDirection &&
		playerAABB.min.y >= wallTopY - 2.f)
	{
		isWallTopTransitioning = true;
	}

	// 天井に接触した瞬間だけノックバック
	if (isTouchingCeiling && !wasTouchingCeiling)
	{
		isCeilingKnockback = true;

		rb->SetVelocity({
			wallNormal.x * 250.f,
			0.f
			});//壁の法線方向にノックバック
	}

	wasTouchingCeiling = isTouchingCeiling;

	

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

	if (isLedgeGraceRunning)
	{
		// 飛び出した方向への入力がなくなったら落下開始
		if (inputX != ledgeGraceDirection)//ledgeGraceDirectionは飛び出した時の方向
		{
			isLedgeGraceRunning = false;
			rb->SetGravity(normalGravity);

			//コヨーテタイムを開始
			coyoteTimer = coyoteTime;
		}
		else//方向が変わっていない場合
		{
			const auto position = owner->GetTransform()->GetPosition();

			//前回から移動した距離を加算
			ledgeGraceDistanceTravelled +=//ledgeGraceDistanceTravelledは飛び出してから移動した距離
				std::abs(position.x - ledgeGracePreviousX);

			ledgeGracePreviousX = position.x;

			//走り切ったら落下開始
			if (ledgeGraceDistanceTravelled >= ledgeGraceLimit)
			{
				isLedgeGraceRunning = false;

				rb->SetGravity(normalGravity);

				//コヨーテタイムを開始
				coyoteTimer = coyoteTime;
			}
			else
			{
				// 猶予走行中は高さを維持して横に進む
				rb->SetGravity(0.f);

				const float currentSpeed =
					speedBoost != nullptr
					? speedBoost->GetMoveSpeed(moveSpeed)
					: moveSpeed;

				rb->SetVelocity({
					inputX * currentSpeed,
					0.f
					});

				return;
			}
		}
	}

	// 天井にぶつかった直後はノックバックの速度を維持する
	if (isCeilingKnockback)
	{
		return;
	}

	// 壁のぼりが可能な状態なら地形に沿った移動に切り替える
	movementMode = useSurfaceMovement
		? MovementMode::Loop
		: MovementMode::Normal;

	switch (movementMode)
	{
	case MovementMode::Normal://通常移動
		HandleNormalMovement(deltaTime);
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

void PlayerController::HandleNormalMovement(float deltaTime)
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

	// 入力から目標となる横速度を計算
	const float targetVelocityX = dirX * currentMoveSpeed;

	// 慣性が0なら従来どおり即座に速度を変更
	float newVelocityX = targetVelocityX;

	if (horizontalInertia > 0.f)
	{
		// 慣性の値が大きいほど、目標速度にゆっくり近づく
		const float rate =
			1.f - std::exp(-deltaTime / horizontalInertia);

		const float currentVelocityX = rb->GetVelocity().x;

		newVelocityX = currentVelocityX
			+ (targetVelocityX - currentVelocityX) * rate;
	}

	// 横速度だけ変更し、ジャンプや落下の速度は維持する
	rb->SetVelocity({
		newVelocityX,
		rb->GetVelocity().y
		});
}

void PlayerController::HandleAnimation()
{
	if (anim == nullptr)
	{
		return;
	}

	// 左右入力があるときだけ、向きを更新する。
	if (inputX > 0.0f)
	{
		isFacingRight = true;
	}
	else if (inputX < 0.0f)
	{
		isFacingRight = false;
	}

	const char* nextAnimation = nullptr;

	if (!isGrounded)
	{
		nextAnimation = isFacingRight ? "JumpRight" : "JumpLeft";
	}
	else if (std::abs(inputX) > 0.01f)
	{
		nextAnimation = isFacingRight ? "WalkRight" : "WalkLeft";
	}
	else
	{
		nextAnimation = isFacingRight ? "StaticRight" : "StaticLeft";
	}

	if (anim->GetCurrAnimName() != nextAnimation)
	{
		anim->SetCurrAnimation(nextAnimation);
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
	// 既存のトゲとの衝突処理
	OnCollisionEnter2D(other);

	if (other.GetTag() != ObjectTag::GROUND &&
		other.GetTag() != ObjectTag::ONE_WAY_PLATFORM)
	{
		return;
	}

	DirectX::XMFLOAT2 normal =
	{
		-manifold.normal.x,
		-manifold.normal.y
	};

	const auto* breakable =
		other.GetComponent<BreakableObjectComponent>();


	if (breakable != nullptr)
	{
		// 加速スタックが2以上で、壊れる壁に横からぶつかった場合
		if (breakable->GetBreakType() ==
			BreakableObjectComponent::BreakType::Boost &&
			speedBoost != nullptr &&
			speedBoost->HasAbility(SpeedBoostComponent::BoostAbility::BreakWall) &&
			std::abs(normal.x) > 0.5f)
		{
			breakable->Break();
			return;
		}

		// 急降下中に、壊れる床へ上からぶつかった場合
		if (breakable->GetBreakType() ==
			BreakableObjectComponent::BreakType::Dive &&
			isDiving &&
			normal.y > 0.5f)
		{
			breakable->Break();
			isDiving = false;
			return;
		}
	}


	// 通常の地形との接触処理
	groundContacts[&other] = normal;
	collisionNormal = normal;
	isTouchingGround = true;

	if (normal.y > 0.5f)
	{
		isGrounded = true;
		isDiving = false;

		//地面の上に新しく接触したときに着地音を鳴らす
		if (playerAudio != nullptr)
		{
			playerAudio->PlayLand();
		}

		DirectX::XMFLOAT2 normal = {
			-manifold.normal.x,
			-manifold.normal.y
		};

		groundContacts[&other] = normal;
		collisionNormal = normal;
		isTouchingGround = true;
	}
}

// プレイヤーが最後に触れたチェックポイントを登録する。
void PlayerController::SetCheckpoint(
	const DirectX::XMFLOAT3& position,
	GameObject* checkpoint)
{
	// 別のチェックポイントに触れたときだけ、旗の表示を切り替える。
	if (lastCheckpointObject != checkpoint)
	{
		// 以前の旗があれば、アニメーションを止めて静止画像に戻す。
		if (lastCheckpointObject != nullptr)
		{
			CheckpointComponent* previousCheckpoint =
				lastCheckpointObject->GetComponent<CheckpointComponent>();

			if (previousCheckpoint != nullptr)
			{
				previousCheckpoint->SetActivated(false);
			}
		}

		// 今回触れた旗を、最後に触れたチェックポイントとして記録する。
		lastCheckpointObject = checkpoint;

		// 新しい旗だけアニメーションさせる。
		if (lastCheckpointObject != nullptr)
		{
			CheckpointComponent* currentCheckpoint =
				lastCheckpointObject->GetComponent<CheckpointComponent>();

			if (currentCheckpoint != nullptr)
			{
				currentCheckpoint->SetActivated(true);
			}
		}
	}

	// 復帰時に使う座標を保存する。
	lastCheckpointPosition = position;

	// 既存の加速処理。今回の旗を加速判定へ渡す。
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

void PlayerController::HandleJump(float deltaTime)
{
	if (rb == nullptr || keyboard == nullptr)
	{
		return;
	}

	// 地面にいる間はジャンプ猶予時間をリセット
	if (isGrounded)
	{
		coyoteTimer = coyoteTime;
	}
	else if (!isLedgeGraceRunning)
	{
		// 猶予走行が終わってからジャンプ猶予時間を減らす
		coyoteTimer = std::max(0.f, coyoteTimer - deltaTime);
	}

	// 猶予時間内ならジャンプできる
	const bool canAirRunJump =
		isLedgeGraceRunning &&
		speedBoost != nullptr &&
		speedBoost->HasAbility(
			SpeedBoostComponent::BoostAbility::AirRun) &&
		!hasUsedAirRunJump;

	// 通常のジャンプ、または空中走行中の追加ジャンプ
	if ((coyoteTimer > 0.f || canAirRunJump) &&
		keyboard->KeyIsTriggered(' '))
	{
		DirectX::XMFLOAT2 velocity = rb->GetVelocity();
		velocity.y = jumpPower;

		rb->SetVelocity(velocity);
		{
			DirectX::XMFLOAT2 velocity = rb->GetVelocity();
			velocity.y = jumpPower;

			rb->SetVelocity(velocity);

			//ジャンプ時にジャンプ音を鳴らす
			if (playerAudio != nullptr)
			{
				playerAudio->PlayJump();
			}

			if (canAirRunJump)
			{
				hasUsedAirRunJump = true;
				isLedgeGraceRunning = false;
				rb->SetGravity(normalGravity);
			}

			isGrounded = false;
			coyoteTimer = 0.f;
		}

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
	if (other.GetTag() != ObjectTag::GROUND &&
		other.GetTag() != ObjectTag::ONE_WAY_PLATFORM)
	{
		return;
	}

	DirectX::XMFLOAT2 normal =
	{
		-manifold.normal.x,
		-manifold.normal.y
	};

	groundContacts[&other] = normal;
	collisionNormal = normal;
	isTouchingGround = true;
	isGrounded = normal.y > 0.5f;

	if (isGrounded)
	{
		isDiving = false;
		isLedgeGraceRunning = false;
		ledgeGraceDistanceTravelled = 0.f;
	}
}
void PlayerController::OnCollisionExit2D(const GameObject& other)
{
	if (other.GetTag() == ObjectTag::GROUND ||
		other.GetTag() == ObjectTag::ONE_WAY_PLATFORM)
	{
		const bool wasGrounded = isGrounded;

		groundContacts.erase(&other);

		isTouchingGround = !groundContacts.empty();

		// まだ足場に接触しているか調べる
		bool hasFloorContact = false;

		for (const auto& contact : groundContacts)
		{
			if (contact.second.y > 0.5f)
			{
				hasFloorContact = true;
				break;
			}
		}

		isGrounded = hasFloorContact;

		if (isTouchingGround)
		{
			collisionNormal = groundContacts.begin()->second;
		}
		else
		{
			collisionNormal = { 0.f, 0.f };
		}

		// 最後の足場から離れたときだけ猶予走行を開始
		if (wasGrounded && !isGrounded && !isLedgeGraceRunning)
		{
			const DirectX::XMFLOAT2 velocity = rb->GetVelocity();

			// ジャンプ上昇中ではない場合に開始
			if (velocity.y <= 0.f && inputX != 0.f)
			{
				const auto position = owner->GetTransform()->GetPosition();

				ledgeGracePreviousX = position.x;
				ledgeGraceDistanceTravelled = 0.f;

				// 足場から飛び出した方向を記録
				ledgeGraceDirection = inputX;

				// 空中走行能力が有効ならそっちを優先、通常時は
				ledgeGraceLimit = 64.f;

				if (speedBoost != nullptr &&
					speedBoost->HasAbility(
						SpeedBoostComponent::BoostAbility::AirRun))
				{
					ledgeGraceLimit = 256.f;
				}

				isLedgeGraceRunning = true;
				coyoteTimer = 0.f;
				hasUsedAirRunJump = false;
			}
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

