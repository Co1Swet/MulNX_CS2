#include <MulNX/MulNX.hpp>

class ShortcutManager final :public MulNX::ModuleBase {
    class Bind{
    public:
        std::string desc;
        std::string msg;
        MulNX::KeyCheckPack KCP;
    };
    std::vector<Bind>binds;
    void Check();
public:
    bool Init();
};