# 以当前目录为唯一的起点，被所有子目录共享
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加子目录
add_subdirectory("MulNXThirdParty")
add_subdirectory("MulNX")
add_subdirectory("MulNXExtensions")
add_subdirectory("CS2OBTool")
add_subdirectory("CS2Injector")
add_subdirectory("CS2InternalHelper")
add_subdirectory("tests")