cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(pdbridge)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(pdbridge.elf
	main.c
)

target_link_libraries(pdbridge.elf
	SceDipswForDriver_stub
	SceModulemgrForDriver_stub
	SceSysrootForKernel_stub
	SceDebugForDriver_stub
	SceSysclibForDriver_stub
)

set_target_properties(pdbridge.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(pdbridge.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/pdbridge.yml pdbridge.elf pdbridge.velf
	COMMAND vita-make-fself -c pdbridge.velf pdbridge.skprx
)

add_dependencies(pdbridge.skprx pdbridge.elf)

vita_create_stubs(stubs pdbridge.elf ${CMAKE_SOURCE_DIR}/pdbridge.yml KERNEL)