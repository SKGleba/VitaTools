cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(vlog)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

add_executable(vlog.elf
	font.c
	blit.c
	main.c
)

target_link_libraries(vlog.elf
	SceThreadmgrForDriver_stub
	SceSysclibForDriver_stub
	SceCpuForDriver_stub
	SceModulemgrForDriver_stub
	ScePervasiveForDriver_stub
	SceSysmemForDriver_stub
	ScePowerForDriver_stub
	SceDisplayForDriver_stub
	SceDebugForDriver_stub
	SceSysrootForKernel_stub
)

set_target_properties(vlog.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(vlog.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/vlog.yml vlog.elf vlog.velf
	COMMAND vita-make-fself -c vlog.velf vlog.skprx
)

add_dependencies(vlog.skprx vlog.elf)
