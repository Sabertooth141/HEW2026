#pragma once
#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "GameObject.h"
#include "GameContext.h"

class TextRenderer;

class Scene
{
public:
	explicit Scene(GameContext& inCtx) : context(inCtx)
	{
	}

	virtual ~Scene() = default;
	Scene& operator=(const Scene&) = delete;

	GameObject* Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos);
	GameObject* Add2DObject();
	void Destroy(GameObject* object);

	virtual void OnEnter()
	{
	}

	virtual void OnExit()
	{
	}

	virtual void Update(float deltaTime);

	virtual void OnDrawUI(TextRenderer& text)
	{
	}

	void Clear();

	void FlushPending();

	std::vector<std::unique_ptr<GameObject>>& GetObjects()
	{
		return objects;
	}

protected:
	GameContext& context;

private:
	void UnregisterFromSys(GameObject* obj);

	std::vector<std::unique_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<GameObject>> pendingSpawn;
	std::vector<GameObject*> pendingDestroy;
	bool isClearing = false;
};
