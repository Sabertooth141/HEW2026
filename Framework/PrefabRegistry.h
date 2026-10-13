#pragma once
#include <algorithm>
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

    // 登録済みプレハブ名の一覧（名前順）
    std::vector<std::string> GetPrefabNames() const
    {
        std::vector<std::string> names;
        names.reserve(prefabs.size());

        for (const auto& [name, function] : prefabs)
        {
	        names.push_back(name);
        }

        std::ranges::sort(names);
        return names;
    }

private:
    PrefabRegistry() = default;
    std::unordered_map<std::string, PrefabFn> prefabs;
};

void RegisterPrefabs();