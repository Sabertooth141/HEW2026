#pragma once
#include <cstddef>
#include <random>

#include "MonoBehavior.h"

class Camera2D;
class Scene;

class EnemySpawner : public MonoBehavior
{
public:
	float spawnInterval = 1.f;
	float spawnMargin = 32.f;			 // spawn outside of the camera view
	float spawnDepth = 64.f;			 // spawn at this depth (z-axis)
	std::size_t maxEnemies = 30;

	void LateUpdate(float deltaTime) override;

private:
	void SpawnEnemies(float deltaTime, Scene& scene, const Camera2D& camera);
	float spawnTimer = 0.f;
	std::mt19937 random{ std::random_device{}() };
};
