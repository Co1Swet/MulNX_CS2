cmake_minimum_required(VERSION 3.31)

# 项目定义
project("MulNX_CS2")

# 版本定义
add_compile_definitions(MulNXVersion="Beta1.7")

# 启用 LTO（仅 Release 配置）
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# 添加编译选项
add_compile_options(/MP)
add_compile_options(/utf-8)
add_compile_options(/EHsc)
add_compile_options(/Zc:__cplusplus) # 让MSVC标注正确的cpp版本

add_compile_options(/Gy) # 函数级链接，配合 /OPT:REF 移除未用函数
add_compile_options(/Gw) # 全局数据优化，允许移除未引用的全局变量
add_compile_options(/GF) # 合并重复字符串常量

# 控制 PDB 文件
string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
string(APPEND CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL " /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
string(APPEND CMAKE_EXE_LINKER_FLAGS_MINSIZEREL " /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")

# 定义全局宏
add_compile_definitions(NOMINMAX) # 防止windows的min max宏破坏程序
add_compile_definitions(WIN32_LEAN_AND_MEAN) # 至少能让它少引入点东西
add_compile_definitions(ASIO_STANDALONE) # 独立版本的asio，不引入boost
add_compile_definitions(MULNX_STRICT) # MulNX的严格模式宏

add_subdirectory("source")

# 定义目标路径
set(MulNXPath ${CMAKE_SOURCE_DIR}/bin/MulNX)
set(CS2OBToolPath ${MulNXPath}/CS2OBTool)
set(CS2InjectorPath ${MulNXPath}/CS2Injector)
set(CS2InternalHelper ${MulNXPath}/CS2InternalHelper)

# 处理 CS2Injector 目标
set_target_properties(CS2Injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CS2InjectorPath}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CS2InjectorPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CS2InjectorPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CS2InjectorPath}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CS2InjectorPath}"
)
# 处理 CS2OBTool 目标
set_target_properties(CS2OBTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CS2OBToolPath}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CS2OBToolPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CS2OBToolPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CS2OBToolPath}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CS2OBToolPath}"
)