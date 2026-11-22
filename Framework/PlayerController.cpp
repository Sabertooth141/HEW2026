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
#include "WarpComponent.h"

void PlayerController::Awake()
{
	MonoBehavior::Awake();

	rb = owner->GetComponent<Rigidbody2DComponent>();
	anim = owner->GetComponent<AnimatorComponent>();
	playerAudio = owner->GetComponent<PlayerAudioComponent>();
	if (rb != nullptr)
	{
		UpdateJumpSettings();
	}
	hp = maxHP;
	playTimer = owner->GetContext().globalContext.playTime;

	// シーンデータから読み込まれた開始位置を保存する。
	initialPosition = owner->GetTransform()->GetPosition();

	// チェックポイントに触れるまでは、開始位置を復帰地点にする。
	lastCheckpointPosition = initialPosition;

	// プレイヤーの当たり判定を取得
	playerCollider = owner->GetComponent<BoxCollider2D>();

	if (playerCollider != nullptr)
	{
		// 通常時の当たり判定の大きさと位置を保存
		normalColliderHalfExtents =
			playerCollider->GetHalfExtents();

		normalColliderOffset =
			playerCollider->GetOffset();

		// プレイヤーの現在の当たり判定を取得
		const AABB aabb = playerCollider->GetWorldAABB();

		previousPlayerBottom = aabb.min.y;
		previousPlayerTop = aabb.max.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}


	//加速
	speedBoost = owner->GetComponent<SpeedBoostComponent>();

}



void PlayerController::UpdateJumpSettings()
{
	if (rb == nullptr)
	{
		return;
	}
	// 0以下の設定値では計算しない
	if (maxJumpHeight <= 0.f || jumpToApexTime <= 0.f)
	{
		return;
	}
	normalGravity =
		(2.0f * maxJumpHeight) /
		(jumpToApexTime * jumpToApexTime);
	// 最大高度と到達時間からジャンプ速度を計算
	jumpPower =
		(2.0f * maxJumpHeight) /
		jumpToApexTime;
	rb->SetGravity(normalGravity);
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
		previousPlayerTop = aabb.max.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}
	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - deltaTime);
	auto* retryTransition =
		owner->GetComponent<RetryTransitionComponent>();//リトライ演出を管理するコンポーネントを取得

	// StageBackground8の下端を越えたら、開始位置へ戻す。
// StageBackground8の中心Yが-1080、高さが1080なので下端は-1620。
	constexpr float stageBackground8BottomY = -1620.0f;

	if (!IsDead() &&
		playerCollider != nullptr &&
		playerCollider->GetWorldAABB().max.y < stageBackground8BottomY)
	{
		RespawnAtInitialPosition();
		return;
	}

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

	// 通常の天井衝突後の滞空時間を減らす
	if (ceilingHangTimer > 0.f)
	{
		ceilingHangTimer = std::max(
			0.f,
			ceilingHangTimer - deltaTime
		);

		// 滞空時間が終わったら重力を戻して落下させる
		if (ceilingHangTimer <= 0.f && rb != nullptr)
		{
			rb->SetGravity(normalGravity);
		}
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

	HandleDash();//ダッシュの入力

	HandleSlideInput();//スライドの入力

	// 天井にぶつかって滞空している間は急降下できない
	if (ceilingHangTimer <= 0.f)
	{
		HandleDive();
	}

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

	// 滞空中はジャンプ入力を受け付けない
	if (ceilingHangTimer <= 0.f)
	{
		HandleJump(deltaTime);
	}
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
	if (keyboard->KeyIsPressed('D'))
	{
		inputX += 1.f;
	}

	if (keyboard->KeyIsPressed('A'))
	{
		inputX -= 1.f;
	}
	
}

void PlayerController::HandleDash()//ダッシュの入力
{
	if (keyboard == nullptr || rb == nullptr)
	{
		return;
	}
	if (!isDashing && keyboard->KeyIsTriggered('Z'))
	{
		isDashing = true;

		if (inputX > 0.f)
		{
			dashDirection = 1.f;
		}
		else if (inputX < 0.f)
		{
			dashDirection = -1.f;
		}
		else
		{
			if (isFacingRight)
			{
				dashDirection = 1.f;
			}
			else
			{
				dashDirection = -1.f;
			}
		}
	}
}

// Xキーでスライドを開始する
void PlayerController::HandleSlideInput()
{
	if (keyboard == nullptr || rb == nullptr || playerCollider == nullptr)
	{
		return;
	}

	// 接地していない・ダッシュ中・死亡中は開始しない
	if (!keyboard->KeyIsTriggered('X') ||
		isSliding || isDashing || !isGrounded || isDead)
	{
		return;
	}

	// スライド開始
	isSliding = true;
	slideTimer = slideDuration;

	// 最後に向いていた方向へ進む
	if (isFacingRight)
	{
		slideDirection = 1.f;
	}
	else
	{
		slideDirection = -1.f;
	}

	const float slideHalfHeight = normalColliderHalfExtents.y * 0.4f;

	playerCollider->SetHalfExtents({
		normalColliderHalfExtents.x,
		slideHalfHeight
		});

	// 当たり判定の下端を維持する
	playerCollider->SetOffset({
		normalColliderOffset.x,
		normalColliderOffset.y -
			(normalColliderHalfExtents.y - slideHalfHeight)
		});
}

// スライド中の移動
void PlayerController::HandleSlideMovement(float deltaTime)
{
	if (rb == nullptr)
	{
		EndSlide();
		return;
	}

	// スライド時間を減らす
	slideTimer -= deltaTime;

	// スライド時間が終了したら元に戻す
	if (slideTimer <= 0.f)
	{
		EndSlide();
		return;
	}

	// 向いている方向へ進む
	DirectX::XMFLOAT2 velocity = rb->GetVelocity();

	velocity.x = slideDirection * slideSpeed;

	rb->SetVelocity(velocity);
}


// スライドを終了して当たり判定を元に戻す
void PlayerController::EndSlide()
{
	if (playerCollider != nullptr)
	{
		// 元の当たり判定の大きさに戻す
		playerCollider->SetHalfExtents(normalColliderHalfExtents);

		// 元の当たり判定の位置に戻す
		playerCollider->SetOffset(normalColliderOffset);
	}

	isSliding = false;
	slideTimer = 0.f;
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

	// スライド中はスライド専用の移動を行う
	if (isSliding)
	{
		HandleSlideMovement(deltaTime);
		return;
	}
	const AABB playerAABB = playerCollider->GetWorldAABB();
	// 壁の上端を越えるときの移動方向
	float wallTopMoveDirection = inputX;

	if (isDashing)
	{
		wallTopMoveDirection = dashDirection;
	}
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
				if (wallNormal.x > 0.f)
				{
					wallTopDirection = -1.f;
				}
				else
				{
					wallTopDirection = 1.f;
				}

				// 壁の上端に到達したら横移動へ切り替える
				if (wallTopMoveDirection == wallTopDirection &&
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
		wallTopMoveDirection == wallTopDirection &&
		playerAABB.min.y >= wallTopY - 2.f)
	{
		isWallTopTransitioning = true;
	}

	// ダッシュ中の進行方向を取得する
	float wallClimbDirection = 0.f;

	if (isDashing)
	{
		wallClimbDirection = dashDirection;
	}

	// 壁から離れる方向にダッシュしているか
	const bool isMovingAwayFromWall =
		isTouchingWall &&
		wallClimbDirection != 0.f &&
		wallClimbDirection * wallNormal.x > 0.f;

	// 壁走り能力を持っているか
	const bool hasWallClimbAbility =
		speedBoost != nullptr &&
		speedBoost->HasAbility(
			SpeedBoostComponent::BoostAbility::WallClimb
		);

	// Spaceキーを押しながら、壁に向かっているか確認する
	const bool isWallClimbInput =//isWallClombInputで壁のぼり出来る条件が揃ってるかみてる
		hasWallClimbAbility &&
		keyboard != nullptr &&
		keyboard->KeyIsPressed(' ') &&//KeyzIsPressedがキー入力され続けてるか確認してる
		isTouchingWall &&
		wallClimbDirection != 0.f &&
		!isMovingAwayFromWall;

	// 実際に壁を登っている最中か
	const bool isWallRunning = isWallClimbInput;

	// 天井に接触した瞬間だけ処理する
	if (isTouchingCeiling && !wasTouchingCeiling)
	{
		// 猶予走行を解除
		isLedgeGraceRunning = false;

		if (isWallRunning)
		{
			// 壁走り中に天井へぶつかった場合
			// 従来どおり操作不能にして強制落下
			isCeilingKnockback = true;

			rb->SetGravity(normalGravity);

			rb->SetVelocity({
				wallNormal.x * 250.f,
				0.f
				});
		}
		else
		{
			// 通常ジャンプなどで天井にぶつかった場合
			// 少しだけその高さにとどまる
			ceilingHangTimer = ceilingHangDuration;

			// 重力を一時停止
			rb->SetGravity(0.f);

			// 上昇速度を止める
			auto velocity = rb->GetVelocity();
			velocity.y = 0.f;
			rb->SetVelocity(velocity);
		}
	}

	wasTouchingCeiling = isTouchingCeiling;

	



	
	// 地形に沿った移動へ切り替える
	const bool useSurfaceMovement =
		hasWallClimbAbility &&
		(
			isWallTopTransitioning ||
			(
				isWallClimbInput &&
				!isTouchingCeiling
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

				float currentSpeed = speedBoost->GetMoveSpeed(moveSpeed);
				float moveDirection;

				if (isDashing)
				{
					moveDirection = dashDirection;
				}
				else
				{
					moveDirection = inputX;
				}

				// ダッシュ中は速度倍率を適用する
				if (isDashing)
				{
					currentSpeed *= dashSpeedMultiplier;
				}

				rb->SetVelocity({
					moveDirection * currentSpeed,
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
	if (useSurfaceMovement)
	{
		movementMode = MovementMode::Loop;
	}
	else
	{
		movementMode = MovementMode::Normal;
	}

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
	// ダッシュ中に左右入力があれば、先に方向を更新する
	if (isDashing && inputX != 0.f)
	{
		dashDirection = inputX;
	}

	// ダッシュ中は最後に設定した方向を維持する
	float moveDirection;

	if (isDashing)
	{
		moveDirection = dashDirection;
	}
	else
	{
		moveDirection = inputX;
	}
	// 壁の上端から足場へ移動中
	if (isWallTopTransitioning)
	{
		float currentSpeed = speedBoost->GetMoveSpeed(moveSpeed);

		// ダッシュ中は速度倍率を適用する
		if (isDashing)
		{
			currentSpeed *= dashSpeedMultiplier;
		}

		rb->SetVelocity({
			moveDirection * currentSpeed,
			0.f
			});

		return;
	}

	if (!isTouchingGround)//地形にぶつかってないと地形に沿った移動はしない
	{
		return;
	}

	// 入力がない場合は移動を停止する
	if (moveDirection == 0.f)
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
	if (moveDirection < 0.f)
	{
		tangent.x = -tangent.x;
		tangent.y = -tangent.y;
	}

	float currentSpeed = speedBoost->GetMoveSpeed(moveSpeed);

	// ダッシュ中は速度倍率を適用する
	if (isDashing)
	{
		currentSpeed *= dashSpeedMultiplier;
	}

	// 地形に沿って移動
	rb->SetVelocity({
		tangent.x * currentSpeed,
		tangent.y * currentSpeed
		});
}


DirectX::XMFLOAT2 PlayerController::SelectSurfaceNormal() const
{
	// 地形を選ぶときに使う移動方向を決める
	float moveDirection;

	if (isDashing)
	{
		moveDirection = dashDirection;
	}
	else
	{
		moveDirection = inputX;
	}

	if (groundContacts.empty() || moveDirection == 0.f)
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
		if (normal.x * moveDirection >= 0.f)
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

	float currentMoveSpeed;

	if (speedBoost != nullptr)
	{
		currentMoveSpeed = speedBoost->GetMoveSpeed(moveSpeed);
	}
	else
	{
		currentMoveSpeed = moveSpeed;
	}

	// ダッシュ中だけ速度を1.5倍にする
	if (isDashing)
	{
		currentMoveSpeed *= dashSpeedMultiplier;
	}

	// ダッシュ中はキーを離しても進行方向を維持する
	float moveDirection;

	if (isDashing)
	{
		moveDirection = dashDirection;
	}
	else
	{
		moveDirection = dirX;
	}

	// 目標となる横速度を計算
	const float targetVelocityX = moveDirection * currentMoveSpeed;

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

	// ダッシュ中に左右入力があれば方向を変更する
	if (isDashing && inputX != 0.f)
	{
		dashDirection = inputX;
	}
}

void PlayerController::HandleAnimation()
{
	if (anim == nullptr)
	{
		return;
	}

	// 実際の横移動速度を取得する。
	// Rigidbodyがない場合は、停止中として扱う。
	float velocityX = 0.0f;

	if (rb != nullptr)
	{
		velocityX = rb->GetVelocity().x;
	}

	// 微小な速度で向きが頻繁に切り替わらないようにする。
	constexpr float moveThreshold = 0.1f;

	// プレイヤーの移動している方向に向きを更新する。
	if (velocityX > moveThreshold)
	{
		isFacingRight = true;
	}
	else if (velocityX < -moveThreshold)
	{
		isFacingRight = false;
	}

	const char* nextAnimation = nullptr;

	if (isDashing)
	{
		//実際の移動中ではなく、ダッシュ開始時に向いていた方向
		nextAnimation = dashDirection >= 0.0f ? "DashRight" : "DashLeft";
	}

	else if (!isGrounded)
	{
		if (isFacingRight)
		{
			nextAnimation = "JumpRight";
		}
		else
		{
			nextAnimation = "JumpLeft";
		}
	}
	else if (std::abs(velocityX) > moveThreshold)
	{
		if (isFacingRight)
		{
			nextAnimation = "WalkRight";
		}
		else
		{
			nextAnimation = "WalkLeft";
		}
	}
	else
	{
		if (isFacingRight)
		{
			nextAnimation = "StaticRight";
		}
		else
		{
			nextAnimation = "StaticLeft";
		}
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


			const auto* spikeTransform = other.GetTransform();
			const float spikeRotation =
				spikeTransform->GetRotation().z;

			// 180度回転しているか
	// GetRotation() はラジアンなので、そのままcosで判定する
			const bool upsideDown = std::cos(spikeRotation) < 0.f;

			const float spikeTop = spikeAABB.max.y;
			const float spikeBottom = spikeAABB.min.y;

			const float playerBottom = playerAABB.min.y;
			const float playerTop = playerAABB.max.y;

			// プレイヤーとトゲが横方向に重なっているか
			const bool horizontalOverlap =
				previousPlayerRight > spikeAABB.min.x &&
				previousPlayerLeft < spikeAABB.max.x;

			// 通常向き：上から落ちてトゲに接触したか
			const bool landedOnSpike =
				rb->GetVelocity().y < 0.f &&
				previousPlayerBottom >= spikeTop - 1.f &&
				playerBottom < spikeTop;

			// 上下逆：下から上昇してトゲに接触したか
			const bool hitUpsideDownSpike =
				rb->GetVelocity().y > 0.f &&
				previousPlayerTop <= spikeBottom + 1.f &&
				playerTop > spikeBottom;

			// 向きに応じて死亡判定を切り替える
			bool hitSpike;

			if (upsideDown)
			{
				hitSpike = hitUpsideDownSpike;
			}
			else
			{
				hitSpike = landedOnSpike;
			}

			if (horizontalOverlap && hitSpike)
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
	// 相手がワープTriggerなら、指定位置へ移動する。
	if (const auto* warp = other.GetComponent<WarpComponent>())
	{
		WarpToPosition(warp->GetTargetPosition());
		return;
	}

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

	// 壁との衝突か確認する
	const bool isWallCollision = std::abs(normal.x) > 0.5f;

	// スライド中に壁へぶつかったらスライドを終了する
	if (isSliding && isWallCollision)
	{
		EndSlide();

		if (rb != nullptr)
		{
			auto velocity = rb->GetVelocity();
			velocity.x = 0.f;
			rb->SetVelocity(velocity);
		}
	}

	// 壁を壊す能力を持っているか記録する
	const bool canBreakBoostWall =
		speedBoost != nullptr &&
		speedBoost->HasAbility(
			SpeedBoostComponent::BoostAbility::BreakWall
		);


	// 壁登り能力を持っているか
	bool canClimbThisWall = false;

	if (isDashing &&
		keyboard != nullptr &&
		keyboard->KeyIsPressed(' ') &&
		dashDirection * normal.x < 0.f &&
		speedBoost != nullptr &&
		speedBoost->HasAbility(
			SpeedBoostComponent::BoostAbility::WallClimb))
	{
		canClimbThisWall = true;
	}

	// 壁登りをしない状態でダッシュ中に壁へぶつかった場合
	if (isDashing && isWallCollision && !canClimbThisWall)
	{
		isDashing = false;

		//if (speedBoost != nullptr)
		//{
		//	speedBoost->ResetBoost();
		//}

		// 壁から押し戻す
		if (rb != nullptr)
		{
			auto velocity = rb->GetVelocity();

			velocity.x = normal.x * 180.f;

			rb->SetVelocity(velocity);
		}
	}

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
	if (speedBoost != nullptr&&isDashing)
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
		rb->SetGravity(normalGravity);
		rb->SetVelocity({ 0.0f, 0.0f });
		rb->SetAngularVel(0.f);
		rb->Wake();
	}

	// 特殊移動や天井衝突の状態を解除
	groundContacts.clear();
	isTouchingGround = false;
	isCeilingKnockback = false;
	ceilingHangTimer = 0.f;
	wasTouchingCeiling = false;
	isLedgeGraceRunning = false;
	movementMode = MovementMode::Normal;
	// 復帰時にダッシュ状態を解除する
	isDashing = false;
	dashDirection = 0.f;
	isDead = false;
	isGrounded = false;

	hp = maxHP;
	invulnerabilityTimer = invulnerabilityDuration;

	// 復帰後のプレイヤーの位置情報を更新する
	auto* playerCollider = owner->GetComponent<BoxCollider2D>();

	if (playerCollider != nullptr)
	{
		const AABB playerAABB = playerCollider->GetWorldAABB();

		previousPlayerBottom = playerAABB.min.y;
		previousPlayerTop = playerAABB.max.y;
		previousPlayerLeft = playerAABB.min.x;
		previousPlayerRight = playerAABB.max.x;
	}
}

void PlayerController::RespawnAtInitialPosition()
{
	// プレイヤーをゲーム開始位置へ戻す。
	owner->GetTransform()->SetPosition(initialPosition);

	// 落下中の速度や猶予移動の重力設定をリセットする。
	if (rb != nullptr)
	{
		rb->SetGravity(normalGravity);
		rb->SetVelocity({ 0.0f, 0.0f });
		rb->SetAngularVel(0.0f);
		rb->Wake();
	}

	// 落下前の接触状態や特殊移動状態を解除する。
	groundContacts.clear();
	// 復帰時にダッシュ状態を解除する
	isDashing = false;
	dashDirection = 0.f;
	isGrounded = false;
	isTouchingGround = false;
	isLedgeGraceRunning = false;
	ledgeGraceDistanceTravelled = 0.0f;
	isDiving = false;
	isCeilingKnockback = false;
	ceilingHangTimer = 0.f;
	wasTouchingCeiling = false;
	isWallTopTransitioning = false;
	hasWallTop = false;
	coyoteTimer = 0.0f;
	collisionNormal = { 0.0f, 0.0f };
	movementMode = MovementMode::Normal;

	// 落下によって加速が残らないようにする。
	if (speedBoost != nullptr)
	{
		speedBoost->ResetBoost();
	}

	// プレイヤー状態を通常に戻す。
	isDead = false;
	hp = maxHP;
	invulnerabilityTimer = invulnerabilityDuration;

	// 復帰先へカメラを合わせる。
	CameraController* cameraController =
		owner->GetComponent<CameraController>();

	if (cameraController != nullptr)
	{
		cameraController->SnapToTarget();
	}

	// 次の衝突判定で使う前フレームの位置情報も更新する。
	BoxCollider2D* playerCollider =
		owner->GetComponent<BoxCollider2D>();

	if (playerCollider != nullptr)
	{
		const AABB playerAABB = playerCollider->GetWorldAABB();

		previousPlayerBottom = playerAABB.min.y;
		previousPlayerTop = playerAABB.max.y;
		previousPlayerLeft = playerAABB.min.x;
		previousPlayerRight = playerAABB.max.x;
	}
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
	// 針との接触が続いている間も、死亡判定を行う
	if (other.GetTag() == ObjectTag::SPIKE)
	{
		OnCollisionEnter2D(other);
		return;
	}

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
	isTouchingGround = !groundContacts.empty();
	//接触中の地形で一つでも床があれば接地扱い
	isGrounded = std::any_of(
		groundContacts.begin(),
		groundContacts.end(),
		[](const auto& contact)
		{
			return contact.second.y > 0.5f;
		});

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
		previousPlayerTop = aabb.max.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}
}

void PlayerController::WarpToPosition(const DirectX::XMFLOAT3& position)
{
	//スライド中なら、当たり判定を通常状態に戻す
	if (isSliding)
	{
		EndSlide();
	}

	owner->GetTransform()->SetPosition(position);

	if (rb != nullptr)
	{
		rb->SetGravity(normalGravity);
		rb->SetVelocity({ 0.0f,0.0f });
		rb->SetAngularVel(0.0f);
		rb->Wake();
	}

	//ワープ前の接触・特殊移動状態を解除する
	// ワープ前の接触・特殊移動状態を解除する。
	groundContacts.clear();
	isGrounded = false;
	isTouchingGround = false;
	isDashing = false;
	dashDirection = 0.0f;
	isDiving = false;
	isLedgeGraceRunning = false;
	ledgeGraceDistanceTravelled = 0.0f;
	isWallTopTransitioning = false;
	hasWallTop = false;
	isCeilingKnockback = false;
	ceilingHangTimer = 0.0f;
	wasTouchingCeiling = false;
	hasPreviousSurfaceNormal = false;
	movementMode = MovementMode::Normal;

	//カメラを移動先へ即合わせる
	if (auto* cameraController = owner->GetComponent<CameraController>())
	{
		cameraController->SnapToTarget();
	}

	//前フレームの当たり判定位置もワープ先に更新する
	if (auto* playerCollider = owner->GetComponent<BoxCollider2D>())
	{
		const AABB aabb = playerCollider->GetWorldAABB();

		previousPlayerBottom = aabb.min.y;
		previousPlayerTop = aabb.max.y;
		previousPlayerLeft = aabb.min.x;
		previousPlayerRight = aabb.max.x;
	}
}