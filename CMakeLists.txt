cmake_minimum_required(VERSION 3.20)
project(arc VERSION 0.1 LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(tomlplusplus REQUIRED)
find_package(libgit2 REQUIRED)
find_package(fmt REQUIRED)

add_library(arc_lib STATIC
  cmds/CmdHelp.cpp
  cmds/CmdDefault.cpp
  cmds/CmdSession.cpp
  cmds/CmdWorkTrees.cpp
)

target_include_directories(arc_lib PUBLIC ${CMAKE_SOURCE_DIR})

target_compile_definitions(arc_lib PUBLIC ARC_VERSION="${PROJECT_VERSION}")

target_link_libraries(arc_lib PUBLIC Boost::program_options tomlplusplus::tomlplusplus libgit2::libgit2package fmt::fmt)

add_executable(arc arc.cc)

target_link_libraries(arc PRIVATE arc_lib)

option(ARC_TEST "Build tests" OFF)

if(ARC_TEST)
  find_package(GTest REQUIRED)

  add_executable(arc_test
    cmds/CmdHelp-test.cc
    cmds/CmdDefault-test.cc
    cmds/CmdStart-test.cc
    cmds/CmdClose-test.cc
    cmds/CmdSend-test.cc
    cmds/CmdSession-test.cc
    cmds/CmdWorkTrees-test.cc
  )

  target_link_libraries(arc_test
    PRIVATE
      arc_lib
      GTest::gtest_main
      GTest::gmock
  )

  include(GoogleTest)
  gtest_discover_tests(arc_test)
endif()
