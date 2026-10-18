add_library(CS2Model STATIC
    ViewModelController/ViewModelController.cpp
)
target_link_libraries(CS2Model PUBLIC
    CS2Support
)