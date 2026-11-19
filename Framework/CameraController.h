#pragma once
#include "MonoBehavior.h"

class Camera2D;


class CameraController : public MonoBehavior
{
public:
	// 画面に映す範囲
	// 通常範囲に対しての比率
	static constexpr float gameplayViewRange = 0.75f;


	void Awake() override;
	void LateUpdate(float deltaTime) override;

	////シーン遷移注かどうかの確認
	bool IsTransitioning() const
	{
		return isTransitioning;
	}

	//リトライなどのときプレイヤーへカメラを即座に合わせる
	void SnapToTarget();

private:
	int FindRoom(const DirectX::XMFLOAT3& position)  const;

	DirectX::XMFLOAT2 ClampToRoom(DirectX::XMFLOAT2 position, int roomIndex) const;

	Camera2D* camera = nullptr;
	int currentRoom = -1;

	DirectX::XMFLOAT2 transitionStart = {};
	DirectX::XMFLOAT2 transitionTarget = {};

	float viewWidth = 1366.0f;
	float viewHeight = 768.0f;

	float transitionTimer = 0.0f;
	float transitionDuration = 0.60f;

	bool isTransitioning = false;

	// カメラが追従する速さ。大きくすると速く、小さくするとゆっくり動く。
	float followSharpness = 8.0f;
};
