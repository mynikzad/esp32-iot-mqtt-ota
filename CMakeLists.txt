cmake_minimum_required(VERSION 3.16)

set(EXTRA_KCONFIG_FILES ${CMAKE_CURRENT_LIST_DIR}/Kconfig.projbuild)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(PARTITION_TABLE_FILENAME partitions.csv)

project(project_name)
