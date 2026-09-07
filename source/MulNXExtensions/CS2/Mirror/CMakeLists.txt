add_library(CS2Mirror STATIC
    ParticleManager/ParticleManager.cpp
    SceneSystem/SceneSystem.cpp
    MaterialSystem/MaterialSystem.cpp
    CS2Hash/CS2Hash.cpp
    ResourceSystem/ResourceSystem.cpp
    ParticleSystemMgr/ParticleSystemMgr.cpp
)
target_link_libraries(CS2Mirror PUBLIC CS2Intro)