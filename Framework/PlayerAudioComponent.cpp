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

	//着地音の読み込み
	landSound = std::make_shared<WaveData>();
	if (!landSound->Load(L"../../assets/sounds/Land.wav",errorMessage))
	{
		OutputDebugStringA("Land.wavの読み込みに失敗しました\n");
		OutputDebugStringA(errorMessage.c_str());
		OutputDebugStringA("\n");

		landSound.reset();
	}
	//歩行音の読み込み
	walkSound = std::make_shared<WaveData>();
	if (!walkSound->Load(L"../../assets/sounds/Walk.wav", errorMessage))
	{
		OutputDebugStringA("Walk.wavの読み込みに失敗しました\n");
		OutputDebugStringA(errorMessage.c_str());
		OutputDebugStringA("\n");

		walkSound.reset();
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

void PlayerAudioComponent::PlayLand()
{
	// WAVの読み込みに失敗していたら再生しない
	if (landSound == nullptr || !landSound->IsLoaded())
	{
		return;
	}

	// 着地音をSEとして再生する
	owner->GetContext().soundManager.PlaySE(landSound, 0.9f);
}

void PlayerAudioComponent::UpdateWalk(float deltaTime, bool isWalking)
{
	// 歩いていない間はタイマーを戻して、足音を鳴らさない
	if (!isWalking || walkSound == nullptr || !walkSound->IsLoaded())
	{
		walkSoundTimer = 0.0f;
		return;
	}

	// 歩いている時間を加算する
	walkSoundTimer += deltaTime;

	// 足音の間隔に達していなければ、まだ鳴らさない
	if (walkSoundTimer < walkSoundInterval)
	{
		return;
	}

	// 次の足音に向けてタイマーを戻す
	walkSoundTimer = 0.0f;

	// 足音をSEとして再生する
	owner->GetContext().soundManager.PlaySE(walkSound, 0.48f);
}