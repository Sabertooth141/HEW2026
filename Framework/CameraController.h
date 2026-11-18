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
	//bool IsTransitioning() const
	//{
	//	return isTransitioning;
	//}

	//リトライなどのときプレイヤーへカメラを即座に合わせる
	void SnapToTarget();

private:
	Camera2D* camera = nullptr;

	//DirectX::XMFLOAT2 transitionStart = {};
	//DirectX::XMFLOAT2 transitionTarget = {};

	float viewWidth = 1366.0f;
	float viewHeight = 768.0f;

	//float transitionTimer = 0.0f;
	//float transitionDuration = 0.60f;

	//bool isTransitioning = false;

	// カメラが追従する速さ。大きくすると速く、小さくするとゆっくり動く。
	float followSharpness = 8.0f;

	// 8枚のステージ背景画像の外周。
	// 画像の中心位置とサイズから計算した範囲。
	static constexpr float mapMinX = -960.0f;
	static constexpr float mapMaxX = 8640.0f;
	static constexpr float mapMinY = -1620.0f;
	static constexpr float mapMaxY = 540.0f;
};
