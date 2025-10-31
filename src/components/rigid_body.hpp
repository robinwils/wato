#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/vector_float3.hpp>

#include "core/physics/physics.hpp"
#include "core/serialize.hpp"
#include "registry/registry.hpp"

struct RigidBody {
    RigidBodyParams  Params;
    rp3d::RigidBody* Body{nullptr};

    static void on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        const auto& body = aRegistry.get<RigidBody>(aEntity);
        if (body.Body) {
            aRegistry.ctx().get<Physics>().World()->destroyRigidBody(body.Body);
        }
    }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.Params.Type);
        ::Serialize(aArchive, aSelf.Params.Velocity);
        ::Serialize(aArchive, aSelf.Params.Direction);
        aArchive.Write(aSelf.Params.GravityEnabled);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.Params.Type);
        ::Deserialize(aArchive, aSelf.Params.Velocity);
        ::Deserialize(aArchive, aSelf.Params.Direction);
        aArchive.Read(aSelf.Params.GravityEnabled);
    }
};

struct Collider {
    ColliderParams  Params;
    rp3d::Collider* Handle{nullptr};

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.Params.CollisionCategoryBits);
        ::Serialize(aArchive, aSelf.Params.CollideWithMaskBits);
        aArchive.Write(aSelf.Params.IsTrigger);
        Transform3D::Serialize(aArchive, aSelf.Params.Offset);

        std::visit(
            VariantVisitor{
                [&](const BoxShapeParams& aShapeP) {
                    ::Serialize(aArchive, static_cast<int8_t>(0));
                    ::Serialize(aArchive, aShapeP.HalfExtents);
                },
                [&](const CapsuleShapeParams& aShapeP) {
                    ::Serialize(aArchive, static_cast<int8_t>(1));
                    ::Serialize(aArchive, aShapeP.Radius);
                    ::Serialize(aArchive, aShapeP.Height);
                },
                [&](const HeightFieldShapeParams& aShapeP) {
                    ::Serialize(aArchive, static_cast<int8_t>(2));
                    ::Serialize(aArchive, aShapeP.Data);
                    ::Serialize(aArchive, aShapeP.Rows);
                    ::Serialize(aArchive, aShapeP.Columns);
                },
            },
            aSelf.Params.ShapeParams);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.Params.CollisionCategoryBits);
        ::Deserialize(aArchive, aSelf.Params.CollideWithMaskBits);
        aArchive.Read(aSelf.Params.IsTrigger);
        Transform3D::Deserialize(aArchive, aSelf.Params.Offset);

        int8_t shapeType = -1;
        ::Deserialize(aArchive, shapeType);
        switch (shapeType) {
            case 0: {
                BoxShapeParams shapeP;
                ::Deserialize(aArchive, shapeP.HalfExtents);
                aSelf.Params.ShapeParams = shapeP;
                return true;
            }
            case 1: {
                CapsuleShapeParams shapeP;
                ::Deserialize(aArchive, shapeP.Radius);
                ::Deserialize(aArchive, shapeP.Height);
                aSelf.Params.ShapeParams = shapeP;
                return true;
            }
            case 2: {
                HeightFieldShapeParams shapeP;
                ::Deserialize(aArchive, shapeP.Data);
                ::Deserialize(aArchive, shapeP.Rows);
                ::Deserialize(aArchive, shapeP.Columns);
                aSelf.Params.ShapeParams = shapeP;
                return true;
            }
            default:
                return false;
        }
    }
};