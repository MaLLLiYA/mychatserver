# 定义SRC_LIST存放当前目录的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 配置由哪些源文件生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 配置要链接的库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)