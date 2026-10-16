#pragma once
#include "MonoBehavior.h"

class Camera2D;

// 付けたオブジェクトをカメラが滑らかに追従する
class CameraController : public MonoBehavior
{
public:
	void Awake() override;
	void LateUpdate(float deltaTime) override;

	// 大きいほど速く追いつく（0で追従しない）
	float followSpeed = 5.f;

private:
	Camera2D* camera = nullptr;
};
