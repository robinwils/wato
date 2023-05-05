#pragma once
#include <entt/entt.hpp>

struct Registry : public entt::basic_registry<entt::entity>
{
	void spawnPlane();
};

