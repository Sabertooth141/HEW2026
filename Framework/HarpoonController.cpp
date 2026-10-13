#include "HarpoonController.h"

#include "GameObject.h"
#include "Scene.h"
#include "Rigidbody2DComponent.h"
#include "PlayerController.h"
#include "PlayerStatus.h"
#include "EnemyController.h"
#include "OreController.h"


void HarpoonController::Awake()
{
    MonoBehavior::Awake();
}


void HarpoonController::Update(float deltaTime)
{
    MonoBehavior::Update(deltaTime);

    if (!isStuck || stuckObject == nullptr)
        return;

    auto* scene = owner->GetContext().gameScene;
    if (scene == nullptr)
    {
        isStuck = false;
        stuckObject = nullptr;
        return;
    }

    // 敵を参照する前に、Scene内にまだ存在するか確認する
    auto& objects = scene->GetObjects();

    bool found = false;

    for (const auto& object : objects)
    {
        if (object.get() == stuckObject)
        {
            found = true;
            break;
        }
    }

    // すでに削除された敵なら、ハンマー処理を終了する
    if (!found)
    {
        isStuck = false;
        stuckObject = nullptr;

        auto* scene = owner->GetContext().gameScene;
        if (scene != nullptr)
        {
            scene->Destroy(owner);
        }

        return;
    }

    // ここから先は、Scene内に存在する敵だけを参照する
    auto* enemyController = stuckObject->GetComponent<EnemyController>();
    auto* oreController = stuckObject->GetComponent<OreController>();

    if (enemyController != nullptr && enemyController->IsDead())
    {
        isStuck = false;
        stuckObject = nullptr;
        return;
    }

    if (oreController != nullptr && oreController->IsDestroyed())
    {
        isStuck = false;
        stuckObject = nullptr;
        return;
    }

    if (enemyController == nullptr && oreController == nullptr)
    {
        isStuck = false;
        stuckObject = nullptr;
        return;
    }

    GameObject* player = nullptr;

    for (auto& object : objects)
    {
        if (object->GetComponent<PlayerController>() != nullptr)
        {
            player = object.get();
            break;
        }
    }

    if (player == nullptr)
        return;

    auto* playerController =
        player->GetComponent<PlayerController>();

    swingAngle -=
        deltaTime *
        playerController->GetStatus().GetSwingSpeed();

    auto playerPos = player->GetTransform()->GetPosition();

    float enemyX =
        playerPos.x + std::cos(swingAngle) * swingRadius;

    float enemyY =
        playerPos.y + std::sin(swingAngle) * swingRadius;

    auto objectPos = stuckObject->GetTransform()->GetPosition();
    objectPos.x = enemyX;
    objectPos.y = enemyY;
    stuckObject->GetTransform()->SetPosition(objectPos);

    auto harpoonPos = owner->GetTransform()->GetPosition();
    harpoonPos.x = enemyX;
    harpoonPos.y = enemyY;
    owner->GetTransform()->SetPosition(harpoonPos);
}

void HarpoonController::OnCollisionEnter2D(const GameObject& other)
{
    MonoBehavior::OnCollisionEnter2D(other);

    // すでにハンマー状態なら、次の衝突で対象を変更しない
    if (isStuck)
        return;

    if (other.GetTag() == ObjectTag::ENEMY ||
        const_cast<GameObject&>(other).GetComponent<OreController>() != nullptr)


    {

        GameObject* player = nullptr;

        auto& objects =
            owner->GetContext().gameScene->GetObjects();

        for (auto& object : objects)
        {
            if (object->GetComponent<PlayerController>() != nullptr)
            {
                player = object.get();
                break;
            }
        }

        if (player != nullptr)
        {
            auto* playerController =
                player->GetComponent<PlayerController>();

            float damage =
                playerController->GetStatus().GetSwingDamage();

            OutputDebugStringA(
                ("Harpoon damage: " + std::to_string(damage) + "\n").c_str()
            );
        }

        isStuck = true;

     

        for (auto& object : objects)
        {
            if (object.get() == &other)
            {
                stuckObject = object.get();

                auto* enemyController =
                    stuckObject->GetComponent<EnemyController>();
                auto* oreController =
                    stuckObject->GetComponent<OreController>();

                isOreTarget = (oreController != nullptr);

                if (enemyController != nullptr && player != nullptr)
                {
                    auto* playerController =
                        player->GetComponent<PlayerController>();

                    int damage = static_cast<int>(
                        playerController->GetStatus().GetSwingDamage());

                    enemyController->SetHammerActive(true, damage);
                    enemyController->SetExternalControl(true);
                }
                auto objectPos =
                    stuckObject->GetTransform()->GetPosition();

                GameObject* player = nullptr;

                for (auto& sceneObject : objects)
                {
                    if (sceneObject->GetComponent<PlayerController>() != nullptr)
                    {
                        player = sceneObject.get();
                        break;
                    }
                }

                if (player != nullptr)
                {
                    auto playerPos =
                        player->GetTransform()->GetPosition();

                    float directionX =
                        objectPos.x - playerPos.x;

                    float directionY =
                        objectPos.y - playerPos.y;

                    swingRadius =
                        std::sqrt(
                            directionX * directionX +
                            directionY * directionY);

                    swingAngle =
                        std::atan2(directionY, directionX);
                }

                OutputDebugStringA("Enemy was remembered!\n");

                break;
            }
        }

        auto* rb =
            owner->GetComponent<Rigidbody2DComponent>();

        if (rb != nullptr)
        {
            rb->SetVelocity({ 0.f, 0.f });
        }
    }
}

int HarpoonController::GetHammerDamage() const
{
    if (owner == nullptr)
        return 0;

    auto* scene = owner->GetContext().gameScene;
    if (scene == nullptr)
        return 0;

    for (const auto& object : scene->GetObjects())
    {
        auto* playerController =
            object->GetComponent<PlayerController>();

        if (playerController != nullptr)
        {
            return static_cast<int>(
                playerController->GetStatus().GetSwingDamage());
        }
    }

    return 0;
}