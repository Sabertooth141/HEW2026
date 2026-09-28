#pragma once

class GameObject;
class Renderer;
class Collider2D;

class IComponent
{
public:
	virtual ~IComponent() = default;

	virtual void Update(float deltaTime)
	{
	}

	virtual void OnCollisionEnter2D(const GameObject& other)
	{
	}

	virtual void OnCollisionStay2D(const GameObject& other)
	{
	}

	virtual void OnCollisionExit2D(const GameObject& other)
	{
	}

public:
	GameObject* owner = nullptr;
};
