#pragma once
#include <memory>
#include <string>
#include <vector>
#include <DirectXMath.h>

#include "GameObject.h"
#include "GameContext.h"

class Scene
{
public:
    explicit Scene(GameContext& inCtx) : context(inCtx) {}

    GameObject* Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos);
    GameObject* Add2DObject();
    void Destroy(GameObject* object);

    void Update(float deltaTime);
    void FlushPending();

    std::vector<std::unique_ptr<GameObject>>& GetObjects() { return objects; }

private:
    GameContext& context;
    std::vector<std::unique_ptr<GameObject>> objects;
    std::vector<std::unique_ptr<GameObject>> pendingSpawn;
    std::vector<GameObject*> pendingDestroy;
};