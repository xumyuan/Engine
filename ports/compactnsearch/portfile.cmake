vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO    InteractiveComputerGraphics/CompactNSearch
    REF     f6c0f32f3946554c91561cbaec4cfe8be0c23430
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

# ============================================================================
# 生成 CMake config 文件，让 find_package(CompactNSearch CONFIG) 正常工作
# ============================================================================
file(WRITE "${CURRENT_PACKAGES_DIR}/share/compactnsearch/CompactNSearchConfig.cmake"
[=[
include(CMakeFindDependencyMacro)

if(NOT TARGET CompactNSearch::CompactNSearch)
    add_library(CompactNSearch::CompactNSearch STATIC IMPORTED)

    find_path(_cns_inc NAMES CompactNSearch.h
        PATHS "${CMAKE_CURRENT_LIST_DIR}/../../include"
        NO_DEFAULT_PATH
    )
    set_target_properties(CompactNSearch::CompactNSearch PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_cns_inc}"
    )

    # Release
    find_library(_cns_lib_release NAMES CompactNSearch
        PATHS "${CMAKE_CURRENT_LIST_DIR}/../../lib"
        NO_DEFAULT_PATH
    )
    if(_cns_lib_release)
        set_property(TARGET CompactNSearch::CompactNSearch APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(CompactNSearch::CompactNSearch PROPERTIES
            IMPORTED_LOCATION_RELEASE "${_cns_lib_release}"
        )
    endif()

    # Debug
    find_library(_cns_lib_debug NAMES CompactNSearch_d CompactNSearch
        PATHS "${CMAKE_CURRENT_LIST_DIR}/../../debug/lib"
        NO_DEFAULT_PATH
    )
    if(_cns_lib_debug)
        set_property(TARGET CompactNSearch::CompactNSearch APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(CompactNSearch::CompactNSearch PROPERTIES
            IMPORTED_LOCATION_DEBUG "${_cns_lib_debug}"
        )
    endif()

    unset(_cns_inc)
    unset(_cns_lib_release)
    unset(_cns_lib_debug)
endif()
]=]
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
