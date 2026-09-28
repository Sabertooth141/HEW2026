#pragma once
#include <functional>
#include <string>
#include <unordered_map>

class GameObject;

using PrefabFn = std::function<void(GameObject&)>;

class PrefabRegistry
{
public:
    static PrefabRegistry& Instance()
    {
        static PrefabRegistry instance;
        return instance;
    }

    void Register(const std::string& name, PrefabFn fn)
    {
        prefabs[name] = std::move(fn);
    }

    const PrefabFn* Find(const std::string& name) const
    {
        auto it = prefabs.find(name);
        return it != prefabs.end() ? &it->second : nullptr;
    }

private:
    PrefabRegistry() = default;
    std::unordered_map<std::string, PrefabFn> prefabs;
};

void RegisterPrefabs();