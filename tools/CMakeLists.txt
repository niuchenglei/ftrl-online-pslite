cmake_minimum_required(VERSION 2.6)
project(ftrl)


#############################################
# OpenMP support
option(USE_OPENMP "use openmp as multi-thread implement" ON)

# Compiler variables
set(BUILD_SHARED_LIBS ON)
set(BUILD_DEPENDENCE_DIR ${PROJECT_SOURCE_DIR}/../deps/lib)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-w -g -W -Wall -fopenmp -std=c++11")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_INSTALL_PREFIX $HOME/${PROJECT_NAME})
set(CMAKE_SHARED_LINKER_FLAGS "-L${BUILD_DEPENDENCE_DIR} -L/usr/local/lib -L/home/w/lib64/")
set(CMAKE_EXE_LINKER_FLAGS "-L${BUILD_DEPENDENCE_DIR} -L/usr/local/lib -L/home/w/lib64/ -lstdc++")
#set(CMAKE_CXX_COMPILER /bin/gcc)

# Compiler options
#add_definitions(-DOMP)
#add_definitions(-DPROF)

#############################################
# Compile file option

add_subdirectory(optimizer)
add_subdirectory(models)
add_subdirectory(utils)
# Get whole source files that need to compile
aux_source_directory(. SOURCE_FILES)

# Compile application to executable program
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_library(protobuf STATIC IMPORTED)
set_property(TARGET protobuf PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/../deps/lib/libprotobuf.a)
add_library(rdkafka STATIC IMPORTED)
set_property(TARGET rdkafka PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/../deps/lib/librdkafka.a)
add_library(ps STATIC IMPORTED)
set_property(TARGET ps PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/../deps/lib/libps.a)
add_library(zmq STATIC IMPORTED)
set_property(TARGET zmq PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/../deps/lib/libzmq.a)
#############################################
add_library(log4cpp STATIC IMPORTED)
set_property(TARGET log4cpp PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/../deps/lib/liblog4cpp.a)

#############################################
# Dependence library setting
target_link_libraries(${PROJECT_NAME} model optim util
pthread
protobuf
ps
zmq
rdkafka
log4cpp
-lprotobuf
-lps
-lrdkafka
-lzmq
-llog4cpp
-lhiredis
dl
)

# Include path
include_directories(
${PROJECT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}/../deps/include
/usr/include
/usr/local/include/
)
#############################################

