cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(bgvpk_sample)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME ${PROJECT_NAME})
set(VITA_TITLEID  "SKGSMPL3E")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS} -a 0x2808000000000000")

add_executable(${PROJECT_NAME}
  main.c
  graphics.c
  font.c
)

target_link_libraries(${PROJECT_NAME}
	SceAppMgr_stub
	SceDisplay_stub
	SceKernelModulemgr_stub
	SceKernelThreadMgr_stub
	taihen_stub
	c
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  FILE ../bgvpk.suprx bgvpk.suprx
  FILE sample_icon.png sample_icon.png
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
