file(GLOB_RECURSE DYNI2C_SOURCES "src/*.cpp")

idf_component_register(
    SRCS ${DYNI2C_SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES driver
)