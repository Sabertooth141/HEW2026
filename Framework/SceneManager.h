#pragma once
#include <functional>
#include <memory>

#include "Scene.h"

class SceneManager
{
public:
	enum class TransitionState
	{
		NONE,
		EXITING,
		ENTERING
	};

	using Factory = std::function<std::unique_ptr<Scene>(GameContext&)>;

	explicit SceneManager(GameContext& inContext) : context(inContext)
	{
	}

	~SceneManager();
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	void Register(const std::string& name, Factory factory);

	template <typename T>
	void Register(const std::string& name)
	{
		Register(name, [](GameContext& context) { return std::make_unique<T>(context); });
	}

	bool IsRegistered(const std::string& name) const
	{
		return factories.contains(name);
	}

	void RequestChange(const std::string& name, float exitTime = 0.f, float enterTime = 0.f);

	void Update(float deltaTime);

	Scene* GetCurrScene() const
	{
		return currScene.get();
	}

	const std::string& GetCurrSceneName() const
	{
		return currName;
	}

	const std::string& GetNextSceneName() const
	{
		return nextSceneName;
	}

	TransitionState GetTransitionState() const
	{
		return transState;
	}

	bool IsTransitioning() const
	{
		return transState != TransitionState::NONE;
	}

	// 0 -> 1 
	float GetTransitionProgress() const;

	std::vector<std::string> GetSceneNames() const;

private:
	void SwitchToScene(const std::string& inSceneName);
	void AdvancePhase();
	float GetPhaseDuration() const;

private:
	GameContext& context;
	// シーンネーム　と　シーンコンストラクタ
	std::unordered_map<std::string, Factory> factories;

	std::unique_ptr<Scene> currScene;
	std::string currName;

	TransitionState transState = TransitionState::NONE;
	std::string nextSceneName;
	float exitTime = 0.f;
	float enterTime = 0.f;
	float phaseTimer = 0.f;
};
