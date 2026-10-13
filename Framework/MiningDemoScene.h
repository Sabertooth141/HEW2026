#pragma once
#include "Scene.h"

// Optional, isolated demonstration of the ore module. Existing game scenes are unchanged.
class MiningDemoScene : public Scene
{
public:
    using Scene::Scene;
    void OnEnter() override;
    void OnUpdate(float deltaTime) override;
    void OnDrawUI(TextRenderer& text) override;

    GameObject* GetPlayerObject() const { return playerObj; }
    GameObject* GetHeldEnemy();
    GameObject* GetStageEnemy();
    GameObject* GetEnemyObject() { return GetStageEnemy(); }
    int GetExperience() const;
    GameObject* GetOreObject(int index);
    bool ToggleHammer();
    bool StartHammerSwing();
    void MovePlayerTo(const DirectX::XMFLOAT3& position);

private:
    void UpdateHeldPosition();
    GameObject* playerObj = nullptr;
    GameObject* heldEnemy = nullptr;
    float facing = 1.f;
    float swingTimer = 0.f;
};
