cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(bgvpk)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(bgvpk
  main.c
  libc2paf.c
  Archives.c
  sha1.c
)

target_link_libraries(bgvpk
  SceIofilemgr_stub
  SceLibKernel_stub
  taihen_stub
  SceSysmodule_stub
  ScePaf_stub_weak
  ScePromoterUtil_stub_weak
  SceKernelThreadMgr_stub
  SceAppMgr_stub
  SceShellSvc_stub_weak
  SceIncomingDialog_stub_weak
  z
)

vita_create_self(bgvpk.suprx bgvpk CONFIG exports.yml UNSAFE)