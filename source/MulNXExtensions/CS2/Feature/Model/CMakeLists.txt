add_library(CS2Model STATIC
    ViewModelController/ViewModelController.cpp
    SkinController/SkinController.cpp
    SkinConfigger/SkinConfigger.cpp
)
target_link_libraries(CS2Model PUBLIC
    CS2Support
)