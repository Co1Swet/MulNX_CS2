#include "FileInjector.hpp"
#include <Mirror/VFileSystem/VFileSystem.hpp>

bool FileInjector::Init() {
    this->pVFileSystem = this->FindModule<VFileSystem>("VFileSystem");

    this->SubscribeSync("Hook/Source2Client002::Inited", [this](auto&&...) {
        auto root = this->Path()->GetRoot();
        const auto cs2 = root / "cs2";

        this->pVFileSystem->AddSearchPath(cs2.string().c_str(), "GAME");

        // 扫描 cs2/materials 下的所有 .vmat_c
        const auto materialRoot = cs2 / "materials";
        std::error_code ec;

        if (!std::filesystem::exists(materialRoot, ec) || ec) {
            this->LogError(std::format("材质目录不存在: {}", materialRoot.string()));
            return;
        }

        size_t count = 0;

        for (auto& entry : std::filesystem::recursive_directory_iterator(
            materialRoot,
            std::filesystem::directory_options::skip_permission_denied,
            ec)) {
            if (ec) { ec.clear(); continue; }
            if (!entry.is_regular_file(ec) || ec) { ec.clear(); continue; }
            if (entry.path().extension() != ".vmat_c") continue;

            auto rel = std::filesystem::relative(entry.path(), materialRoot, ec);
            if (ec) { ec.clear(); continue; }

            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>(
                "Sky/materials/ReportNew"_hash);
            rp->str1 = "materials/" + rel.generic_string();
            this->PublishAsync(std::move(msg));

            ++count;
        }

        this->LogSucc(std::format("材质扫描完成，共找到 {} 个 .vmat_c", count));
        });

    return true;
}