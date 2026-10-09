#include "IPCer.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>
#include <strstream>
#include <Windows.h>

bool MulNX::IPCer::GetWindowPathByName(const LPCWSTR& WindowName, fs::path& Output) {
    HWND hwnd = FindWindowW(NULL, WindowName);
    if (!hwnd) {
        this->LogError("没有找到窗口");
        return false; // 窗口未找到
    }

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        this->LogError("进程ID未找到");
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!hProcess) {
        this->LogError("无法打开进程");
        return false;
    }

    WCHAR exePath[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;

    //获取可执行文件路径
    if (!QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
        this->LogError("得到可执行文件路径失败");
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);

    Output = exePath;
    return true;
}

bool MulNX::IPCer::Init() {

    return true;
}

std::vector<std::string> MulNX::IPCer::GetDirNamesByPath(fs::path path) {
    std::vector<std::string> fileNames;
    //检查路径存在且为文件夹
    if (!fs::exists(path) || !fs::is_directory(path)) {
        this->LogError(std::format("尝试获取 {} 的所有子文件夹名时出错：文件夹 {} 不存在！", path.string(), path.string()));
        return fileNames;
    }
    //遍历目录
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string FileName = entry.path().filename().string();
            fileNames.push_back(std::move(FileName));
        }
    }
    return fileNames;
}
std::vector<std::string> MulNX::IPCer::GetFileNamesByPath(fs::path& FolderPath) {
    std::vector<std::string> FileNames;
    //检查路径存在且为文件夹
    if (!fs::exists(FolderPath) || !fs::is_directory(FolderPath)) {
        this->LogError("尝试获取指定文件夹内文件名时出现错误：目标文件夹不存在！\n文件夹路径：" + FolderPath.string());
        return FileNames;
    }
    //遍历目录
    for (const auto& entry : fs::directory_iterator(FolderPath)) {
        if (entry.is_regular_file()) {
            std::string FileName = entry.path().filename().string();
            FileNames.push_back(std::move(FileName));
        }
    }
    return FileNames;
}


bool MulNX::IPCer::GetFileNames(std::vector<std::string>& FileNames, const fs::path& FolderPath, const std::vector<std::string>& Filter, const bool Extension) {
    //检查路径存在且为文件夹
    if (!fs::exists(FolderPath) || !fs::is_directory(FolderPath)) {
        this->LogError("尝试获取指定文件夹内文件名时出现错误：目标文件夹不存在！\n文件夹路径：" + FolderPath.string());
        return false;
    }
    //清空输出向量
    FileNames.clear();

    //遍历文件夹中的所有条目
    for (const auto& entry : fs::directory_iterator(FolderPath)) {
        if (entry.is_regular_file()) {  // 只处理普通文件
            fs::path FilePath = entry.path();
            std::string ExtensionName = FilePath.extension().string();
            if (std::find(Filter.begin(), Filter.end(), ExtensionName) != Filter.end()) {
                if (Extension) {
                    FileNames.push_back(FilePath.filename().string());
                }
                else {
                    FileNames.push_back(FilePath.stem().string());
                }
            }
        }
    }

    return true;
}

bool MulNX::IPCer::FileDelete(const std::string& FileName, const fs::path& FolderPath) {
    //检查文件名是否为空
    if (FileName.empty()) {
        this->LogError("文件名为空，无法删除文件！");
        return false;
    }
    //构建完整的文件路径
    fs::path FilePath = FolderPath / FileName;
    //检查文件是否存在
    if (!fs::exists(FilePath)) {
        this->LogError("指定的文件不存在，无法删除文件！\n文件路径：" + FilePath.string());
        return false;
    }
    //检查是否为普通文件
    if (!fs::is_regular_file(FilePath)) {
        this->LogError("指定的路径不是有效的文件，无法删除！\n文件路径：" + FilePath.string());
        return false;
    }
    //删除文件
    try {
        if (!fs::remove(FilePath)) {
            this->LogError("无法删除文件，可能权限不足或文件被占用！\n文件路径：" + FilePath.string());
            return false;
        }
    }
    catch (const fs::filesystem_error& e) {
        this->LogError("删除文件时出错，错误信息：" + std::string(e.what()));
        return false;
    }
    this->LogSucc("成功删除文件，路径：" + FilePath.string());
    return true;
}
bool MulNX::IPCer::FileMove(const std::string& FileName, const fs::path& Resource, const fs::path& Target) {
    //检查文件名是否为空
    if (FileName.empty()) {
        this->LogError("文件名为空，无法移动文件！");
        return false;
    }
    //构建完整的源文件路径和目标文件路径
    fs::path SourcePath = Resource / FileName;
    fs::path TargetPath = Target / FileName;
    //检查源文件是否存在
    if (!fs::exists(SourcePath)) {
        this->LogError("源文件不存在，无法移动文件！\n源路径：" + SourcePath.string());
        return false;
    }
    //检查源文件是否为普通文件
    if (!fs::is_regular_file(SourcePath)) {
        this->LogError("源路径不是有效的文件，无法移动！\n源路径：" + SourcePath.string());
        return false;
    }
    //检查目标文件夹是否存在
    if (!fs::exists(Target)) {
        this->LogError("目标文件夹不存在，无法移动文件！\n目标路径：" + Target.string());
        return false;
    }
    //检查目标是否为目录
    if (!fs::is_directory(Target)) {
        this->LogError("目标路径不是目录，无法移动文件！\n目标路径：" + Target.string());
        return false;
    }
    //如果目标文件已存在，先删除它（执行覆盖操作）
    if (fs::exists(TargetPath)) {
        try {
            if (!fs::remove(TargetPath)) {
                this->LogError("无法删除已存在的目标文件，移动失败！\n目标路径：" + TargetPath.string());
                return false;
            }
        }
        catch (const fs::filesystem_error& e) {
            this->LogError("删除已存在目标文件时出错，错误信息：" + std::string(e.what()));
            return false;
        }
    }
    //移动文件
    try {
        fs::rename(SourcePath, TargetPath);
    }
    catch (const fs::filesystem_error& e) {
        //如果rename失败（可能因为跨卷移动），尝试使用copy+remove
        try {
            //复制文件（覆盖已存在的目标文件）
            fs::copy(SourcePath, TargetPath, fs::copy_options::overwrite_existing);
            //验证目标文件是否创建成功
            if (!fs::exists(TargetPath)) {
                this->LogError("文件复制成功但目标文件未找到，移动失败！");
                return false;
            }
            //删除源文件
            if (!fs::remove(SourcePath)) {
                this->LogWarning("文件复制成功但源文件删除失败，移动不完整！\n源文件路径：" + SourcePath.string());
                //这里仍然返回true，因为文件已成功复制到目标位置（覆盖了已存在的文件）
            }
        }
        catch (const fs::filesystem_error& copy_error) {
            this->LogError("文件移动失败，错误信息：" + std::string(copy_error.what()));
            return false;
        }
    }
    this->LogSucc("文件移动成功！\n从：" + SourcePath.string() + "\n到：" + TargetPath.string());
    return true;
}