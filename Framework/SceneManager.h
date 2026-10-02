#pragma once
#include <functional>
#include <memory>

#include "Scene.h"

class SceneManager
{
public:
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

	void RequestChange(const std::string& name, float inFadeTime = 0.4f);

	void Update(float deltaTime);
	
	Scene* GetCurrScene() const
	{
		return currScene.get();
	}

	const std::string& GetCurrSceneName() const
	{
		return currName;
	}

	bool IsTransitioning() const
	{
		return transState != TransitionState::NONE;
	}

	// 0 = clear; 1 = fully black
	float GetFadeAlpha() const;

private:
	void SwitchToScene(const std::string& inSceneName);

private:
	enum class TransitionState
	{
		NONE,
		FADE_OUT,
		FADE_IN
	};

	GameContext& context;
	std::unordered_map<std::string, Factory> factories;

	std::unique_ptr<Scene> currScene;
	std::string currName;

	TransitionState transState = TransitionState::NONE;
	std::string nextSceneName;
	float fadeTime = 0.f;
	float fadeTimer = 0.f;
};
