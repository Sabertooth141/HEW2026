#include "CameraController.h"

#include <algorithm>
#include <cmath>

#include "Camera2D.h"
#include "GameObject.h"
#include "WindowSettings.h"

namespace
{
	struct Room
	{
		float minX;
		float maxX;
		float minY;
		float maxY;
	};

	//背景画像の配置に対応する部屋の範囲
	constexpr Room rooms[] =
	{
		{ -960.f,  960.f,  -540.f,  540.f }, // 部屋1
		{ 960.f, 2880.f,  -540.f,  540.f }, // 部屋2
		{ 2880.f, 4800.f,  -540.f,  540.f }, // 部屋3
		{ 2880.f, 4800.f, -1620.f, -540.f }, // 部屋4
		{ 4800.f, 6720.f, -1620.f, -540.f }, // 部屋5
		{ 4800.f, 6720.f,  -540.f,  540.f }, // 部屋6
		{ 6720.f, 8640.f,  -540.f,  540.f }, // 部屋7
		{ 6720.f, 8640.f, -1620.f, -540.f }  // 部屋8
	};

	constexpr int roomCount = sizeof(rooms) / sizeof(rooms[0]);

	float ClampAxis(float value, float minimum, float maximum, float viewSize)
	{
		//表示範囲より小さい部屋では中央に固定
		if (maximum - minimum <= viewSize)
		{
			return (minimum + maximum) * 0.5f;
		}

		return std::clamp(
			value,
			minimum + viewSize * 0.5f,
			maximum - viewSize * 0.5f
		);
	}
}

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

int CameraController::FindRoom(const DirectX::XMFLOAT3& position) const
{
	for (int i = 0;i < roomCount;++i)
	{
		const Room& room = rooms[i];

		//共有する教会で所属する部屋が重ならないようにする
		if (position.x >= room.minX && position.x < room.maxX && position.y >= room.minY && position.y < room.maxY)
		{
			return i;
		}
	}

	return -1;
}

DirectX::XMFLOAT2 CameraController::ClampToRoom(DirectX::XMFLOAT2 position, int roomIndex)const
{
	const Room& room = rooms[roomIndex];

	return
	{
		ClampAxis(position.x,room.minX,room.maxX,viewWidth),
		ClampAxis(position.y,room.minY,room.maxY,viewHeight)
	};
}

void CameraController::LateUpdate(float deltaTime)
{
	if (camera == nullptr)
	{
		return;
	}

	if (currentRoom < 0)
	{
		SnapToTarget();
		return;
	}

	const auto playerPosition = owner->GetTransform()->GetPosition();
	const float dt = (std::max)(0.0f, deltaTime);

	if (!isTransitioning)
	{
		const int nextRoom = FindRoom(playerPosition);

		// 未配置の領域に出た場合は現在の部屋を維持する。
		if (nextRoom >= 0 && nextRoom != currentRoom)
		{
			transitionStart = camera->GetPosition();
			currentRoom = nextRoom;

			transitionTarget = ClampToRoom(
				{ playerPosition.x, playerPosition.y },
				currentRoom);

			transitionTimer = 0.0f;
			isTransitioning = true;
		}
	}

	if (isTransitioning)
	{
		// 切り替え中は通常の追従処理を実行しない。
		if (transitionDuration <= 0.0f)
		{
			camera->SetPosition(transitionTarget);
			isTransitioning = false;
			return;
		}

		transitionTimer = (std::min)(
			transitionTimer + dt, transitionDuration);

		float progress = transitionTimer / transitionDuration;

		// スクロール開始・終了を滑らかにする。
		progress = progress * progress * (3.0f - 2.0f * progress);

		camera->SetPosition(
			{
				transitionStart.x +
					(transitionTarget.x - transitionStart.x) * progress,
				transitionStart.y +
					(transitionTarget.y - transitionStart.y) * progress
			});

		if (transitionTimer >= transitionDuration)
		{
			camera->SetPosition(transitionTarget);
			isTransitioning = false;
		}

		return;
	}

	// 通常時は、現在の部屋の中だけでプレイヤーを追う。
	const auto target = ClampToRoom(
		{ playerPosition.x, playerPosition.y },
		currentRoom);

	const auto current = camera->GetPosition();
	const float amount = 1.0f - std::exp(-followSharpness * dt);

	camera->SetPosition(ClampToRoom(
		{
			current.x + (target.x - current.x) * amount,
			current.y + (target.y - current.y) * amount
		}, currentRoom));
}

void CameraController::SnapToTarget()
{
	if (camera == nullptr)
	{
		return;
	}

	const auto position = owner->GetTransform()->GetPosition();
	currentRoom = FindRoom(position);

	//復帰位置が部屋の外なら、一番近い部屋へ
	if (currentRoom < 0)
	{
		float nearestDistance = (std::numeric_limits<float>::max)();

		for (int i = 0;i < roomCount;++i)
		{
			const Room& room = rooms[i];
			
			const float dx = position.x - std::clamp(position.x, room.minX, room.maxX);

			const float dy = position.y - std::clamp(position.y, room.minY, room.maxY);

			const float distance = dx * dx + dy * dy;

			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				currentRoom = i;
			}
		}
	}

	const auto target = ClampToRoom({ position.x,position.y }, currentRoom);

	camera->SetPosition(target);

	transitionStart = target;
	transitionTarget = target;
	transitionTimer = 0.0f;
	isTransitioning = false;
}