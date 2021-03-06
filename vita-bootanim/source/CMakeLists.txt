﻿cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME vbanim)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "vbanim installer")
set(VITA_TITLEID  "SKGVBANIM")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fno-builtin-printf")
if (DEFINED SAFETY)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
endif()
set(IGNORE_WARNING "${SAFETY}")

include_directories(${PROJECT_BINARY_DIR})

include_directories(
  installer
)

add_executable(${SHORT_NAME}
  installer/main.c
  installer/graphics.c
  installer/font.c
)

target_link_libraries(${SHORT_NAME}
  SceDisplay_stub
  SceShellSvc_stub
  SceProcessmgr_stub
  SceSysmodule_stub
  SceVshBridge_stub
  SceCtrl_stub
  SceAppMgr_stub
)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_BINARY_DIR}/kernel/vbanim.skprx vbanim.skprx
  FILE ${CMAKE_BINARY_DIR}/user/vbanim.suprx vbanim.suprx
  FILE ${CMAKE_BINARY_DIR}/res/boot.rcf boot.rcf
  FILE ${CMAKE_BINARY_DIR}/res/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_BINARY_DIR}/res/template.xml sce_sys/livearea/contents/template.xml
  FILE ${CMAKE_BINARY_DIR}/res/bg.png sce_sys/livearea/contents/bg.png
)
