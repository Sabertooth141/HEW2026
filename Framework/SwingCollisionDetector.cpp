
#include "SwingCollisionDetector.h"
#include "GameObject.h"
#include "Scene.h"
#include "HarpoonController.h"
#include "PlayerController.h"
#include "EnemyController.h"
#include <Windows.h>

void SwingCollisionDetector::OnCollisionEnter2D(const GameObject& other)
{
    MonoBehavior::OnCollisionEnter2D(other);

    // 自分自身や敵以外との衝突は無視する
    if (&other == owner || other.GetTag() != ObjectTag::Enemy)
        return;

    auto* scene = owner->GetContext().gameScene;
    if (!scene)
        return;

    auto& objects = scene->GetObjects();

    // 自分がハープーンにつかまっているか調べる
    bool isSwinging = false;

    for (const auto& object : objects)
    {
        auto* harpoon = object->GetComponent<HarpoonController>();

        if (harpoon &&
            harpoon->IsStuck() &&
            harpoon->GetStuckObject() == owner)
        {
            isSwinging = true;
            break;
        }
    }

    // ハンマー状態でなければダメージを与えない
    if (!isSwinging)
        return;

    // 衝突相手の GameObject を探す
    GameObject* target = nullptr;

    for (const auto& object : objects)
    {
        if (object.get() == &other)
        {
            target = object.get();
            break;
        }
    }

    if (!target)
        return;

    // 相手もハンマー状態ならダメージを与えない
    for (const auto& object : objects)
    {
        auto* harpoon = object->GetComponent<HarpoonController>();

        if (harpoon &&
            harpoon->IsStuck() &&
            harpoon->GetStuckObject() == target)
        {
            return;
        }
    }

    // プレイヤーの攻撃力を取得する
    PlayerController* player = nullptr;

    for (const auto& object : objects)
    {
        player = object->GetComponent<PlayerController>();

        if (player)
            break;
    }

    if (!player)
        return;

    // 衝突相手の HP を減らす
    auto* enemy = target->GetComponent<EnemyController>();

    if (enemy)
    {
        const float damage = player->GetStatus().GetSwingDamage();
        enemy->TakeDamage(damage);
        OutputDebugStringA("Swing damage applied!\n");
    }
}
