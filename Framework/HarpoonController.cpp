#include "HarpoonController.h"

#include "GameObject.h"
#include "Scene.h"
#include "Rigidbody2DComponent.h"
#include "PlayerController.h"
#include "PlayerStatus.h"

void HarpoonController::Awake()
{
    MonoBehavior::Awake();
}

void HarpoonController::Update(float deltaTime)
{
    MonoBehavior::Update(deltaTime);

    if (isStuck && stuckObject != nullptr)
    {
        auto& objects =
            owner->GetContext().gameScene->GetObjects();

        GameObject* player = nullptr;

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

            swingAngle -=
                deltaTime *
                playerController->GetStatus().GetSwingSpeed();

            auto playerPos =
                player->GetTransform()->GetPosition();

            float enemyX =
                playerPos.x +
                std::cos(swingAngle) * swingRadius;

            float enemyY =
                playerPos.y +
                std::sin(swingAngle) * swingRadius;

            auto objectPos =
                stuckObject->GetTransform()->GetPosition();

            objectPos.x = enemyX;
            objectPos.y = enemyY;

            stuckObject->GetTransform()->SetPosition(objectPos);

            auto harpoonPos =
                owner->GetTransform()->GetPosition();

            harpoonPos.x = enemyX;
            harpoonPos.y = enemyY;

            owner->GetTransform()->SetPosition(harpoonPos);
        }
    }
}

void HarpoonController::OnCollisionEnter2D(const GameObject& other)
{
    MonoBehavior::OnCollisionEnter2D(other);

    if (other.GetTag() == ObjectTag::Enemy// ||
      //  other.GetTag() == ObjectTag::Ore
        )
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