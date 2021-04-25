cmake_minimum_required(VERSION 3.17)
project(microshell)
set(CMAKE_C_STANDARD 99)

add_executable(${PROJECT_NAME} microshell.c)

# From GIT
#add_executable(${PROJECT_NAME} from_github/microshell.c)
