#include <inttypes.h>
#include <stdint.h>

namespace ImGui
{
struct Font {
    enum Enum {
        Regular,
        Mono,

        Count
    };
};

void PushFont(Font::Enum _font);

// BK - simple string class for convenience.
class ImString
{
   public:
    ImString();
    ImString(const ImString& rhs);
    ImString(const char* rhs);
    ~ImString();

    ImString& operator=(const ImString& rhs);
    ImString& operator=(const char* rhs);

    void Clear();
    bool IsEmpty() const;

    const char* CStr() const { return NULL == Ptr ? "" : Ptr; }

   private:
    char* Ptr;
};

}  // namespace ImGui