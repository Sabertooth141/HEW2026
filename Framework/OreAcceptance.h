#pragma once

#include <cstdint>

#include "MiningDemoScene.h"

// Run with DXPractice.exe OreAcceptance. The normal App loop advances physics,
// scripts and deferred deletion; this scene never recursively ticks a system.
class OreAcceptanceScene : public MiningDemoScene
{
public:
    using MiningDemoScene::MiningDemoScene;

    void OnEnter() override;
    void OnUpdate(float deltaTime) override;

private:
    enum class Phase
    {
        Warmup,
        OrdinaryContact,
        ReturnEnemy,
        HeldContact,
        HeldStay,
        BeginSwing,
        WaitSwing,
        BrokenStay,
        TierContact,
        WaitCollection,
        CollectionStable,
        StartDeath,
        WaitDeath,
        SpawnCancellation,
        WaitCancellation,
        RequestReset,
        ReloadPending,
        ReloadWarmup
    };

    void Advance(Phase next);
    void RunPhase();
    void Require(bool condition, const char* description);
    void Finish(bool success, const char* description);
    void MoveToPickup(int value);
    bool ContainsAddress(std::uintptr_t address);

    Phase phase = Phase::Warmup;
    int phaseFrames = 0;
    float phaseTime = 0.f;
    int tier = 0;
    int hits = 0;
    int expectedEnemyHP = 200;
    int collectionIndex = 0;
    std::uintptr_t oreAddress = 0;
    std::uintptr_t enemyAddress = 0;
    std::uintptr_t cancelledAddress = 0;
};
