#include <input/action.hpp>

#include "core/graph.hpp"
#include "test.hpp"

TEST_SUITE("graph")
{
    TEST_CASE("graph.is_inside")
    {
        Graph g(
            30 * GraphCell::kCellsPerAxis,
            20 * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});

        CHECK(g.IsInside(GraphCell(1, 2)));
        CHECK_FALSE(g.IsInside(GraphCell(1000, 2)));

        // Cell at limits
        CHECK(g.IsInside(GraphCell(0, 0)));
        CHECK(g.IsInside(
            GraphCell(30 * GraphCell::kCellsPerAxis - 1, 20 * GraphCell::kCellsPerAxis - 1)));
        CHECK_FALSE(g.IsInside(GraphCell(30 * GraphCell::kCellsPerAxis, 0)));
        CHECK_FALSE(g.IsInside(GraphCell(0, 20 * GraphCell::kCellsPerAxis)));

        // World points at limits
        CHECK(g.IsInside(0.0f, 0.0f));
        CHECK(g.IsInside(15.0f, 10.0f));
        CHECK(g.IsInside(29.99f, 19.99f));
        CHECK_FALSE(g.IsInside(30.0f, 0.0f));
        CHECK_FALSE(g.IsInside(0.0f, 20.0f));
        CHECK_FALSE(g.IsInside(-0.01f, 0.0f));
        CHECK_FALSE(g.IsInside(0.0f, -0.01f));
    }

    TEST_CASE("graph.is_inside_with_offset")
    {
        Graph g(
            30 * GraphCell::kCellsPerAxis,
            20 * GraphCell::kCellsPerAxis,
            glm::vec2{5.0f, 3.0f});

        CHECK_FALSE(g.IsInside(0.0f, 0.0f));
        CHECK_FALSE(g.IsInside(4.99f, 2.99f));
        CHECK(g.IsInside(5.0f, 3.0f));
        CHECK(g.IsInside(20.0f, 13.0f));
        CHECK(g.IsInside(34.99f, 22.99f));
        CHECK_FALSE(g.IsInside(35.0f, 23.0f));
    }

    TEST_CASE("graph.cell_from_world")
    {
        Graph g(
            30 * GraphCell::kCellsPerAxis,
            20 * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});

        CHECK_EQ(
            GraphCell(GraphCell::kCellsPerAxis * 10, GraphCell::kCellsPerAxis * 10),
            g.CellFromWorld(10.0f, 10.0f));
    }

    TEST_CASE("graph.simple_path")
    {
        Graph g(
            30 * GraphCell::kCellsPerAxis,
            20 * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});

        CHECK_EQ(std::nullopt, g.GetNextCell(GraphCell(1, 1)));
        g.ComputePaths(GraphCell(1, 2));

        CHECK_EQ(GraphCell(1, 2), g.GetNextCell(GraphCell(1, 1)));
        CHECK_EQ(GraphCell(1, 2), g.GetNextCell(GraphCell(1, 3)));
    }

    TEST_CASE("graph.path_with_obstacles")
    {
        Graph g(
            30 * GraphCell::kCellsPerAxis,
            20 * GraphCell::kCellsPerAxis,
            glm::vec2{0.0f, 0.0f});

        GraphCell start(0, 5);
        GraphCell dest(4, 5);
        GraphCell wall1(2, 4);
        GraphCell wall2(2, 5);
        GraphCell wall3(2, 6);

        g.AddObstacle(wall1);
        g.AddObstacle(wall2);
        g.AddObstacle(wall3);
        g.ComputePaths(dest.ToWorld());

        auto next = g.GetNextCell(start);
        CHECK_NE(std::nullopt, next);
        CHECK_NE(wall1, *next);
        CHECK_NE(wall2, *next);
        CHECK_NE(wall3, *next);

        CHECK_EQ(std::nullopt, g.GetNextCell(wall2));

        GraphCell current = start;
        for (int i = 0; i < 100; ++i) {
            if (current == dest) break;
            auto step = g.GetNextCell(current);
            REQUIRE_NE(std::nullopt, step);
            CHECK_NE(wall1, *step);
            CHECK_NE(wall2, *step);
            CHECK_NE(wall3, *step);
            current = *step;
        }
        CHECK_EQ(dest, current);
    }
}
