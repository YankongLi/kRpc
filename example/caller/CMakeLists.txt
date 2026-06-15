set(SRC_LIST calluserservice.cc ../user.pb.cc)
add_executable(consumer ${SRC_LIST})

# 1. 告诉 CMake 去寻找系统中安装的 Protobuf 包 (如果顶层 CMakeLists 已经写了这句，这里可以省略)
find_package(protobuf CONFIG REQUIRED)

# 2. 修改这里：不要只写 protobuf，而是使用 CMake 的目标形式 protobuf::libprotobuf
# 这样 CMake 会自动把 protobuf 依赖的 absl 库全部链接进来
target_link_libraries(consumer kRpc protobuf::libprotobuf)