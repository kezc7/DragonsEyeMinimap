# Get port from Github
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/CommonLibVR
    REF e617713b2ae8a927bf925d1ad138cc48ab72e414 # 22/09/24
    SHA512 4dfc7e18ebf1a1778027b4e5ffb3b681092dd9d876ade0a0a5f564be62806c4191e0914d1090c1b953c43003d76c2367b68dcc746ae44e56cd77c96ebeb0d425
    HEAD_REF ng
)

# Get submodule and copy to extern/ folder (done manually because Vcpkg does not support Git submodules)
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH_OPENVR
    REPO ValveSoftware/openvr
    REF ebdea152f8aac77e9a6db29682b81d762159df7e # 01/09/17
    SHA512 4fb668d933ac5b73eb4e97eb29816176e500a4eaebe2480cd0411c95edfb713d58312036f15db50884a2ef5f4ca44859e108dec2b982af9163cefcfc02531f63
    HEAD_REF master
)
 file(COPY "${SOURCE_PATH_OPENVR}/" DESTINATION "${SOURCE_PATH}/extern/openvr")
 file(REMOVE_RECURSE "${SOURCE_PATH_OPENVR}/")

 # Configure options to build
vcpkg_configure_cmake(
        SOURCE_PATH "${SOURCE_PATH}"
        PREFER_NINJA
        OPTIONS -DBUILD_TESTS=off -DSKSE_SUPPORT_XBYAK=on
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake)
vcpkg_copy_pdbs()

file(INSTALL "${SOURCE_PATH}/extern/openvr/headers/openvr.h" DESTINATION ${CURRENT_PACKAGES_DIR}/include)
file(GLOB CMAKE_CONFIGS "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE/*.cmake")
file(INSTALL ${CMAKE_CONFIGS} DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
