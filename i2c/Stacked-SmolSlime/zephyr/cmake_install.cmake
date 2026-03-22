# Install script for directory: C:/ncs/v3.2.4/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/ncs/toolchains/fd21892d0f/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/arch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/soc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/boards/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/subsys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/drivers/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/nrf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/hostap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/mcuboot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/trusted-firmware-m/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cjson/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/azure-sdk-for-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cirrus-logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/memfault-firmware-sdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/canopennode/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/chre/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/lz4/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/zscilib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cmsis/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cmsis-dsp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cmsis-nn/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/cmsis_6/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/hal_nordic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/hal_st/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/hal_tdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/hal_wurthelektronik/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/liblc3/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/libmetal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/littlefs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/loramac-node/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/lvgl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/mipi-sys-t/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/nanopb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/nrf_wifi/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/open-amp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/percepio/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/picolibc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/segger/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/uoscore-uedhoc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/zcbor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/nrfxlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/nrf_hw_models/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/modules/connectedhomeip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/kernel/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/cmake/flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/cmake/usage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/Lyall/Documents/GitHub/Stacked-SmolSlime/i2c/Stacked-SmolSlime/zephyr/cmake/reports/cmake_install.cmake")
endif()

