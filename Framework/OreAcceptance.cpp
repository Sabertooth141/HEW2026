#include "OreAcceptance.h"

#include <array>
#include <exception>
#include <fstream>
#include <limits>

#include "AnimatorComponent.h"
#include "BoxCollider2D.h"
#include "ExperiencePickupController.h"
#include "GameScene.h"
#include "HammerContactComponent.h"
#include "OreController.h"
#include "OreDemoEnemyController.h"
#include "PlayerController.h"
#include "Rigidbody2DComponent.h"
#include "SceneManager.h"
#include "Win.h"

namespace
{
    constexpr std::array<int, 3> MaxHP{30, 60, 100};
    constexpr std::array<int, 3> ContactDamage{5, 10, 20};
    constexpr std::array<int, 3> Experience{5, 10, 20};
    constexpr std::array<int, 3> CollectedExperience{5, 15, 35};

    struct AcceptanceRun
    {
        std::ofstream log;
        ULONGLONG startedAt = 0;
        int passed = 0;
        bool started = false;
        bool resetRequested = false;
    };

    AcceptanceRun& Run()
    {
        static AcceptanceRun run;
        return run;
    }

    template<class T>
    int CountComponents(Scene& scene)
    {
        int count = 0;
        for (auto& object : scene.GetObjects())
            if (object->GetComponent<T>()) ++count;
        return count;
    }

    [[noreturn]] void FinishAcceptance(bool success, const char* description)
    {
        auto& run = Run();
        run.log << (success ? "ALL ORE UPLOAD ACCEPTANCE CHECKS PASSED: " : "FAIL ")
            << description << '\n' << "Passed checks: " << run.passed << '\n';
        run.log.flush();
        run.log.close();
        // App::Run returns 0 for WM_QUIT; preserve the acceptance failure code.
        ExitProcess(success ? 0 : 1);
    }

    void RecordCheck(bool condition, const char* description)
    {
        if (!condition) FinishAcceptance(false, description);
        auto& run = Run();
        ++run.passed;
        run.log << "PASS " << description << '\n';
        run.log.flush();
    }

    // Registered only after the demo acceptance has completed. Inheriting the
    // team's GameScene preserves its original player and OnEnter/OnUpdate.
    // SceneManager still performs the normal Game-specific ore placement.
    class GameStageAcceptanceScene : public GameScene
    {
    public:
        using GameScene::GameScene;

        void OnUpdate(float deltaTime) override
        {
            GameScene::OnUpdate(deltaTime);
            if (GetTickCount64() - Run().startedAt > 30000)
                FinishAcceptance(false, "Game stage acceptance timed out");
            if (++frames < 3) return;

            RecordCheck(context.sceneManager->GetCurrSceneName() == "Game",
                "normal Game scene registration receives stage ore placement");
            RecordCheck(CountComponents<OreController>(*this) == 3,
                "original Game stage contains exactly three live ore objects");

            std::array<int, 3> tierCounts{};
            std::array<bool, 3> correctHP{};
            for (auto& object : GetObjects())
            {
                auto* ore = object->GetComponent<OreController>();
                if (!ore) continue;
                int index = -1;
                switch (ore->GetOreType())
                {
                case OreType::Gray: index = 0; break;
                case OreType::LightBlue: index = 1; break;
                case OreType::Purple: index = 2; break;
                }
                if (index < 0) continue;
                ++tierCounts[index];
                correctHP[index] = ore->GetHP() == MaxHP[index] &&
                    ore->GetMaxHP() == MaxHP[index];
            }
            for (int index = 0; index < 3; ++index)
                RecordCheck(tierCounts[index] == 1 && correctHP[index],
                    "original Game places one configured ore of each HP tier");

            RecordCheck(CountComponents<PlayerController>(*this) == 1,
                "original Game keeps its native PlayerController");
            RecordCheck(CountComponents<OreDemoEnemyController>(*this) == 0,
                "original Game does not contain the isolated demo enemy");
            FinishAcceptance(true,
                "upload module, physical demo, reset and original Game placement verified");
        }

    private:
        int frames = 0;
    };
}

void OreAcceptanceScene::OnEnter()
{
    auto& run = Run();
    if (!run.started)
    {
        run.log.open("ore-upload-acceptance.txt", std::ios::trunc);
        if (!run.log)
        {
            OutputDebugStringA("Unable to open ore-upload-acceptance.txt\n");
            ExitProcess(2);
        }
        run.started = true;
        run.startedAt = GetTickCount64();
        run.log << "ORE UPLOAD ACCEPTANCE\n"
            << "New module/demo checks; not the previous v4 test results.\n"
            << "F/Space/movement use the same gameplay methods as input handlers.\n"
            << "Physics, scripts, LateUpdate and FlushPending use the normal App loop.\n";
        run.log.flush();
    }

    MiningDemoScene::OnEnter();
    Advance(run.resetRequested ? Phase::ReloadWarmup : Phase::Warmup);
}

void OreAcceptanceScene::OnUpdate(float deltaTime)
{
    try
    {
        if (GetTickCount64() - Run().startedAt > 30000)
            Finish(false, "acceptance did not finish within 30 seconds");

        ++phaseFrames;
        if (deltaTime > 0.f) phaseTime += deltaTime;
        MiningDemoScene::OnUpdate(deltaTime);
        RunPhase();
    }
    catch (const std::exception& error)
    {
        Finish(false, error.what());
    }
}

void OreAcceptanceScene::Advance(Phase next)
{
    phase = next;
    phaseFrames = 0;
    phaseTime = 0.f;
}

void OreAcceptanceScene::Require(bool condition, const char* description)
{
    RecordCheck(condition, description);
}

void OreAcceptanceScene::Finish(bool success, const char* description)
{
    FinishAcceptance(success, description);
}

bool OreAcceptanceScene::ContainsAddress(std::uintptr_t address)
{
    for (auto& object : GetObjects())
        if (reinterpret_cast<std::uintptr_t>(object.get()) == address) return true;
    return false;
}

void OreAcceptanceScene::MoveToPickup(int value)
{
    for (auto& object : GetObjects())
    {
        auto* pickup = object->GetComponent<ExperiencePickupController>();
        if (!pickup || pickup->IsCollected() || pickup->GetValue() != value) continue;
        MovePlayerTo(object->GetTransform()->GetPosition());
        return;
    }
    Finish(false, "expected experience pickup was not present");
}

void OreAcceptanceScene::RunPhase()
{
    switch (phase)
    {
    case Phase::Warmup:
    {
        if (phaseFrames < 3) return;
        Require(context.gameScene == this, "acceptance scene owns the current GameContext");
        Require(GetPlayerObject() != nullptr, "demo player exists");
        Require(GetExperience() == 0, "experience receiver starts at zero");
        Require(CountComponents<OreController>(*this) == 3, "three ore prefabs are live");
        for (int index = 0; index < 3; ++index)
        {
            auto* ore = GetOreObject(index);
            Require(ore && ore->GetComponent<OreController>()->GetHP() == MaxHP[index],
                "ore tier starts with its provisional HP");
        }
        auto* enemy = GetEnemyObject();
        Require(enemy != nullptr, "isolated demo enemy exists");
        auto* health = enemy->GetComponent<OreDemoEnemyController>();
        auto* hammer = enemy->GetComponent<HammerContactComponent>();
        Require(health && health->GetHP() == 200 && health->GetMaxHP() == 200,
            "demo enemy owns actual HP 200");
        Require(hammer && !hammer->IsHammerState(), "ordinary enemy is not a hammer");
        // Freeze ordinary pursuit only in this test fixture; collisions are real.
        health->moveSpeed = 0.f;
        enemy->GetTransform()->SetPosition(GetOreObject(0)->GetTransform()->GetPosition());
        enemy->GetComponent<Rigidbody2DComponent>()->Wake();
        Advance(Phase::OrdinaryContact);
        return;
    }
    case Phase::OrdinaryContact:
    {
        if (phaseFrames < 3) return;
        auto* enemy = GetEnemyObject();
        Require(enemy && enemy->GetComponent<OreDemoEnemyController>()->GetHP() == 200,
            "ordinary physical contact does not damage the enemy");
        Require(GetOreObject(0)->GetComponent<OreController>()->GetHP() == 30,
            "ordinary physical contact does not damage ore");
        const auto playerPosition = GetPlayerObject()->GetTransform()->GetPosition();
        enemy->GetTransform()->SetPosition({playerPosition.x + 80.f, playerPosition.y, 1.f});
        enemy->GetComponent<Rigidbody2DComponent>()->Wake();
        Advance(Phase::ReturnEnemy);
        return;
    }
    case Phase::ReturnEnemy:
    {
        if (phaseFrames < 3) return;
        Require(ToggleHammer(), "F gameplay path picks up the nearby enemy");
        auto* enemy = GetHeldEnemy();
        Require(enemy && enemy == GetEnemyObject(), "held reference is the live demo enemy");
        auto* hammer = enemy->GetComponent<HammerContactComponent>();
        Require(hammer->IsHammerState() && !hammer->IsActive(),
            "holding and swinging are separate states");
        hammer->attackPower = 0;
        const auto orePosition = GetOreObject(0)->GetTransform()->GetPosition();
        oreAddress = reinterpret_cast<std::uintptr_t>(GetOreObject(0));
        MovePlayerTo({orePosition.x - 72.f, orePosition.y, 1.f});
        expectedEnemyHP -= ContactDamage[0];
        Advance(Phase::HeldContact);
        return;
    }
    case Phase::HeldContact:
    {
        if (phaseFrames < 3) return;
        auto* enemy = GetHeldEnemy();
        Require(enemy && enemy->GetComponent<OreDemoEnemyController>()->GetHP() == expectedEnemyHP,
            "held contact with zero attack power damages actual enemy HP");
        Require(GetOreObject(0)->GetComponent<OreController>()->GetHP() == 30,
            "held contact without a swing leaves ore HP unchanged");
        Require(!enemy->GetComponent<HammerContactComponent>()->IsActive(),
            "contact damage did not manufacture a swing");
        enemy->GetComponent<HammerContactComponent>()->attackPower = 20;
        Advance(Phase::HeldStay);
        return;
    }
    case Phase::HeldStay:
        if (phaseFrames < 8) return;
        Require(GetHeldEnemy()->GetComponent<OreDemoEnemyController>()->GetHP() == expectedEnemyHP,
            "continuous held contact does not repeat damage every frame");
        tier = 0;
        hits = 0;
        Advance(Phase::BeginSwing);
        return;

    case Phase::BeginSwing:
        Require(StartHammerSwing(), "Space gameplay path begins a new real swing");
        ++hits;
        expectedEnemyHP -= ContactDamage[tier];
        Advance(Phase::WaitSwing);
        return;

    case Phase::WaitSwing:
    {
        auto* enemy = GetHeldEnemy();
        if (!enemy) Finish(false, "swing lost its live held enemy");
        if (phaseTime > 3.f) Finish(false, "demo swing failed to end");
        if (phaseFrames < 2 || enemy->GetComponent<HammerContactComponent>()->IsActive()) return;
        Require(true, "swing retains a live held enemy");
        const int remainingHP = MaxHP[tier] > hits * 20 ? MaxHP[tier] - hits * 20 : 0;
        auto* ore = GetOreObject(tier);
        Require(remainingHP == 0 ? ore == nullptr :
            ore && ore->GetComponent<OreController>()->GetHP() == remainingHP,
            "real swing deals exactly one attack to the selected ore");
        Require(enemy->GetComponent<OreDemoEnemyController>()->GetHP() == expectedEnemyHP,
            "real swing applies one reflected contact damage to enemy HP");
        if (remainingHP != 0)
        {
            Advance(Phase::BeginSwing);
            return;
        }
        Require(CountComponents<OreController>(*this) == 2 - tier,
            "HP-zero ore is physically removed from the object list");
        Require(!ContainsAddress(oreAddress), "destroyed ore reference is absent after FlushPending");
        Require(CountComponents<ExperiencePickupController>(*this) == tier + 1,
            "one experience object is spawned per destroyed ore");
        Advance(Phase::BrokenStay);
        return;
    }
    case Phase::BrokenStay:
    {
        if (phaseFrames < 6) return;
        Require(CountComponents<ExperiencePickupController>(*this) == tier + 1,
            "destroyed ore does not spawn a second drop");
        Require(GetExperience() == 0, "uncollected drops do not grant experience");
        if (tier < 2)
        {
            ++tier;
            hits = 0;
            auto* ore = GetOreObject(tier);
            const auto position = ore->GetTransform()->GetPosition();
            oreAddress = reinterpret_cast<std::uintptr_t>(ore);
            MovePlayerTo({position.x - 72.f, position.y, 1.f});
            expectedEnemyHP -= ContactDamage[tier];
            Advance(Phase::TierContact);
            return;
        }
        Require(CountComponents<OreController>(*this) == 0, "all three ore tiers are actually deleted");
        Require(expectedEnemyHP == 25, "demo contact and swing path has its own expected damage total");
        Require(ToggleHammer() && GetHeldEnemy() == nullptr,
            "F gameplay path releases the held enemy");
        collectionIndex = 0;
        MoveToPickup(Experience[collectionIndex]);
        Advance(Phase::WaitCollection);
        return;
    }
    case Phase::TierContact:
        if (phaseFrames < 3) return;
        Require(GetHeldEnemy()->GetComponent<OreDemoEnemyController>()->GetHP() == expectedEnemyHP,
            "new ore contact uses that tier's reflected damage");
        Require(GetOreObject(tier)->GetComponent<OreController>()->GetHP() == MaxHP[tier],
            "new tier contact needs a swing to damage ore");
        Advance(Phase::BeginSwing);
        return;

    case Phase::WaitCollection:
        if (phaseFrames < 3) return;
        Require(GetExperience() == CollectedExperience[collectionIndex],
            "real player collision forwards the pickup value to its experience receiver");
        Require(CountComponents<ExperiencePickupController>(*this) == 2 - collectionIndex,
            "collected experience object is actually removed");
        ++collectionIndex;
        if (collectionIndex < 3)
        {
            MoveToPickup(Experience[collectionIndex]);
            Advance(Phase::WaitCollection);
        }
        else Advance(Phase::CollectionStable);
        return;

    case Phase::CollectionStable:
        if (phaseFrames < 8) return;
        Require(GetExperience() == 35 && CountComponents<ExperiencePickupController>(*this) == 0,
            "Enter/Stay and later frames cannot award the same experience again");
        Advance(Phase::StartDeath);
        return;

    case Phase::StartDeath:
    {
        auto* enemy = GetEnemyObject();
        Require(enemy != nullptr, "demo enemy survived the three-tier gameplay path");
        const auto position = enemy->GetTransform()->GetPosition();
        MovePlayerTo({position.x - 80.f, position.y, 1.f});
        Require(ToggleHammer() && GetHeldEnemy() == enemy,
            "enemy can be held again before the death safety check");
        enemyAddress = reinterpret_cast<std::uintptr_t>(enemy);
        auto* health = enemy->GetComponent<OreDemoEnemyController>();
        Require(health->TakeDamage((std::numeric_limits<int>::max)()) && health->GetHP() == 0,
            "lethal damage clamps actual demo enemy HP to zero");
        Require(!enemy->GetComponent<HammerContactComponent>()->IsHammerState(),
            "death immediately stops hammer state");
        Require(GetHeldEnemy() == nullptr && GetEnemyObject() == nullptr,
            "live lookups reject the dead held enemy before deletion");
        Advance(Phase::WaitDeath);
        return;
    }
    case Phase::WaitDeath:
        if (phaseFrames < 3) return;
        Require(CountComponents<OreDemoEnemyController>(*this) == 0 && !ContainsAddress(enemyAddress),
            "dead enemy is physically removed after LateUpdate and FlushPending");
        Require(GetHeldEnemy() == nullptr && GetEnemyObject() == nullptr,
            "held and enemy references remain safe after actual deletion");
        Require(!StartHammerSwing() && !ToggleHammer(),
            "swing and pickup safely reject a deleted enemy");
        Advance(Phase::SpawnCancellation);
        return;

    case Phase::SpawnCancellation:
    {
        auto* cancelled = Instantiate("ore_gray", {0.f, 350.f, 1.f});
        Require(cancelled != nullptr, "pending-spawn cancellation fixture is created");
        cancelledAddress = reinterpret_cast<std::uintptr_t>(cancelled);
        Destroy(cancelled);
        Advance(Phase::WaitCancellation);
        return;
    }
    case Phase::WaitCancellation:
        if (phaseFrames < 3) return;
        Require(!ContainsAddress(cancelledAddress) && CountComponents<OreController>(*this) == 0,
            "destroying a pending spawn cannot resurrect it during FlushPending");
        Require(GetExperience() == 35 && CountComponents<ExperiencePickupController>(*this) == 0,
            "cancelled spawn does not manufacture a drop or reward");
        Advance(Phase::RequestReset);
        return;

    case Phase::RequestReset:
        // Only this explicitly selected acceptance process replaces the demo
        // factory, so its normal reset path can continue checking after reload.
        context.sceneManager->Register<OreAcceptanceScene>("OreDemo");
        Run().resetRequested = true;
        context.sceneManager->RequestChange("OreDemo");
        Advance(Phase::ReloadPending);
        return;

    case Phase::ReloadPending:
        return;

    case Phase::ReloadWarmup:
        if (phaseFrames < 3) return;
        Require(context.gameScene == this, "reset installs a fresh live scene");
        Require(GetPlayerObject() && GetExperience() == 0,
            "reset restores the player experience receiver to zero");
        Require(GetEnemyObject() && GetEnemyObject()->GetComponent<OreDemoEnemyController>()->GetHP() == 200,
            "reset restores a live demo enemy at full HP");
        Require(GetHeldEnemy() == nullptr, "reset clears the held enemy reference");
        Require(CountComponents<OreController>(*this) == 3 &&
            CountComponents<ExperiencePickupController>(*this) == 0,
            "reset restores all ores without old experience drops");
        for (int index = 0; index < 3; ++index)
            Require(GetOreObject(index) && GetOreObject(index)->GetComponent<OreController>()->GetHP() == MaxHP[index],
                "reset restores the configured HP for each tier");
        context.sceneManager->Register<GameStageAcceptanceScene>("Game");
        context.sceneManager->RequestChange("Game");
        Advance(Phase::ReloadPending);
        return;
    }
}
