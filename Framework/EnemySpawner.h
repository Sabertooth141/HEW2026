#pragma once
#include <cstddef>
#include <random>

class Camera2D;
class Scene;

class EnemySpawner
{
public:
	// World units match pixels in the current orthographic camera.
	float spawnInterval = 1.f;
	float spawnMargin = 32.f;
	float spawnDepth = 64.f;
	std::size_t maxEnemies = 30;

	void Update(float deltaTime, Scene& scene, const Camera2D& camera);

private:
	float spawnTimer = 0.f;
	std::mt19937 random{ std::random_device{}() };
};
