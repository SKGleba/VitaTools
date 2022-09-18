cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(devdump)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_executable(devdump.elf
	main.c
	f00d.c
	nvs.c
	dev.c
)

target_link_libraries(devdump.elf
	SceKernelSuspendForDriver_stub
	SceSysclibForDriver_stub
	SceSysmemForDriver_stub
	SceDebugForDriver_stub
	SceSysconForDriver_stub
	SceSblSmCommForKernel_stub
	SceIofilemgrForDriver_stub
	SceSdifForDriver_stub
	SceMsifForDriver_stub
	SceSysrootForKernel_stub
)

set_target_properties(devdump.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(devdump.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/devdump.yml devdump.elf devdump.velf
	COMMAND vita-make-fself -c devdump.velf devdump.skprx
)
add_dependencies(devdump.skprx devdump.elf)