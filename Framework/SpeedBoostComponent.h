
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <unordered_set>

#include "MonoBehavior.h"

class SpeedBoostComponent : public MonoBehavior
{
public:
	enum class BoostAbility
	{
		WallClimb,  // 壁のぼり
		BreakWall,  // 壁破壊
		Dive,        // 急降下
		AirRun      // 空中走行
	};

	static constexpr int MaxAbilityStack = 4;

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

	// 指定したスタック数で能力が有効か確認する
	bool IsAbilityEnabledForStack(
		int targetStack,
		BoostAbility ability) const
	{
		if (targetStack < 1 || targetStack > MaxAbilityStack)
		{
			return false;
		}

		const int abilityIndex = static_cast<int>(ability);

		return abilitySettings[targetStack - 1][abilityIndex];
	}

	// 指定したスタック数の能力設定を変更する
	void SetAbilityEnabled(
		int targetStack,
		BoostAbility ability,
		bool enabled)
	{
		if (targetStack < 1 || targetStack > MaxAbilityStack)
		{
			return;
		}

		const int abilityIndex = static_cast<int>(ability);

		abilitySettings[targetStack - 1][abilityIndex] = enabled;
	}

	// 現在のスタック数で能力が使用可能か確認する
	bool HasAbility(BoostAbility ability) const
	{
		// 加速時間が切れていたら能力は使えない
		if (remainingTime <= 0.f || stackCount <= 0)
		{
			return false;
		}

		// 4スタック以上の場合は3スタックの設定を使用する
		const int currentStack =
			std::min(stackCount, MaxAbilityStack);

		return IsAbilityEnabledForStack(currentStack, ability);
	}

	float GetRemainingTime() const
	{
		return remainingTime;
	}

	void SetPaused(bool inPaused)
	{
		isPaused = inPaused;
	}

	// 死亡時に加速の制限時間、スタックをリセット
	void ResetBoost()
	{
		stackCount = 0;
		remainingTime = 0.f;
	}

private:
	int stackCount = 0;
	float remainingTime = 0.0f;

	float boostDuration = 5.0f;
	float speedIncreasePerStack = 0.15f;

	bool isPaused = false;

	std::unordered_set<const GameObject*> usedCheckpoints;

	// 各スタック数で使用できる能力を保存する
	// 行がスタック数
	// 列は壁のぼり、壁破壊、急降下、空中走行
		std::array<std::array<bool, 4>, 4> abilitySettings =
	{ {
		{ true,  false, false, false }, // 1スタック
		{ true,  true,  false, false }, // 2スタック
		{ true,  true,  true,  false },
		{ true,  true,  true,  false }// 3スタック

	} };
};
