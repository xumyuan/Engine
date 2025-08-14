vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO    InteractiveComputerGraphics/CompactNSearch
    REF     f6c0f32f3946554c91561cbaec4cfe8be0c23430 # 或 commit hash
    SHA512  e083a118a623a7a8995125716a089cafdf7f7cada458686625168b27e3f9ce09715b4bc25dd2d84717dff5a1b0b69fa57acd4e6512a5a8c1b7aab8aa4d118202
)
vcpkg_replace_string(
    "${SOURCE_PATH}/CMakeLists.txt"
    "add_library(CompactNSearch"
    "add_library(CompactNSearch STATIC"
)


vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_DEMO=OFF
        -DBUILD_AS_SHARED_LIBS=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_CXX_STANDARD=20
        -DUSE_DOUBLE_PRECISION=OFF
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")


