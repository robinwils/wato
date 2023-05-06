#pragma once
#include <entt/entt.hpp>
#include <renderer/bgfx_utils.hpp>

struct Registry : public entt::basic_registry<entt::entity>
{
	void spawnPlane();

private:
	BxFactory m_bxFactory;
};

