#pragma once

#include <algorithm>

#include "MonoBehavior.h"

//スタミナの所持量、消費、回復を管理（仮で作ったものなので消していいです、消した場合は加速装置の方の修正しといてください）
class StaminaComponent :public MonoBehavior
{
public:
	//スタミナの最大量を設定
	void SetMaxStamina(float amount)
	{
		maxStamina = std::max(0.0f, amount);
		currentStamina = std::min(currentStamina, maxStamina);
	}
	
	//ダッシュなどのアクションに必要な最低スタミナ量
	void SetMinimumActivationAmount(float amount)
	{
		minimumActivationAmount = std::max(0.0f, amount);
	}

	//通常時の一秒あたりの回復量
	void SetRecoveryPerSecond(float amount)
	{
		recoveryPerSecond = std::max(0.0f, amount);
	}

	float GetCurrentStamina() const
	{
		return currentStamina;
	}

	float GetMaxStamina() const
	{
		return maxStamina;
	}

	float GetRatio() const
	{
		if (maxStamina <= 0.0f)
		{
			return 0.0f;
		}

		return std::clamp(currentStamina / maxStamina, 0.0f, 1.0f);
	}

	bool IsEmpty() const
	{
		return currentStamina <= 0.0f;
	}

	//スタミナの残量がダッシュなどが可能な最低量に達しているか
	bool CanStartAction() const
	{
		return currentStamina >= minimumActivationAmount;
	}

	//ジャンプ、スライディングなど、一回分の消費
	bool TryConsume(float amount)
	{
		if (amount < -0.0f || !CanStartAction() || currentStamina < amount)
		{
			return false;
		}

		currentStamina = std::max(0.0f, currentStamina - amount);
		return true;
	}

	// ダッシュ中など、継続的な消費に使う。
	// 消費後にスタミナが残っていれば true、0なら false。
	bool Drain(float amount)
	{
		if (amount > 0.0f)
		{
			currentStamina =
				std::max(0.0f, currentStamina - amount);
		}

		return !IsEmpty();
	}

	// チェックポイントや加速装置などから直接回復する。
	void Recover(float amount)
	{
		if (amount <= 0.0f)
		{
			return;
		}

		currentStamina =
			std::min(maxStamina, currentStamina + amount);
	}

	// 自然回復を有効・無効にする。
	// 後でプレイヤー側から「ダッシュしていない間だけ有効」に設定する。
	void SetRecoveryEnabled(bool enabled)
	{
		isRecoveryEnabled = enabled;
	}

	void Update(float deltaTime) override
	{
		if (!isRecoveryEnabled || deltaTime <= 0.0f)
		{
			return;
		}

		Recover(recoveryPerSecond * deltaTime);
	}

private:
	float maxStamina = 100.0f;
	float currentStamina = 100.0f;

	float minimumActivationAmount = 10.0f;
	float recoveryPerSecond = 20.0f;

	bool isRecoveryEnabled = true;
};
