#pragma once
#include <vector>

#include "MonoBehavior.h"

class ScriptSystem
{
public:
	void RegisterScript(MonoBehavior* script)
	{
		script->Awake();
		pendingScripts.push_back(script);
	}

	void Update(float deltaTime)
	{
		for (auto* script : pendingScripts)
		{
			if (script->IsEnabled())
			{
				script->Start();
				script->SetStarted(true);
			}
			activeScripts.push_back(script);
		}
		pendingScripts.clear();

		for (auto* script : activeScripts)
		{
			if (script->IsEnabled())
			{
				script->Update(deltaTime);
			}
		}

		FlushDestroyed();
	}

	void LateUpdate(float deltaTime)
	{
		for (const auto& script : activeScripts)
		{
			if (script->IsEnabled())
			{
				script->LateUpdate(deltaTime);
			}
		}
	}

	void UnregisterScript(MonoBehavior* script)
	{
		script->OnDestroy();
		destroyQueue.push_back(script);
	}

	void Unregister(GameObject* object)
	{
		const auto kill = [&](std::vector<MonoBehavior*>& v)
		{
			std::erase_if(v, [&](MonoBehavior* s)
			{
				if (s->owner != object) return false;
				s->OnDestroy();
				return true;
			});
		};
		kill(activeScripts);
		kill(pendingScripts); // a script destroyed on its spawn frame is still only in pending
	}

private:
	void FlushDestroyed()
	{
		for (auto* destroyed : destroyQueue)
		{
			std::erase(activeScripts, destroyed);
			std::erase(pendingScripts, destroyed);
		}
		destroyQueue.clear();
	}

private:
	std::vector<MonoBehavior*> pendingScripts;
	std::vector<MonoBehavior*> activeScripts;
	std::vector<MonoBehavior*> destroyQueue;
};
