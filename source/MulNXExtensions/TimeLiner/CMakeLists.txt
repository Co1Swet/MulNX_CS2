add_library(TimeLiner
    TimeLiner.cpp
    FlowClock/FlowClock.cpp
)

target_link_libraries(TimeLiner PUBLIC
    MulNX
)