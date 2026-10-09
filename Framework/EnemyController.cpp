#include "EnemyController.h"

#include <algorithm>
#include <cmath>

#include "GameObject.h"
#include "Scene.h"

#include "BoxCollider2D.h"

void EnemyController::SetStatus(
    int hp,
    int attackPower,
    float moveSpeed,
    int dropExperience)
{
    status.hp = std::max(1, hp);
    status.attackPower = std::max(0, attackPower);
    status.moveSpeed = std::fmax(0.f, moveSpeed);
    status.dropExperience = std::max(0, dropExperience);
}

void EnemyController::SetExternalControl(bool enabled)
{
    if (dead)
    {
        return;
    }

    externalControl = enabled;
    knockbackTimer = 0.f;
}

void EnemyController::LateUpdate(float deltaTime)
{
    // 死亡処理は衝突判定の途中ではなく、ここで実行する
    if (dead)
    {
        FinishDeath();
        return;
    }

    if (deltaTime <= 0.f || !std::isfinite(deltaTime))
    {
        return;
    }

	damageCooldownTimer = std::max(0.f, damageCooldownTimer - deltaTime);

    // 捕獲中の位置・回転は銛側に任せる
    if (externalControl)
    {
        return;
    }

    auto* transform = owner->GetTransform();
    auto position = transform->GetPosition();

    // ノックバック中はプレイヤーを追いかけない
    if (knockbackTimer > 0.f)
    {
        const float moveTime =
            std::min(deltaTime, knockbackTimer);

        const float distance =
            std::fmax(0.f, knockbackSpeed) * moveTime;

        position.x += knockbackDirection.x * distance;
        position.y += knockbackDirection.y * distance;

        transform->SetPosition(position);

        knockbackTimer =
            std::max(0.f, knockbackTimer - deltaTime);

        return;
    }

    auto* scene = owner->GetContext().gameScene;
    if (!scene)
    {
        return;
    }

    // 通常時はプレイヤーへ向かって移動
    for (const auto& object : scene->GetObjects())
    {
        if (object->GetTag() != ObjectTag::Player)
        {
            continue;
        }

        const auto target =
            object->GetTransform()->GetPosition();

        const float dx = target.x - position.x;
        const float dy = target.y - position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= 0.001f)
        {
            return;
        }

        const float step =
            std::min(distance, status.moveSpeed * deltaTime);

        position.x += dx / distance * step;
        position.y += dy / distance * step;

        transform->SetPosition(position);
        return;
    }
}

bool EnemyController::TakeDamage(
    int damage,
    const DirectX::XMFLOAT3& attackPosition)
{
    if (dead || damage <= 0 || damageCooldownTimer > 0.f)
    {
        return false;
    }

    damageCooldownTimer = std::fmax(0.f, damageCooldown);
    status.hp = std::max(0, status.hp - damage);

    // 死亡時
    if (status.hp == 0)
    {
        dead = true;
        attackActive = false;
        deathPosition = owner->GetTransform()->GetPosition();
        knockbackTimer = 0.f;

        if (auto* anim = owner->GetComponent<AnimatorComponent>())
        {
            anim->SetEnabled(false);
        }

        if (auto* collider = owner->GetComponent<BoxCollider2D>())
        {
            collider->SetColliderActive(false);
        }

        // 経験値生成と削除予約は既存のFinishDeath()で行う
        return true;
    }

    // 捕獲されている間は銛側に任せる
    if (externalControl)
    {
        return true;
    }

    const auto position = owner->GetTransform()->GetPosition();

	// ノックバックの方向
    const float dx = position.x - attackPosition.x;
    const float dy = position.y - attackPosition.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0.001f)
    {
        knockbackDirection = { dx / distance, dy / distance };
    }
    else
    {
        knockbackDirection = { 1.f, 0.f };
    }

    knockbackTimer = std::fmax(0.f, knockbackDuration);

    return true;
}

void EnemyController::FinishDeath()
{
    if (deathHandled)
    {
        return;
    }

    auto* scene = owner->GetContext().gameScene;
    if (!scene)
    {
        return;
    }

    deathHandled = true;

  
    for (int i = 0; i < status.dropExperience; ++i)
    {
        auto position = deathPosition;

        const float angle = static_cast<float>(i) * 2.399963f;
        const float radius =
            18.f * std::sqrt(static_cast<float>(i));

        position.x += std::cos(angle) * radius;
        position.y += std::sin(angle) * radius;

        scene->Instantiate("experience", position);
    }

    scene->Destroy(owner);
}

void EnemyController::OnCollisionEnter2D(const GameObject& other)
{
    ReceiveCollisionDamage(other);
}

void EnemyController::OnCollisionStay2D(const GameObject& other)
{
    ReceiveCollisionDamage(other);
}

void EnemyController::ReceiveCollisionDamage(const GameObject& other)
{
	// 死亡時や捕獲中はダメージを受けない
    if (dead || externalControl)
    {
        return;
    }

    if (other.GetTag() != ObjectTag::Enemy)
    {
        return;
    }

    const auto* attacker = other.GetComponent<EnemyController>();

    // 敵同士が触れただけのときはダメージを与えない
    if (!attacker || !attacker->IsAttackActive())
    {
        return;
    }

    const auto* attackerTransform = other.GetTransform();

    if (!attackerTransform)
    {
        return;
    }

    TakeDamage(
        attacker->GetAttackPower(),
        attackerTransform->GetPosition());
}
