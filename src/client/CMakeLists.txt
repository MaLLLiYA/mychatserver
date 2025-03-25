# 定义SRC_LIST存放当前目录的所有源文件
aux_source_directory(. SRC_LIST)

# 配置由哪些源文件生成可执行文件
add_executable(ChatClient ${SRC_LIST})

# 配置要链接的库
target_link_libraries(ChatClient pthread)