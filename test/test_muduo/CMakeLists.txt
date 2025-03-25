# 配置头文件搜索路径
# include_directories()

# 配置库文件搜索路径
# link_directories()

# 配置要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

# 配置可执行文件的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(server ${SRC_LIST})

# 配置要链接的库
target_link_libraries(server muduo_net muduo_base pthread)