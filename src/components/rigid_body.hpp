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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Params.Type, 0, 3)) return false;
        if (!ArchiveValue(aArchive, Params.Velocity, 0.0f, 100.0f)) return false;
        if (!ArchiveVector(aArchive, Params.Direction, 0.0f, 1.0f)) return false;
        if (!ArchiveBool(aArchive, Params.GravityEnabled)) return false;
        return true;
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

    bool Archive(auto& aArchive)
    {
        uint32_t maxCategoryValue = SafeI32(Category::Count);
        if (!ArchiveValue(aArchive, Params.CollisionCategoryBits, 0u, maxCategoryValue))
            return false;
        if (!ArchiveValue(aArchive, Params.CollideWithMaskBits, 0u, maxCategoryValue)) return false;
        if (!ArchiveBool(aArchive, Params.IsTrigger)) return false;
        Params.Offset.Archive(aArchive);

        // uint32_t variantSize = SafeU32(std::variant_size_v<ColliderShapeParams>);
        // if (!std::visit(
        //         VariantVisitor{
        //             [&](BoxShapeParams& aShapeP) {
        //                 uint32_t tag = 0;
        //                 if (!ArchiveValue(aArchive, tag, 0u, variantSize)) return false;
        //                 if (!ArchiveVector(aArchive, aShapeP.HalfExtents, 0.0f, 10.0f))
        //                     return false;
        //                 return true;
        //             },
        //             [&](CapsuleShapeParams& aShapeP) {
        //                 uint32_t tag = 1;
        //                 if (!ArchiveValue(aArchive, tag, 0u, variantSize)) return false;
        //                 if (!ArchiveValue(aArchive, aShapeP.Radius, 0.0f, 10.0f)) return false;
        //                 if (!ArchiveValue(aArchive, aShapeP.Height, 0.0f, 10.0f)) return false;
        //                 return true;
        //             },
        //             [&](HeightFieldShapeParams& aShapeP) {
        //                 uint32_t tag = 3;
        //                 if (!ArchiveValue(aArchive, tag, 0u, variantSize)) return false;
        //                 if (!ArchiveValue(aArchive, aShapeP.Rows, 0, 1000)) return false;
        //                 if (!ArchiveValue(aArchive, aShapeP.Columns, 0, 1000)) return false;
        //                 return true;
        //             },
        //         },
        //         Params.ShapeParams))
        //     return false;
        ArchiveVariant(aArchive, Params.ShapeParams);
        return true;
    }

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
