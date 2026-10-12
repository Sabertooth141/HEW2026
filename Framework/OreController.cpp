#include "OreController.h"

#include <cmath>

#include "AnimatorComponent.h"
#include "BoxCollider2D.h"
#include "ExperiencePickupController.h"
#include "GameObject.h"
#include "HammerContactComponent.h"
#include "OreGameplayAdapter.h"
#include "Scene.h"
#include "EnemyController.h"
#include "HarpoonController.h"

OreController::OreController(OreType type)
    : state(type)
{
}

OreController::OreController(OreType type, const OreStats& stats)
    : state(type, stats)
{
}

bool OreController::TakeDamage(int damage)
{
    if (!state.TakeDamage(damage)) return false;
    if (state.IsDestroyed()) QueueDestruction();
    return true;
}

void OreController::LateUpdate(float)
{
    ++contactFrameId; // Local frame boundary; no GameContext/API changes needed.
    RemoveExpiredNativeContacts();
    if (destructionPending) FinishDestruction();
}

void OreController::OnDestroy()
{
    // Clearing/replacing a scene cancels drops that have not been finalized.
    destructionPending = false;
    destructionHandled = true;
    nativeContacts.clear();
    SetEnabled(false);
}

void OreController::OnCollisionEnter2D(const GameObject& other)
{
    OutputDebugStringA("Ore collision: ENTER\n");
    HandleHammerContact(other);
}

void OreController::OnCollisionStay2D(const GameObject& other)
{
    OutputDebugStringA("Ore collision: STAY\n");
    HandleHammerContact(other);
}

void OreController::OnCollisionExit2D(const GameObject& other)
{
    // The framework currently lacks const component access (see below).
    const auto* hammer = const_cast<GameObject&>(other)
        .GetComponent<HammerContactComponent>();
    if (hammer) state.EndHammerContact(hammer->GetContactId());
    if (auto contact = nativeContacts.find(&other); contact != nativeContacts.end())
    {
        // Carry fractional damage across separate contacts; a new contact still
        // receives one new hit, including Enter after Exit.
        contact->second.appliedForContact = false;
        contact->second.captureSource = nullptr;
    }
}

void OreController::HandleHammerContact(const GameObject& other)
{
    if (!owner || IsDestroyed()) return;

    // This framework lacks const GameObject::GetComponent. Collision objects
    // are live mutable GameObjects; read the bridge as const, or pass the live
    // object to the native adapter when its enemy damage receiver is needed.
    const auto* hammer = const_cast<GameObject&>(other)
        .GetComponent<HammerContactComponent>();
    if (!hammer)
    {
        HandleNativeHammerContact(const_cast<GameObject&>(other));
        return;
    }

    const auto activeAttackId = hammer->IsActive() ? hammer->GetAttackId() : 0;
    const bool damageEnemy = state.TryRegisterHammerContactDamage(
        hammer->GetContactId(), hammer->GetHammerStateId(),
        activeAttackId, contactFrameId);
    const bool damageOre = hammer->IsActive() &&
        hammer->GetAttackPower() > 0 &&
        state.TryRegisterHammerHit(activeAttackId);
    if (!damageEnemy && !damageOre) return;

    const auto contactPosition = owner->GetTransform()->GetPosition();
    const int returnDamage = GetStats().collisionDamage;
    if (damageOre) TakeDamage(hammer->GetAttackPower());
    // Both sides receive this contact's damage, including the hit that breaks
    // the ore. New objects and deletion are deferred until after physics.
    if (damageEnemy) hammer->ApplyContactDamage(returnDamage, contactPosition);
}

void OreController::HandleNativeHammerContact(GameObject& other)
{
    // ハンマー状態の鉱石が、通常の鉱石にぶつかった場合
    auto* otherOre = other.GetComponent<OreController>();

    if (otherOre != nullptr)
    {
        auto* scene = owner->GetContext().gameScene;

        if (scene != nullptr)
        {
            for (const auto& object : scene->GetObjects())
            {
                auto* harpoon =
                    object->GetComponent<HarpoonController>();

                if (harpoon != nullptr &&
                    harpoon->IsHammerObject(&other))
                {
                    OutputDebugStringA(
                        "Hammer ore recognized!\n");

                    const int damage = harpoon->GetHammerDamage();

                    OutputDebugStringA(
                        ("Hammer damage: " +
                            std::to_string(damage) + "\n").c_str());

                    if (damage > 0)
                    {
                        TakeDamage(damage);
                        OutputDebugStringA(
                            "Normal ore damaged!\n");
                    }
                    return;
                }
            }
        }
    }

    // ハンマー状態の敵が鉱石にぶつかった場合
    auto* enemy = other.GetComponent<EnemyController>();

    if (enemy != nullptr && enemy->IsHammerActive())
    {
        auto& contact = nativeContacts[&other];

        // 同じ接触中に何度もダメージを与えない
        if (contact.appliedForContact)
            return;

        contact.appliedForContact = true;
        contact.captureSource = nullptr;
        contact.lastDamageFrame = contactFrameId;

        const int damage = enemy->GetHammerDamage();

        if (damage > 0)
        {
            TakeDamage(damage);
        }

        return;
    }
    const auto query = OreGameplayAdapter::QueryNativeHammerContact(other);
    if (!query.captured)
    {
        if (auto contact = nativeContacts.find(&other);
            contact != nativeContacts.end())
        {
            // Capturing an enemy that already overlaps the ore is recognized
            // by a later Stay callback; ordinary enemies receive no ore damage.
            contact->second.appliedForContact = false;
            contact->second.captureSource = nullptr;
        }
        return;
    }

    auto& contact = nativeContacts[&other];
    if (contact.appliedForContact && contact.captureSource == query.captureSource)
        return;
    if (contact.lastDamageFrame == contactFrameId) return;

    contact.appliedForContact = true;
    contact.captureSource = query.captureSource;
    contact.lastDamageFrame = contactFrameId;

    // Native player damage is float. Retain its fractional part rather than
    // truncating every contact (e.g. four hits of 0.75 deal three integer HP).
    const double accumulated = contact.fractionalDamage + query.damage;
    const double wholeDamage = std::floor(accumulated);
    contact.fractionalDamage = accumulated - wholeDamage;
    const int damage = wholeDamage >= GetHP() ? GetHP() :
        static_cast<int>(wholeDamage);
    const float returnDamage = static_cast<float>(GetStats().collisionDamage);
    if (damage > 0) TakeDamage(damage);
    OreGameplayAdapter::ApplyNativeContactDamage(other, returnDamage);
}

void OreController::RemoveExpiredNativeContacts()
{
    if (nativeContacts.empty()) return;
    auto* scene = owner ? owner->GetContext().gameScene : nullptr;
    if (!scene)
    {
        nativeContacts.clear();
        return;
    }
    for (auto contact = nativeContacts.begin(); contact != nativeContacts.end();)
    {
        bool live = false;
        for (const auto& object : scene->GetObjects())
        {
            if (object.get() == contact->first)
            {
                live = true;
                break;
            }
        }
        if (live) ++contact;
        else contact = nativeContacts.erase(contact);
    }
}

void OreController::QueueDestruction()
{
    if (destructionPending || destructionHandled || !owner) return;
    destructionPending = true;
    destructionPosition = owner->GetTransform()->GetPosition();

    if (auto* collider = owner->GetComponent<BoxCollider2D>())
        collider->SetColliderActive(false);
    if (auto* animator = owner->GetComponent<AnimatorComponent>())
        animator->SetEnabled(false);
    // Keep this script enabled so ScriptSystem can finalize in LateUpdate.
    // Instantiating a collidable pickup inside a collision callback can resize
    // PhysicsSystem::entries and invalidate its in-flight candidate pointers.
}

void OreController::FinishDestruction()
{
    if (!destructionPending || destructionHandled || !owner) return;
    destructionPending = false;
    destructionHandled = true; // Set before spawning to prevent re-entry.
    SetEnabled(false);
    if (!owner->GetContext().gameScene) return;

    if (auto* pickup = Instantiate("ore_experience", destructionPosition))
    {
        auto* experience = pickup->GetComponent<ExperiencePickupController>();
        if (!experience)
            experience = &pickup->AddComponent<ExperiencePickupController>();
        experience->SetValue(GetStats().experience);
    }
    DestroySelf();
}
