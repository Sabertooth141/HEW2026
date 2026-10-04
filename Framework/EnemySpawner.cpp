#include "EnemySpawner.h"

#include <algorithm>
#include <cmath>

#include "Camera2D.h"
#include "EnemySpawnPosition.h"
#include "Scene.h"
#include "WindowSettings.h"
#include "GameObject.h"

void EnemySpawner::LateUpdate(float deltaTime)
{
	auto& context = owner->GetContext();

	if (!context.gameScene)
	{
		return;
	}

	SpawnEnemies(deltaTime, *context.gameScene, context.camera);
}

void EnemySpawner::SpawnEnemies(float deltaTime,Scene& scene,const Camera2D& camera)
{
	if (deltaTime <= 0.f || !std::isfinite(deltaTime))
	{
		return;
	}

	const auto& objects = scene.GetObjects();
	const bool hasPlayer = std::any_of(objects.begin(), objects.end(), [](const auto& object)
	{
		return object->GetTag() == ObjectTag::Player;
	});

	if (!hasPlayer)
	{
		spawnTimer = 0.f;
		return;
	}

	spawnTimer += deltaTime;

	const float interval = std::fmax(0.05f, spawnInterval);

	if (spawnTimer < interval)
	{
		return;
	}

	// Avoid a burst of enemies after a long frame or after reaching the limit.
	spawnTimer = std::fmod(spawnTimer, interval);

	const auto count = std::count_if(objects.begin(), objects.end(), [](const auto& object)
	{
		return object->GetTag() == ObjectTag::Enemy;
	});

	if (static_cast<std::size_t>(count) >= maxEnemies)
	{
		return;
	}

	if (auto* enemy = scene.Instantiate("enemy", { 0.f, 0.f, 1.f }))
	{
		const auto scale = enemy->GetTransform()->GetScale();
		const auto position = RandomEnemySpawnPosition(camera.GetPosition(),
							{ static_cast<float>(WIN_WIDTH), static_cast<float>(WIN_HEIGHT) },
							{ scale.x, scale.y }, spawnMargin, spawnDepth, random);

		enemy->GetTransform()->SetPosition({ position.x, position.y, 1.f });
	}
}
