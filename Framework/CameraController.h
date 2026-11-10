#pragma once
#include "MonoBehavior.h"

class Camera2D;


class CameraController : public MonoBehavior
{
public:
	void Awake() override;
	void LateUpdate(float deltaTime) override;

	//シーン遷移注かどうかの確認
	bool IsTransitioning() const
	{
		return isTransitioning;
	}

	//プレイヤー位置を含む固定カメラへ移動する
	void SnapToTarget();

private:
	Camera2D* camera = nullptr;

	DirectX::XMFLOAT2 transitionStart = {};
	DirectX::XMFLOAT2 transitionTarget = {};

	float viewWidth = 1366.0f;
	float viewHeight = 768.0f;

	float transitionTimer = 0.0f;
	float transitionDuration = 0.60f;

	bool isTransitioning = false;
};
