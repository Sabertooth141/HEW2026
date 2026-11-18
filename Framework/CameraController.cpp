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

	//viewWidth = static_cast<float>(WIN_WIDTH);
	//viewHeight = static_cast<float>(WIN_HEIGHT);

	// ズーム後に画面へ映るワールドサイズを計算する。
	viewWidth =
		static_cast<float>(WIN_WIDTH) * gameplayViewRange;

	viewHeight =
		static_cast<float>(WIN_HEIGHT) * gameplayViewRange;

	//開始時はカメラをプレイヤーに合わせる
	SnapToTarget();
}

void CameraController::LateUpdate(float deltaTime)//プレイヤーを追っているカメラ
{
	if (camera == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT2 currentPosition =
		camera->GetPosition();

	const DirectX::XMFLOAT3 playerPosition =
		owner->GetTransform()->GetPosition();

	// カメラがステージ端より外を映さないように、
	// プレイヤー位置をカメラの目標位置として使える範囲に制限する。
	const float minCameraX =
		mapMinX + viewWidth * 0.5f;
	const float maxCameraX =
		mapMaxX - viewWidth * 0.5f;

	const float minCameraY =
		mapMinY + viewHeight * 0.5f;
	const float maxCameraY =
		mapMaxY - viewHeight * 0.5f;

	const float targetX =
		std::clamp(playerPosition.x, minCameraX, maxCameraX);

	const float targetY =
		std::clamp(playerPosition.y, minCameraY, maxCameraY);

	// deltaTimeに応じて、カメラを目標位置へなめらかに近づける。
	const float followAmount =
		1.0f - std::exp(-followSharpness * deltaTime);

	const float nextX =
		currentPosition.x +
		(targetX - currentPosition.x) * followAmount;

	const float nextY =
		currentPosition.y +
		(targetY - currentPosition.y) * followAmount;

	// 補間後にもステージ範囲を越えないよう、もう一度制限する。
	camera->SetPosition(
		{
			std::clamp(nextX, minCameraX, maxCameraX),
			std::clamp(nextY, minCameraY, maxCameraY)
		});

	//if (!isTransitioning)
	//{
	//	const DirectX::XMFLOAT2 cameraPosition = camera->GetPosition();
	//	const DirectX::XMFLOAT3 playerPosition = owner->GetTransform()->GetPosition();

	//	float moveX = 0.0f;
	//	float moveY = 0.0f;

	//	const float halfWidth = viewWidth * 0.5f;
	//	const float halfHeight = viewHeight * 0.5f;

	//	if (playerPosition.x > cameraPosition.x + halfWidth)
	//	{
	//		moveX = 1.0f;
	//	}
	//	else if (playerPosition.x < cameraPosition.x - halfWidth)
	//	{
	//		moveX = -1.0f;
	//	}

	//	if (playerPosition.y > cameraPosition.y + halfHeight)
	//	{
	//		moveY = 1.0f;
	//	}
	//	else if (playerPosition.y < cameraPosition.y - halfHeight)
	//	{
	//		moveY = -1.0f;
	//	}

	//	//どの端も越えていなければ、カメラはそのまま
	//	if (moveX == 0.0f && moveY == 0.0f)
	//	{
	//		return;
	//	}

	//	transitionStart = cameraPosition;

	//	//次の固定カメラ位置を決める
	//	transitionTarget =
	//	{
	//		cameraPosition.x + moveX * viewWidth,
	//		cameraPosition.y + moveY * viewHeight
	//	};

	//	transitionTimer = 0.0f;
	//	isTransitioning = true;
	//}

	//if (transitionDuration <= 0.0f)
	//{
	//	camera->SetPosition(transitionTarget);
	//	isTransitioning = false;
	//	return;
	//}

	//transitionTimer = std::min(transitionTimer + deltaTime, transitionDuration);

	//float progress = transitionTimer / transitionDuration;

	////開始と終了をなめらかにする
	//progress = progress * progress * (3.0f - 2.0f * progress);

	//const DirectX::XMFLOAT2 cameraPosition =
	//{
	//	transitionStart.x + (transitionTarget.x - transitionStart.x) * progress,
	//	transitionStart.y + (transitionTarget.y - transitionStart.y) * progress
	//};

	//camera->SetPosition(cameraPosition);

	//if (transitionTimer >= transitionDuration)
	//{
	//	camera->SetPosition(transitionTarget);
	//	isTransitioning = false;
	//}
}

void CameraController::SnapToTarget()
{
	if (camera == nullptr)
	{
		return;
	}

	const DirectX::XMFLOAT3 playerPosition = owner->GetTransform()->GetPosition();

	const float minCameraX =
		mapMinX + viewWidth * 0.5f;
	const float maxCameraX =
		mapMaxX - viewWidth * 0.5f;

	const float minCameraY =
		mapMinY + viewHeight * 0.5f;
	const float maxCameraY =
		mapMaxY - viewHeight * 0.5f;

	// プレイヤー位置を中心にしつつ、ステージ端ではカメラを止める。
	camera->SetPosition(
		{
			std::clamp(playerPosition.x, minCameraX, maxCameraX),
			std::clamp(playerPosition.y, minCameraY, maxCameraY)
		});

	////プレイヤーを含む固定画面の場所の番号
	//const float cameraCellX = std::floor((playerPosition.x + viewWidth * 0.5f) / viewWidth);

	//const float cameraCellY = std::floor((playerPosition.y + viewHeight * 0.5f) / viewHeight);

	////固定画面の番号から固定カメラ位置を計算する
	//const DirectX::XMFLOAT2 cameraPosition =
	//{
	//	cameraCellX * viewWidth,
	//	cameraCellY * viewHeight
	//};

	//camera->SetPosition(cameraPosition);

	//transitionStart = cameraPosition;
	//transitionTarget = cameraPosition;
	//transitionTimer = 0.0f;
	//isTransitioning = false;
}