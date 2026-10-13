#include "OrePrefabs.h"

#include "ExperiencePickupController.h"
#include "GameObject.h"
#include "HammerContactComponent.h"
#include "OreController.h"
#include "OreDemoEnemyController.h"
#include "PrefabRegistry.h"
#include "Scene.h"

namespace
{
void RegisterOre(const char* name, OreType type, const wchar_t* texture)
{
    PrefabRegistry::Instance().Register(name, [type, texture](GameObject& object)
    {
        object.SetTag(ObjectTag::DEFAULT);
        auto& animator = object.AddComponent<AnimatorComponent>(object.GetRenderer());
        animator.SetStatic(texture);
        object.GetTransform()->SetScale({128.f, 128.f, 1.f});
        auto& body = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f, true);
        body.SetGravity(0.f);
        object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2{0.42f, 0.38f},
            DirectX::XMFLOAT2{0.f, 0.f}, false, *object.GetTransform());
        object.AddComponent<OreController>(type);
    });
}
}

void RegisterOrePrefabs()
{
    RegisterOre("ore_gray", OreType::Gray, L"../../assets/ore/ore_gray.png");
    RegisterOre("ore_blue", OreType::LightBlue, L"../../assets/ore/ore_blue.png");
    RegisterOre("ore_purple", OreType::Purple, L"../../assets/ore/ore_purple.png");
    PrefabRegistry::Instance().Register("ore_experience", [](GameObject& object)
    {
        object.SetTag(ObjectTag::DEFAULT);
        auto& animator = object.AddComponent<AnimatorComponent>(object.GetRenderer());
        animator.SetSortOrder(1);
        animator.SetStatic(L"../../assets/ore/ore_blue.png");
        object.GetTransform()->SetScale({28.f, 28.f, 1.f});
        auto& body = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f, true);
        body.SetGravity(0.f);
        object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2{0.45f, 0.45f},
            DirectX::XMFLOAT2{0.f, 0.f}, true, *object.GetTransform());
        object.AddComponent<ExperiencePickupController>();
    });
    // Demo-only enemy; the team's "enemy" prefab and HP controller stay intact.
    PrefabRegistry::Instance().Register("ore_demo_enemy", [](GameObject& object)
    {
        object.SetTag(ObjectTag::ENEMY);
        auto& animator = object.AddComponent<AnimatorComponent>(object.GetRenderer());
        animator.SetRenderLayer(RenderLayer::Enemy);
        animator.SetStatic(L"../../assets/ore/hammer_enemy.png");
        object.GetTransform()->SetScale({48.f, 48.f, 1.f});
        auto& body = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f);
        body.SetGravity(0.f);
        body.SetFreezeRotation(true);
        object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2{0.4f, 0.4f},
            DirectX::XMFLOAT2{0.f, 0.f}, true, *object.GetTransform());
        auto& health = object.AddComponent<OreDemoEnemyController>(200);
        auto& hammer = object.AddComponent<HammerContactComponent>(20);
        hammer.SetDamageCallback([controller = &health](int damage, const DirectX::XMFLOAT3& position)
        {
            controller->TakeDamage(damage, position);
        });
    });
}

void PlaceStageOres(Scene& scene)
{
    scene.Instantiate("ore_gray", {-220.f, 50.f, 1.f});
    scene.Instantiate("ore_blue", {0.f, 50.f, 1.f});
    scene.Instantiate("ore_purple", {220.f, 50.f, 1.f});
}
