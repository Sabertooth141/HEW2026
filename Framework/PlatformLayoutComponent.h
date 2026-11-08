#pragma once

#include <array>
#include <vector>

#include "BoxCollider2D.h"
#include "GameObject.h"
#include "MonoBehavior.h"
#include "Rigidbody2DComponent.h"
#include "Scene.h"

struct PlatformData
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
};

class PlatformLayoutComponent :public MonoBehavior
{
public:
	void Start() override
	{
		for (const PlatformData& platformData : platformDataList)
		{
			CreatePlatform(platformData);
		}
	}

private:
	void CreatePlatform(const PlatformData& platformData)
	{
		GameObject* platformObj = owner->GetContext().gameScene->Add2DObject();
		if (platformObj == nullptr)
		{
			return;
		}

		platformObj->SetName("Platform");
		platformObj->SetTag(ObjectTag::GROUND);
		platformObj->GetTransform()->SetPosition(platformData.position);
		platformObj->GetTransform()->SetScale(platformData.scale);

		auto& platformRb = platformObj->AddComponent<Rigidbody2DComponent>(*platformObj->GetTransform(), 1.0f);
		platformRb.SetGravity(0.0f);
		platformRb.SetIsStatic(true);

		platformObj->AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.5f, 0.5f),
			DirectX::XMFLOAT2(0.0f, 0.0f),
			false,
			*platformObj->GetTransform());

		platformObjects.push_back(platformObj);
	}

	const std::array<PlatformData, 5> platformDataList =
	{
		PlatformData{ { 1.5f, -240.f, 1.f },{ 3000.f, 80.f, 1.f } }, // 地面
		PlatformData{ { 1510.f, -30.f, 1.f },{ 90.f, 900.f, 1.f } }, // 右側の壁
		PlatformData{ { 1330.f, 400.f, 1.f },{ 320.f, 40.f, 1.f } }, // 右側の天井

		PlatformData{ { -1510.f, -30.f, 1.f },{ 90.f, 900.f, 1.f } }, // 左側の壁
		PlatformData{ { -1700.f, 400.f, 1.f },{ 320.f, 40.f, 1.f } }  // 左側の天井
	};

	std::vector<GameObject*> platformObjects;
};