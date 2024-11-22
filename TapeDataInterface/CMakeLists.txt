cmake_minimum_required(VERSION 3.0)

project(tapedatainterface)

add_executable(tapedatainterface main.cpp)

install(TARGETS tapedatainterface RUNTIME DESTINATION bin)
