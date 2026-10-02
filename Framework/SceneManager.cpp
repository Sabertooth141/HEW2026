#include "SceneManager.h"

#include "Camera2D.h"

SceneManager::~SceneManager()
{
	if (currScene)
	{
		currScene->Clear();
	}
}

void SceneManager::Register(const std::string& name, Factory factory)
{
	factories[name] = std::move(factory);
}

void SceneManager::RequestChange(const std::string& name, float inFadeTime)
{
	if (!IsRegistered(name))
	{
		assert(false && "SceneManager::RequestChange: unknown scene name");
		return;
	}

	if (IsTransitioning())
	{
		return;
	}

	fadeTime = std::max(inFadeTime, 0.001f);
	fadeTimer = 0.f;

	// if currScene not set == first scene -> just load and fade in
	if (!currScene)
	{
		SwitchToScene(name);
		transState = TransitionState::FADE_IN;
		return;
	}

	// if not fade out currscene
	nextSceneName = name;
	transState = TransitionState::FADE_OUT;
}

void SceneManager::Update(float deltaTime)
{
	if (!IsTransitioning())
	{
		return;
	}

	fadeTimer += std::min(deltaTime, 1.f / 30.f);
	if (fadeTimer < fadeTime)
	{
		return;
	}

	// if was fading out -> switch to next scene fade in
	// else -> switch to not in transition
	fadeTimer = 0.f;
	if (transState == TransitionState::FADE_OUT)
	{
		SwitchToScene(nextSceneName);
		transState = TransitionState::FADE_IN;
	}
	else
	{
		transState = TransitionState::NONE;
	}
}

float SceneManager::GetFadeAlpha() const
{
	const float timer = std::clamp(fadeTimer / fadeTime, 0.f, 1.f);

	// if fading out -> go from 0 to 1
	// if fading in -> from 1 to 0
	switch (transState)
	{
	case TransitionState::FADE_OUT:
		return timer;
	case TransitionState::FADE_IN:
		return 1.f - timer;
	default:
		return 0.f;
	}
}

void SceneManager::SwitchToScene(const std::string& inSceneName)
{
	// uninit current scene
	if (currScene)
	{
		currScene->OnExit();
		currScene->Clear();
		currScene.reset();
	}

	context.camera.SetPosition({0.f, 0.f});

	currScene = factories.at(inSceneName)(context);
	currName = inSceneName;

	context.gameScene = currScene.get();
	currScene->OnEnter();
}
