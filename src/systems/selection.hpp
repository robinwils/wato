#pragma once

#include "systems/system.hpp"

/**
 * @brief single and box selection system
 *
 * Checks whether selectable entities are within the user selection box (multi)
 * or ray (single)
 * the box is a simple rect, we will project entities' positions to screen space
 * and check whether they are inside the selection box.
 * TODO: square bounding box projection for more precision
 * TODO: For bigger units we might want to make multiple points
 * TODO: Double click to select all units of same type. Detect with a threshold between 2 mouse down
 * Runs at deterministic 60 FPS.
 */
class SelectionSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

    const char* Name() const override { return "SelectionSystem"; }

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
