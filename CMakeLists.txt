cmake_minimum_required(VERSION 3.15)

project(ZKDS) # zkview data source

set(CMAKE_CXX_STANDARD 11)

#cmake在编译过程中打印编印信息
if(NOT MSVC)
    set(CMAKE_VERBOSE_MAKEFILEON ON) 
    set(CMAKE_CXX_FLAGS "-Wno-error=deprecated-declarations -Wno-deprecated-declarations")
else()
    add_compile_options(/W1)
endif()


# 设置输出bin文件路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# src 链接库
add_subdirectory(${CMAKE_SOURCE_DIR}/websocketServer)
add_subdirectory(${CMAKE_SOURCE_DIR}/common)

set(EXTRA_LIBS ${EXTRA_LIBS}  WebSocket common)

add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp)
# 链接动态链接库
target_link_libraries(${PROJECT_NAME} ${LD_TRT_LIBS} ${LD_CUDA_LIBS} ${OpenCV_LIBS} ${EXTRA_LIBS} ${FFMEPG_LIB})
