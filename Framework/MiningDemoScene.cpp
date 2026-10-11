#include "MiningDemoScene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

#include "OreDemoEnemyController.h"
#include "OreExperienceReceiver.h"
#include "Camera2D.h"
#include "HammerContactComponent.h"
#include "Keyboard.h"
#include "OreController.h"
#include "SceneManager.h"
#include "TextRenderer.h"

namespace
{
    constexpr std::array<float, 3> OreX{ -220.f, 0.f, 220.f };
    constexpr std::array<const char*, 3> OrePrefabs{ "ore_gray", "ore_blue", "ore_purple" };
    constexpr std::array<OreType, 3> OreTypes{ OreType::Gray, OreType::LightBlue, OreType::Purple };
    constexpr std::array<const char*, 3> OreLabels{ "灰色", "水色", "紫色" };
    constexpr float SwingDuration = 0.28f;
    constexpr float HammerRadius = 72.f;
}

void MiningDemoScene::OnEnter()
{
    context.camera.SetPosition({ 0.f, 0.f });
    heldEnemy = nullptr;
    facing = 1.f;
    swingTimer = 0.f;

    playerObj = Add2DObject();
    playerObj->SetName("mining_player");
    playerObj->SetTag(ObjectTag::PLAYER);
    playerObj->AddComponent<OreExperienceReceiver>();
    playerObj->GetTransform()->SetPosition({ -90.f, -155.f, 1.f });
    auto& animation = playerObj->AddComponent<AnimatorComponent>(context.renderer);
    animation.SetRenderLayer(RenderLayer::Player);
    animation.SetStatic(L"../../assets/ore/collector.png");
    playerObj->GetTransform()->SetScale({ 56.f, 56.f, 1.f });
    auto& body = playerObj->AddComponent<Rigidbody2DComponent>(*playerObj->GetTransform(), 1.f);
    body.SetGravity(0.f);
    body.SetFreezeRotation(true);
    playerObj->AddComponent<BoxCollider2D>(DirectX::XMFLOAT2{ 0.38f, 0.38f },
        DirectX::XMFLOAT2{ 0.f, 0.f }, true, *playerObj->GetTransform());

    for (int i = 0; i < 3; ++i)
        Instantiate(OrePrefabs[i], { OreX[i], 50.f, 1.f });
    Instantiate("ore_demo_enemy", { -10.f, -155.f, 1.f });
    FlushPending();
}

GameObject* MiningDemoScene::GetStageEnemy()
{
    for (auto& object : GetObjects())
    {
        auto* enemy = object->GetComponent<OreDemoEnemyController>();
        if (object->GetTag() == ObjectTag::ENEMY && enemy && !enemy->IsDead())
            return object.get();
    }
    return nullptr;
}

int MiningDemoScene::GetExperience() const
{
    // The original framework exposes only a mutable object-list accessor.
    // Read the live receiver instead of adding fields to GlobalContext.
    for (const auto& object : const_cast<MiningDemoScene*>(this)->GetObjects())
    {
        if (object.get() != playerObj) continue;
        const auto* receiver = object->GetComponent<OreExperienceReceiver>();
        return receiver ? receiver->GetTotal() : 0;
    }
    return 0;
}

GameObject* MiningDemoScene::GetHeldEnemy()
{
    for (auto& object : GetObjects())
    {
        if (object.get() != heldEnemy) continue;
        auto* enemy = object->GetComponent<OreDemoEnemyController>();
        if (object->GetTag() == ObjectTag::ENEMY && enemy && !enemy->IsDead() &&
            object->GetComponent<HammerContactComponent>()) return object.get();
    }
    heldEnemy = nullptr;
    swingTimer = 0.f;
    return nullptr;
}

GameObject* MiningDemoScene::GetOreObject(int index)
{
    if (index < 0 || index >= 3) return nullptr;
    for (auto& object : GetObjects())
    {
        auto* ore = object->GetComponent<OreController>();
        if (ore && !ore->IsDestroyed() &&
            ore->GetOreType() == OreTypes[index]) return object.get();
    }
    return nullptr;
}

bool MiningDemoScene::ToggleHammer()
{
    if (auto* held = GetHeldEnemy())
    {
        held->GetComponent<HammerContactComponent>()->SetHammerState(false);
        held->GetTransform()->SetRotation({ 0.f, 0.f, 0.f });
        heldEnemy = nullptr;
        swingTimer = 0.f;
        return true;
    }
    const auto position = playerObj->GetTransform()->GetPosition();
    GameObject* nearest = nullptr;
    float nearestDistance = 100.f * 100.f;
    for (auto& object : GetObjects())
    {
        auto* enemy = object->GetComponent<OreDemoEnemyController>();
        auto* hammer = object->GetComponent<HammerContactComponent>();
        if (object->GetTag() != ObjectTag::ENEMY || !enemy || enemy->IsDead() || !hammer) continue;
        const auto target = object->GetTransform()->GetPosition();
        const float dx = target.x - position.x, dy = target.y - position.y;
        const float distance = dx * dx + dy * dy;
        if (distance <= nearestDistance) { nearest = object.get(); nearestDistance = distance; }
    }
    if (!nearest) return false;
    heldEnemy = nearest;
    heldEnemy->GetComponent<HammerContactComponent>()->SetHammerState(true);
    UpdateHeldPosition();
    return true;
}

bool MiningDemoScene::StartHammerSwing()
{
    auto* held = GetHeldEnemy();
    if (!held || swingTimer > 0.f) return false;
    held->GetComponent<HammerContactComponent>()->BeginSwing();
    swingTimer = SwingDuration;
    UpdateHeldPosition();
    return true;
}

void MiningDemoScene::MovePlayerTo(const DirectX::XMFLOAT3& position)
{
    playerObj->GetTransform()->SetPosition({
        std::clamp(position.x, -360.f, 360.f), std::clamp(position.y, -190.f, 145.f), 1.f });
    playerObj->GetComponent<Rigidbody2DComponent>()->Wake();
    UpdateHeldPosition();
}

void MiningDemoScene::UpdateHeldPosition()
{
    auto* held = GetHeldEnemy();
    if (!held) return;
    const auto position = playerObj->GetTransform()->GetPosition();
    const float progress = 1.f - swingTimer / SwingDuration;
    const float angle = swingTimer > 0.f ? -0.9f + progress * 1.8f : 0.f;
    held->GetTransform()->SetPosition({ position.x + facing * HammerRadius * std::cos(angle),
        position.y + HammerRadius * std::sin(angle), 1.f });
    held->GetTransform()->SetRotation({ 0.f, 0.f, swingTimer > 0.f ? facing * angle : 0.f });
    held->GetComponent<Rigidbody2DComponent>()->Wake();
}

void MiningDemoScene::OnUpdate(float deltaTime)
{
    const float dt = std::clamp(deltaTime, 0.f, 0.05f);
    auto& keyboard = context.keyboard;
    if (keyboard.KeyIsTriggered(VK_ESCAPE)) { context.sceneManager->RequestChange("Title"); return; }
    if (keyboard.KeyIsTriggered('R')) { context.sceneManager->RequestChange("OreDemo"); return; }

    if (swingTimer > 0.f)
    {
        swingTimer = (std::max)(0.f, swingTimer - dt);
        if (swingTimer == 0.f)
            if (auto* held = GetHeldEnemy()) held->GetComponent<HammerContactComponent>()->EndSwing();
    }
    float x = (keyboard.KeyIsPressed('D') ? 1.f : 0.f) - (keyboard.KeyIsPressed('A') ? 1.f : 0.f);
    float y = (keyboard.KeyIsPressed('W') ? 1.f : 0.f) - (keyboard.KeyIsPressed('S') ? 1.f : 0.f);
    const float length = std::sqrt(x * x + y * y);
    if (length > 0.f)
    {
        if (x != 0.f) facing = x > 0.f ? 1.f : -1.f;
        const auto position = playerObj->GetTransform()->GetPosition();
        MovePlayerTo({ position.x + x / length * 205.f * dt,
            position.y + y / length * 205.f * dt, 1.f });
    }
    if (keyboard.KeyIsTriggered('F')) ToggleHammer();
    if (keyboard.KeyIsTriggered(VK_SPACE)) StartHammerSwing();
    UpdateHeldPosition();
}

void MiningDemoScene::OnDrawUI(TextRenderer& text)
{
    text.DrawScreen("鉱石モジュール 独立デモ", { 400.f, 34.f }, TextColor::White, 1.5f, TextAlign::Center);
    text.DrawScreen("独立した実演用ステージ / 敵をハンマーにして鉱石を叩きます", { 400.f, 70.f },
        TextColor::Cyan, 0.85f, TextAlign::Center);
    text.DrawScreen(std::format("経験値  {}", GetExperience()), { 28.f, 110.f }, TextColor::Yellow, 1.1f);
    if (auto* enemyObject = GetStageEnemy())
    {
        auto* enemy = enemyObject->GetComponent<OreDemoEnemyController>();
        text.DrawScreen(std::format("敵 HP {} / {}", enemy->GetHP(), enemy->GetMaxHP()), { 772.f, 110.f },
            TextColor::White, 1.1f, TextAlign::Right);
    }
    else text.DrawScreen("敵 HP 0  /  R で再開", { 772.f, 110.f }, TextColor::Red, 1.f, TextAlign::Right);
    bool allBroken = true;
    for (int i = 0; i < 3; ++i)
    {
        const float x = 400.f + OreX[i];
        text.DrawScreen(OreLabels[i], { x, 158.f }, TextColor::White, 1.f, TextAlign::Center);
        auto* object = GetOreObject(i);
        auto* ore = object ? object->GetComponent<OreController>() : nullptr;
        if (ore)
        {
            allBroken = false;
            text.DrawScreen(std::format("HP {} / {}", ore->GetHP(), ore->GetMaxHP()),
                { x, 335.f }, TextColor::White, 0.95f, TextAlign::Center);
        }
        else text.DrawScreen("破壊済み", { x, 335.f }, TextColor::Green, 0.95f, TextAlign::Center);
    }
    const auto playerPosition = playerObj->GetTransform()->GetPosition();
    text.DrawWorld("プレイヤー", { playerPosition.x, playerPosition.y + 42.f }, TextColor::Cyan, 0.7f, TextAlign::Center);
    if (auto* held = GetHeldEnemy())
    {
        const auto position = held->GetTransform()->GetPosition();
        text.DrawWorld("ハンマー状態", { position.x, position.y + 36.f }, TextColor::Yellow, 0.7f, TextAlign::Center);
    }
    text.DrawScreen(allBroken ? "全鉱石を破壊！ 落ちた経験値に触れて回収できます。" :
        "敵を持って鉱石へ近づき、SPACE で攻撃。経験値は触れて回収。", { 400.f, 500.f },
        allBroken ? TextColor::Green : TextColor::Yellow, 0.8f, TextAlign::Center);
    text.DrawScreen("WASD プレイヤー移動   F 敵を持つ / 放す   SPACE 振る", { 400.f, 540.f },
        TextColor::White, 0.85f, TextAlign::Center);
    text.DrawScreen("R デモをリセット   ESC タイトル", { 400.f, 572.f },
        TextColor::White, 0.8f, TextAlign::Center);
}
