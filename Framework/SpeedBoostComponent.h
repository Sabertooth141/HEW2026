#pragma once

#include <algorithm>
#include <unordered_set>

#include "MonoBehavior.h"

class SpeedBoostComponent :public MonoBehavior
{
public:
	void Update(float deltaTime) override
	{
		if (isPaused || remainingTime <= 0.f)
		{
			return;
		}

		remainingTime = std::max(0.f, remainingTime - deltaTime);

		if (remainingTime <= 0.f)
		{
			ResetBoost();
		}
	}

	void AddStack()
	{
		++stackCount;
		remainingTime = boostDuration;
	}

	bool TryAddStack(const GameObject* checkpoint)
	{
		if (checkpoint == nullptr)
		{
			return false;
		}

		// 加速時間が切れていたら、同じチェックポイントで再加速可能
		if (remainingTime <= 0.f)
		{
			stackCount = 0;
			usedCheckpoints.clear();
		}

		// 現在の制限時間内ですでに使ったチェックポイントなら追加しない
		const auto [iterator, wasInserted] =
			usedCheckpoints.insert(checkpoint);

		if (!wasInserted)
		{
			return false;
		}

		++stackCount;
		remainingTime = boostDuration;
		return true;
	}

	float GetMoveSpeed(float baseSpeed) const
	{
		if (remainingTime <= 0.0f)
		{
			return baseSpeed;
		}

		return baseSpeed * (1.0f + stackCount * speedIncreasePerStack);
	}

	int GetStackCount() const
	{
		return stackCount;
	}

	enum class BoostAbility
	{
		WallClimb // 壁登り
	};

	// スタック数に応じて能力が使用可能か確認する
	bool HasAbility(BoostAbility ability) const
	{
		// 加速時間が切れていたら能力は使えない
		if (remainingTime <= 0.f)
		{
			return false;
		}

		switch (ability)
		{
		case BoostAbility::WallClimb:
			return stackCount >= wallClimbRequiredStacks;
		}

		return false;
	}

	float GetRemainingTime() const
	{
		return remainingTime;
	}

	void SetPaused(bool inPaused)
	{
		isPaused = inPaused;
	}

	//死亡時に加速の制限時間、スタックリセット
	void ResetBoost()
	{
		stackCount = 0;
		remainingTime = 0.f;
	}



private:
	int stackCount = 0;
	float  remainingTime = 0.0f;

	float boostDuration = 5.0f;
	float speedIncreasePerStack = 0.15f;

	bool isPaused = false;

	std::unordered_set<const GameObject*> usedCheckpoints;

	// 壁登りを解放するために必要なスタック数
	int wallClimbRequiredStacks = 2;
};