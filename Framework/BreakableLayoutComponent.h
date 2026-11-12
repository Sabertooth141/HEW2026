
#pragma once

#include <vector>

#include "GameObject.h"
#include "MonoBehavior.h"

struct BreakableData
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 scale;
};

class BreakableLayoutComponent : public MonoBehavior
{
public:
    void Start() override
    {
        // 壊れる床を生成
        for (const BreakableData& data : breakableFloorDataList)
        {
            GameObject* object = Instantiate("breakable_floor", data.position);

            if (object == nullptr)
            {
                continue;
            }

            object->SetName("BreakableFloor");
            object->GetTransform()->SetScale(data.scale);
        }

        // 壊れる壁を生成
        for (const BreakableData& data : breakableWallDataList)
        {
            GameObject* object = Instantiate("breakable_wall", data.position);

            if (object == nullptr)
            {
                continue;
            }

            object->SetName("BreakableWall");
            object->GetTransform()->SetScale(data.scale);
        }
    }

private:
    // 壊れる床の配置
    const std::vector<BreakableData> breakableFloorDataList =
    {
        BreakableData{
            { 0.f, -80.f, 1.f },
            { 240.f, 40.f, 1.f }
        }
    };

    // 壊れる壁の配置
    const std::vector<BreakableData> breakableWallDataList =
    {
        BreakableData{
            { -1250.f, -80.f, 1.f },
            { 40.f, 20.f, 1.f }
        }
    };
};
