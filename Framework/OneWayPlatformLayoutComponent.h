
#pragma once

#include <vector>

#include "GameObject.h"
#include "MonoBehavior.h"

struct OneWayPlatformData
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scale;
};

class OneWayPlatformLayoutComponent : public MonoBehavior
{
public:
    void Start() override
    {
        for (const OneWayPlatformData& data : platformDataList)
        {
            GameObject* object =
                Instantiate("one_way_platform", data.position);

            if (object == nullptr)
            {
                continue;
            }

            object->SetName("OneWayPlatform");
            object->GetTransform()->SetScale(data.scale);
        }
    }

private:
    // 片方向足場の配置
    const std::vector<OneWayPlatformData> platformDataList =
    {
        // ステージ中央付近のテスト用足場
        OneWayPlatformData{
            { 500.f, 100.f, 1.f },
            { 300.f, 30.f, 1.f }
        }
    };
};

