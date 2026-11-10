#pragma once
#include <unordered_map>
#include "MonoBehavior.h"
#include "SpeedBoostComponent.h"

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

	void OnCollisionEnter2D(
		const GameObject& other,
		const CollisionManifold& manifold) override;

	void OnCollisionStay2D(
		const GameObject& other,
		const CollisionManifold& manifold) override;

	//チェックポイント
	void SetCheckpoint(const DirectX::XMFLOAT3& position, const GameObject* checkpoint);
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

	void OnCollisionStay2D(const GameObject& other) override;
	void OnCollisionExit2D(const GameObject& other) override;

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
	void HandleJump();       // ジャンプ処理
	void Respawn();

	void HandleNormalMovement();

	bool isGrounded = false;
	float jumpPower = 500.f;

	// トゲに触れて死亡しているか
	bool isDead = false;

	// プレイヤーの足元のY座標
	float previousPlayerBottom = 0.f;
	float previousPlayerLeft = 0.f;
	float previousPlayerRight = 0.f;

	DirectX::XMFLOAT3 lastCheckpointPosition = {};

	DirectX::XMFLOAT2 SelectSurfaceNormal() const;

	Rigidbody2DComponent* rb = nullptr;
	AnimatorComponent* anim = nullptr;

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
	float ceilingKnockbackTimer = 0.f;

	// 壁の上端から上側の足場へ移動中か
	bool isWallTopTransitioning = false;
	// 壁の上端位置を記憶する
	float wallTopY = 0.f;
	bool hasWallTop = false;
	SpeedBoostComponent* speedBoost = nullptr;
};
