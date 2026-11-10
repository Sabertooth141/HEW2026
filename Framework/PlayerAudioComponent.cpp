#include "PlayerAudioComponent.h"

#include "GameObject.h"
#include "SoundManager.h"
#include "WaveData.h"

#include <Windows.h>

void PlayerAudioComponent::Awake()
{
	MonoBehavior::Awake();

	// ジャンプ音を読み込む
	jumpSound = std::make_shared<WaveData>();

	std::string errorMessage;
	if (!jumpSound->Load(L"../../assets/sounds/Jump.wav", errorMessage))
	{
		//読み込み失敗時ログに出す
		OutputDebugStringA("Jump.wavの読み込みに失敗しました");
		OutputDebugStringA(errorMessage.c_str());
		OutputDebugStringA("\n");

		jumpSound.reset();
	}
}

void PlayerAudioComponent::PlayJump()
{
	//読み込みに失敗していたら再生しない
	if (jumpSound == nullptr||!jumpSound->IsLoaded())
	{
		return;
	}

	//SoundManagerに音声をわたして再生する
	owner->GetContext().soundManager.PlaySE(jumpSound, 0.8f);
}