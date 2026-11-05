#include "CameraController.h"

#include <algorithm>
#include <cmath>

#include "Camera2D.h"
#include "GameObject.h"
#include "WindowSettings.h"

void CameraController::Awake()
{
	MonoBehavior::Awake();
	camera = &owner->GetContext().camera;

	viewWidth = static_cast<float>(WIN_WIDTH);
	viewHeight = static_cast<float>(WIN_HEIGHT);

	//開始時はプレイヤーを含む固定カメラ位置に合わせる
	SnapToTarget();
}

void CameraController::LateUpdate(float deltaTime)
{
	if (camera == nullptr)
	{
		return;
	}

	if (!isTransitioning)
	{
		const DirectX::XMFLOAT2 cameraPosition = camera->GetPosition();
		const DirectX::XMFLOAT3 playerPosition = owner->GetTransform()->GetPosition();

		float moveX = 0.0f;
		float moveY = 0.0f;

		const float halfWidth = viewWidth * 0.5f;
		const float halfHeight = viewHeight * 0.5f;

		if (playerPosition.x > cameraPosition.x + halfWidth)
		{
			moveX = 1.0f;
		}
		else if (playerPosition.x < cameraPosition.x - halfWidth)
		{
			moveX = -1.0f;
		}

		if (playerPosition.y > cameraPosition.y + halfHeight)
		{
			moveY = 1.0f;
		}
		else if (playerPosition.y < cameraPosition.y - halfHeight)
		{
			moveY = -1.0f;
		}

		//どの端も越えていなければ、カメラはそのまま
		if (moveX == 0.0f && moveY == 0.0f)
		{
			return;
		}

		transitionStart = cameraPosition;

		//次の固定カメラ位置を決める
		transitionTarget =
		{
			cameraPosition.x + moveX * viewWidth,
			cameraPosition.y + moveY * viewHeight
		};

		transitionTimer = 0.0f;
		isTransitioning = true;
	}

	if (transitionDuration <= 0.0f)
	{
		camera->SetPosition(transitionTarget);
		isTransitioning = false;
		return;
	}

	transitionTimer = std::min(transitionTimer + deltaTime, transitionDuration);

	float progress = transitionTimer / transitionDuration;

	//開始と終了をなめらかにする
	progress = progress * progress * (3.0f - 2.0f * progress);

	const DirectX::XMFLOAT2 cameraPosition =
	{
		transitionStart.x + (transitionTarget.x - transitionStart.x) * progress,
		transitionStart.y + (transitionTarget.y - transitionStart.y) * progress
	};

	camera->SetPosition(cameraPosition);

	if (transitionTimer >= transitionDuration)
	{
		camera->SetPosition(transitionTarget);
		isTransitioning = false;
	}
}

void CameraController::SnapToTarget()
{
	if (camera == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 playerPosition = owner->GetTransform()->GetPosition();

	//プレイヤーを含む固定画面の場所の番号
	const float cameraCellX = std::floor((playerPosition.x + viewWidth * 0.5f) / viewWidth);

	const float cameraCellY = std::floor((playerPosition.y + viewHeight * 0.5f) / viewHeight);

	//固定画面の番号から固定カメラ位置を計算する
	const DirectX::XMFLOAT2 cameraPosition =
	{
		cameraCellX * viewWidth,
		cameraCellY * viewHeight
	};

	camera->SetPosition(cameraPosition);

	transitionStart = cameraPosition;
	transitionTarget = cameraPosition;
	transitionTimer = 0.0f;
	isTransitioning = false;
}