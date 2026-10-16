#include "CameraController.h"

#include <cmath>

#include "Camera2D.h"
#include "GameObject.h"

void CameraController::Awake()
{
	MonoBehavior::Awake();
	camera = &owner->GetContext().camera;
}

void CameraController::LateUpdate(float deltaTime)
{
	if (camera == nullptr || followSpeed <= 0.f)
	{
		return;
	}

	DirectX::XMFLOAT2 camPos = camera->GetPosition();
	const DirectX::XMFLOAT3 targetPos = owner->GetTransform()->GetPosition();

	// フレームレートに依存しない指数補間
	const float t = 1.f - std::exp(-followSpeed * deltaTime);
	camPos.x += (targetPos.x - camPos.x) * t;
	camPos.y += (targetPos.y - camPos.y) * t;

	camera->SetPosition(camPos);
}
