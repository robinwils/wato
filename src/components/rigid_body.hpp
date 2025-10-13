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
        aArchive.template Write<std::underlying_type_t<rp3d::BodyType>>(&aSelf.Params.Type, 1);
        ::Serialize(aArchive, aSelf.Params.Velocity);
        aArchive.template Write<float>(glm::value_ptr(aSelf.Params.Direction), 3);
        ::Serialize(aArchive, aSelf.Params.GravityEnabled);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<std::underlying_type_t<rp3d::BodyType>>(&aSelf.Params.Type, 1);
        ::Deserialize(aArchive, aSelf.Params.Velocity);
        aArchive.template Read<float>(glm::value_ptr(aSelf.Params.Direction), 3);
        ::Deserialize(aArchive, aSelf.Params.GravityEnabled);
    }
};

struct Collider {
    ColliderParams  Params;
    rp3d::Collider* Handle{nullptr};

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<unsigned short>(&aSelf.Params.CollisionCategoryBits, 1);
        aArchive.template Write<unsigned short>(&aSelf.Params.CollideWithMaskBits, 1);
        aArchive.template Write<bool>(&aSelf.Params.IsTrigger, 1);
        Transform3D::Serialize(aArchive, aSelf.Params.Offset);
        aArchive.template Write<rp3d::BoxShape>(&aSelf.Params.IsTrigger, 1);

        std::visit(
            VariantVisitor{
                [&](const BoxShapeParams& aShapeP) {
                    ::Serialize(aArchive, 0);
                    aArchive.template Write<float>(glm::value_ptr(aShapeP.HalfExtents), 3);
                },
                [&](const CapsuleShapeParams& aShapeP) {
                    ::Serialize(aArchive, 1);
                    ::Serialize(aArchive, aShapeP.Radius);
                    ::Serialize(aArchive, aShapeP.Height);
                },
                [&](const HeightFieldShapeParams& aShapeP) {
                    ::Serialize(aArchive, 2);
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
        ::Deserialize(aArchive, &aSelf.Params.CollideWithMaskBits);
        ::Deserialize(aArchive, &aSelf.Params.IsTrigger);
        Transform3D::Deserialize(aArchive, aSelf);

        int8_t shapeType = -1;
        ::Deserialize(aArchive, shapeType);
        switch (shapeType) {
            case 0: {
                BoxShapeParams shapeP;
                aArchive.template Read<float>(glm::value_ptr(shapeP.HalfExtents), 3);
            }
            case 1: {
                CapsuleShapeParams shapeP;
                ::Deserialize(aArchive, shapeP.Radius);
                ::Deserialize(aArchive, shapeP.Height);
            }
            case 2: {
                HeightFieldShapeParams shapeP;
                ::Deserialize(aArchive, shapeP.Data);
                ::Deserialize(aArchive, shapeP.Rows);
                ::Deserialize(aArchive, shapeP.Columns);
            }
            default:
                return false;
        }
        return true;
    }
};
