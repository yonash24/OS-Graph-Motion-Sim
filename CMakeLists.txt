cmake_minimum_required(VERSION 4.1)
project(OS_Graph_Motion_Sim C)

set(CMAKE_C_STANDARD 11)

add_executable(OS_Graph_Motion_Sim
        main.c
        dijkstra.c)
