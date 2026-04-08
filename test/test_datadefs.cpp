#include <spdlog/spdlog.h>

#include <fstream>
#include <glaze/glaze.hpp>
#include <sstream>

#include "components/tower.hpp"
#include "core/gameplay_definitions.hpp"
#include "test.hpp"

using namespace entt::literals;

TEST_CASE("data_definitions")
{
    struct MyOpts : glz::opts {
        bool error_on_unknown_keys = false;
        bool skip_null_members     = true;
        bool prettify              = true;
    };

    std::ifstream file(TESTDATA_DIR "/gameplay.json");
    REQUIRE(file.is_open());

    std::stringstream buf;
    buf << file.rdbuf();
    std::string json = glz::prettify_json(buf.str());

    GameplayDef def{};
    auto        ret = glz::read<MyOpts{}>(def, json);
    CHECK_FALSE(ret);
    if (ret) {
        spdlog::error(glz::format_error(ret, json));
    }

    REQUIRE(def.Towers.contains(TowerType::Arrow));

    const auto& arrow = def.Towers[TowerType::Arrow];
    CHECK_EQ("tower_model"_hs, arrow.Model.Object.ModelHash);
    CHECK_EQ("tower_model", arrow.Model.Object.Name);

    std::string out{};
    REQUIRE_FALSE(glz::write<MyOpts{}>(def, out));
    CHECK_EQ(json, out);
}
