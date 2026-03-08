#include <input/action.hpp>

TEST_CASE("action.encode")
{
    StreamEncoder enc;
    Action        ae1 = kBuildTowerAction;
    Action        ae2 = kSendCreepAction;

    ae1.Archive(enc);
    ae2.Archive(enc);

    StreamDecoder dec(enc.Data());
    Action        ad1;
    Action        ad2;

    CHECK_EQ(ad1.Archive(dec), true);
    CHECK_EQ(ad1, ae1);

    CHECK_EQ(ad2.Archive(dec), true);
    CHECK_EQ(ad2, ae2);
}

