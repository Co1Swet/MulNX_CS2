#pragma once
#include <CameraSystem/ElementManager/ElementConfig.hpp>
#include <CameraSystem/SolutionManager/SolutionConfig.hpp>
#include <CameraSystem/ProjectManager/ProjectConfig.hpp>
#include <string>
#include <filesystem>

class WorkspaceConfig {
public:   
	ElementConfig ElementCfg{};
	SolutionConfig SolutionCfg{};
	ProjectConfig ProjectCfg{};
};

class Workspace {
public:
	std::string Name{};
	//std::vector<std::string> ProjectNames{};

	WorkspaceConfig Config{};

	Workspace(std::string Name) :
		Name(Name) {

	}

	//保存配置文件
    std::pair<bool, std::string> Save(const std::filesystem::path& FolderPath);
};