#pragma once

#include "MonoBehavior.h"
#include "OreState.h"
#include <unordered_map>

class OreController : public MonoBehavior
{
public:
    explicit OreController(OreType type = OreType::Gray);
    OreController(OreType type, const OreStats& stats);

    bool TakeDamage(int damage);
    int GetHP() const { return state.GetHP(); }
    int GetMaxHP() const { return state.GetMaxHP(); }
    OreType GetOreType() const { return state.GetOreType(); }
    const OreStats& GetStats() const { return state.GetStats(); }
    bool IsDestroyed() const { return state.IsDestroyed(); }

    void LateUpdate(float deltaTime) override;
    void OnDestroy() override;
    void OnCollisionEnter2D(const GameObject& other) override;
    void OnCollisionStay2D(const GameObject& other) override;
    void OnCollisionExit2D(const GameObject& other) override;

private:
    void HandleHammerContact(const GameObject& other);
    void HandleNativeHammerContact(GameObject& other);
    void RemoveExpiredNativeContacts();
    void QueueDestruction();
    void FinishDestruction();

    OreState state;
    bool destructionPending = false;
    bool destructionHandled = false;
    std::uint64_t contactFrameId = 1;
    DirectX::XMFLOAT3 destructionPosition{};

    struct NativeContactState
    {
        const GameObject* captureSource = nullptr;
        double fractionalDamage = 0.0;
        std::uint64_t lastDamageFrame = 0;
        bool appliedForContact = false;
    };
    // Keys and captureSource are identity tokens. Never dereference a retained
    // pointer; contact handling uses live objects supplied by physics instead.
    std::unordered_map<const GameObject*, NativeContactState> nativeContacts;
};
