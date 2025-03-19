#include <inttypes.h>
#include <stdint.h>

#include <cstddef>

namespace ImGui
{
struct Font {
    enum Enum {
        Regular,
        Mono,

        Count
    };
};

void PushFont(Font::Enum aFont);

// BK - simple string class for convenience.
class ImString
{
   public:
    ImString();
    ImString(const ImString& aRhs);
    ImString(const char* aRhs);
    ~ImString();

    ImString& operator=(const ImString& aRhs);
    ImString& operator=(const char* aRhs);

    void Clear();
    bool IsEmpty() const;

    const char* CStr() const { return NULL == mPtr ? "" : mPtr; }

   private:
    char* mPtr;
};

}  // namespace ImGui
