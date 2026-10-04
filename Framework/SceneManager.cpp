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

void SceneManager::RequestChange(const std::string& name, const float inExitTime, const float inEnterTime)
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

	exitTime = std::max(inExitTime, 0.f);
	enterTime = std::max(inEnterTime, 0.f);
	phaseTimer = 0.f;
	nextSceneName = name;

	// first scene: nothing to exit from
	if (!currScene)
	{
		SwitchToScene(name);
		transState = enterTime > 0.f ? TransitionState::ENTERING : TransitionState::NONE;
		return;
	}

	// the actual switch always happens in Update() (end of frame), even with exitTime = 0
	transState = TransitionState::EXITING;
}

void SceneManager::Update(float deltaTime)
{
	if (!IsTransitioning())
	{
		return;
	}

	// scene loading spikes dt; clamp so a phase isn't skipped in one frame
	phaseTimer += std::min(deltaTime, 1.f / 30.f);
	if (phaseTimer < GetPhaseDuration())
	{
		return;
	}

	AdvancePhase();
}

void SceneManager::AdvancePhase()
{
	phaseTimer = 0.f;

	if (transState == TransitionState::EXITING)
	{
		SwitchToScene(nextSceneName);
		transState = enterTime > 0.f ? TransitionState::ENTERING : TransitionState::NONE;
	}
	else
	{
		transState = TransitionState::NONE;
	}
}

float SceneManager::GetPhaseDuration() const
{
	switch (transState)
	{
	case TransitionState::EXITING:
		return exitTime;
	case TransitionState::ENTERING:
		return enterTime;
	default:
		return 0.f;
	}
}

float SceneManager::GetTransitionProgress() const
{
	const float duration = GetPhaseDuration();
	if (duration <= 0.f)
	{
		return IsTransitioning() ? 1.f : 0.f;
	}
	return std::clamp(phaseTimer / duration, 0.f, 1.f);
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
