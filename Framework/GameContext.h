#pragma once

class Camera2D;
class Renderer;
class PhysicsSystem;
class ScriptSystem;
class AnimationSystem;
class RenderSystem;
class Scene;
class Keyboard;
class Mosue;
class SceneManager;

struct GlobalContext
{
    float towerHeight = 0.f;
};

struct GameContext
{
    Renderer& renderer;
    PhysicsSystem& physicsSys;
    ScriptSystem& scriptSys;
    AnimationSystem& animationSys;
    RenderSystem& renderSys;
    Camera2D& camera;
    Keyboard& keyboard;
    Mouse& mouse;

    Scene* gameScene = nullptr;
    SceneManager* sceneManager = nullptr;
    GlobalContext globalContext;
};