#pragma once

#include "MonoBehavior.h"

// 接触時のワープ先を保持する共通コンポーネント。
class WarpComponent : public MonoBehavior
{
public:
	DirectX::XMFLOAT3 GetTargetPosition() const
	{
		return targetPosition;
	}

	void SetTargetPosition(const DirectX::XMFLOAT3& position)
	{
		targetPosition = position;
	}

private:
	// インスペクターから設定するワールド座標。
	DirectX::XMFLOAT3 targetPosition = {};
};