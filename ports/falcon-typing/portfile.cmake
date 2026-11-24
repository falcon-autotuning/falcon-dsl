vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO falcon-autotuning/falcon-typing
    REF v${VERSION}
    SHA512 0eaaa47f4640c8d4c341228e3b457bd776bfaaf6bc04db14d9907e63a816efc5eda35abe606e7f3fd8cf557b376cfdc49aabff6f3a7db8d3a23c09787c5f7145
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/LICENSE"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
     RENAME copyright)

vcpkg_copy_pdbs()
