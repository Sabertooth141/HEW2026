#pragma once
#include <DirectXMath.h>
#include <string>

#include "IComponent.h"

class Mouse;
class Keyboard;

class MonoBehavior : public IComponent
{
public:
	virtual void Awake()
	{
	}

	virtual void Start()
	{
	}

	void Update(float deltaTime) override
	{
	}

	virtual void LateUpdate(float deltaTime)
	{
	}

	virtual void OnDestroy()
	{
	}

	void SetInput(Keyboard& inKeyboard, Mouse& inMouse);

	bool IsStarted() const
	{
		return started;
	}

	bool IsEnabled() const
	{
		return enabled;
	}

	void SetStarted(const bool inStarted)
	{
		started = inStarted;
	}

	void SetEnabled(const bool inEnabled)
	{
		enabled = inEnabled;
	}

	GameObject* Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos) const;
	void Destroy(GameObject* object) const;
	void DestroySelf() const;

public:
	Keyboard* keyboard = nullptr;
	Mouse* mouse = nullptr;

protected:
	bool started = false;
	bool enabled = true;
};
