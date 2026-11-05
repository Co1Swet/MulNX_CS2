add_library(TimeLiner
    TimeLiner.cpp
    FlowClock/FlowClock.cpp
    TimeMarker/TimeMarkerManager.cpp
)

target_link_libraries(TimeLiner PUBLIC
    MulNX
)