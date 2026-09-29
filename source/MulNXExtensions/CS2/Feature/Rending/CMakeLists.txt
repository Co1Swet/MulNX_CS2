add_library(CS2Rending STATIC
    CloudController/CloudController.cpp

)
target_link_libraries(CS2Rending PUBLIC
    CS2Support
)