#pragma once
#include <unordered_map>
#include "MonoBehavior.h"
#include "SpeedBoostComponent.h"

class Rigidbody2DComponent;
class AnimatorComponent;
class PlayerAudioComponent;
class BoxCollider2D;

// サンプル用の基本的なプレイヤー操作
// WASDで上下左右に移動し、敵に触れるとHPが減る
class PlayerController : public MonoBehavior
{
public:
	~PlayerController() override = default;

	void Awake() override;
	void Update(float deltaTime) override;

	void OnCollisionEnter2D(const GameObject& other) override;

	void OnCollisionEnter2D(
		const GameObject& other,
		const CollisionManifold& manifold) override;

	void OnCollisionStay2D(
		const GameObject& other,
		const CollisionManifold& manifold) override;

	//チェックポイント
	void SetCheckpoint(const DirectX::XMFLOAT3& position, GameObject* checkpoint);
	DirectX::XMFLOAT3 GetCheckpointPosition() const
	{
		return lastCheckpointPosition;
	}
	//復帰処理
	void RespawnAtCheckPoint();

	float GetHP() const { return hp; }
	float GetMaxHP() const { return maxHP; }
	bool IsDead() const { return isDead || hp <= 0.f; }

	float moveSpeed = 300.f;
	float maxHP = 100.f;
	float invulnerabilityDuration = 1.f;

	float dashSpeedMultiplier=1.5f; //ダッシュの倍率

	//横方向の慣性
	float horizontalInertia = 0.f;

	void OnCollisionStay2D(const GameObject& other) override;
	void OnCollisionExit2D(const GameObject& other) override;

	// ジャンプ設定
float maxJumpHeight = 156.25f;  // 最大ジャンプ高度
float jumpToApexTime = 0.625f;  // 最高地点に到達するまでの時間

float jumpPower = 500.f;        // 計算によって決まるジャンプ速度

void UpdateJumpSettings();
	// プレイヤーをゲーム開始位置へ戻す。
	void RespawnAtInitialPosition();

	// 通常ジャンプで天井にぶつかった後の滞空時間
	float ceilingHangTimer = 0.f;

	// 天井にぶつかってから落下を始めるまでの時間
	float ceilingHangDuration = 0.2f;

private:

	enum class MovementMode
	{
		Normal, // 通常移動
		Loop    // ループ移動
	};

	MovementMode movementMode = MovementMode::Normal;
	void HandleLoopMovement(float deltaTime);

	void HandleInput();
	void HandleMovement(float deltaTime);
	void HandleAnimation();
	void HandleJump(float deltaTime);      // ジャンプ処理
	void Respawn();
	void HandleDash();

	// スライド
	void HandleSlideInput();
	void HandleSlideMovement(float deltaTime);
	void EndSlide();

	void HandleNormalMovement(float deltaTime);

	void HandleDive();

	bool isDiving = false;

	bool isDashing = false;//ダッシュ中か調べる

	// スライド中か
	bool isSliding = false;

	// スライドの残り時間
	float slideTimer = 0.f;

	// スライド継続時間
	float slideDuration = 0.35f;

	// スライド速度
	float slideSpeed = 700.f;

	// スライドする方向
	float slideDirection = 1.f;

	// 通常時の当たり判定を保存
	DirectX::XMFLOAT2 normalColliderHalfExtents = {};
	DirectX::XMFLOAT2 normalColliderOffset = {};


	BoxCollider2D* playerCollider = nullptr;

	float dashDirection = 1.f;

	bool isGrounded = false;

	// 入力がないときも最後に向いていた方向を保つ。
	bool isFacingRight = true;

	// コヨーテタイム
	float coyoteTime = 0.5f;
	float coyoteTimer = 0.f;


	bool isLedgeGraceRunning = false;
	float ledgeGracePreviousX = 0.f;
	float ledgeGraceDistanceTravelled = 0.f;
	float ledgeGraceLimit = 64.f;
	float ledgeGraceDirection = 0.f;

	

	// トゲに触れて死亡しているか
	bool isDead = false;

	// プレイヤーの足元のY座標
	float previousPlayerBottom = 0.f;
	float previousPlayerTop = 0.f;
	float previousPlayerLeft = 0.f;
	float previousPlayerRight = 0.f;

	DirectX::XMFLOAT3 lastCheckpointPosition = {};
	// 最後に触れたチェックポイントのオブジェクト。
	GameObject* lastCheckpointObject = nullptr;

	// ゲーム開始時のプレイヤー位置。
	DirectX::XMFLOAT3 initialPosition = {};

	DirectX::XMFLOAT2 SelectSurfaceNormal() const;

	Rigidbody2DComponent* rb = nullptr;
	AnimatorComponent* anim = nullptr;
	PlayerAudioComponent* playerAudio = nullptr;

	float normalGravity = 0.f;

	float inputX = 0.f;
	float inputY = 0.f;

	float hp = 100.f;
	float invulnerabilityTimer = 0.f;

	float playTimer = 0.f;

	DirectX::XMFLOAT2 collisionNormal = { 0.f, 0.f };

	// 前フレームで移動していた地形面
	DirectX::XMFLOAT2 previousSurfaceNormal = { 0.f, 0.f };
	bool hasPreviousSurfaceNormal = false;

	std::unordered_map<const GameObject*, DirectX::XMFLOAT2> groundContacts;

	bool isTouchingGround = false;

	bool wasTouchingCeiling = false;
	// 天井にぶつかった後、地面に着地するまで操作を禁止する
	bool isCeilingKnockback = false;

	// 壁の上端から上側の足場へ移動中か
	bool isWallTopTransitioning = false;
	// 壁の上端位置を記憶する
	float wallTopY = 0.f;
	// 壁の上端を越えるときの移動方向
	float wallTopDirection = 0.f;
	bool hasWallTop = false;
	SpeedBoostComponent* speedBoost = nullptr;

	bool hasUsedAirRunJump = false;
};
