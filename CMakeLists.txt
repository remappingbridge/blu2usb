cmake_minimum_required(VERSION 3.20)

option(BLU2USB_BUILD_PICO "Build the Raspberry Pi Pico 2 W production firmware" OFF)
option(BLU2USB_BUILD_TESTS "Build host tests" ON)

if(BLU2USB_BUILD_PICO)
    if((NOT DEFINED PICO_SDK_PATH OR PICO_SDK_PATH STREQUAL "") AND
       DEFINED ENV{PICO_SDK_PATH} AND NOT "$ENV{PICO_SDK_PATH}" STREQUAL "")
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to Raspberry Pi Pico SDK")
    endif()
    if(NOT DEFINED PICO_SDK_PATH OR PICO_SDK_PATH STREQUAL "")
        message(FATAL_ERROR "PICO_SDK_PATH is required when BLU2USB_BUILD_PICO=ON")
    endif()
    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
endif()

project(blu2usb_picow VERSION 0.3.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(BLU2USB_BUILD_PICO)
    if(NOT PICO_BOARD STREQUAL "pico2_w")
        message(FATAL_ERROR "Production firmware is intentionally fixed to PICO_BOARD=pico2_w")
    endif()
    pico_sdk_init()
endif()

include(cmake/Blu2UsbModules.cmake)
blu2usb_declare_contract_modules()

add_library(blu2usb_contract STATIC src/domain/version.c)
target_include_directories(blu2usb_contract PUBLIC include)
target_compile_definitions(blu2usb_contract PUBLIC BLU2USB_VERSION_STRING="0.3.0-g03")

add_library(blu2usb_profiles STATIC src/profiles/custom_template.c)
target_include_directories(blu2usb_profiles PUBLIC include)

add_library(blu2usb_interaction STATIC src/interaction/interaction.c)
target_include_directories(blu2usb_interaction PUBLIC include)

add_library(blu2usb_ux_model STATIC src/ux_model/ux_model.c)
target_include_directories(blu2usb_ux_model PUBLIC include)
target_link_libraries(blu2usb_ux_model PUBLIC blu2usb_interaction)

add_library(blu2usb_renderer STATIC src/renderer/renderer.c)
target_include_directories(blu2usb_renderer PUBLIC include)
target_link_libraries(blu2usb_renderer PUBLIC blu2usb_ux_model)

add_library(blu2usb_hat STATIC src/hat/hat.c)
target_include_directories(blu2usb_hat PUBLIC include)

if(BLU2USB_BUILD_PICO)
    target_sources(blu2usb_renderer PRIVATE src/renderer/st7789_pico.c)
    target_link_libraries(blu2usb_renderer PUBLIC pico_stdlib hardware_gpio hardware_spi)
    target_sources(blu2usb_hat PRIVATE src/hat/hat_pico.c)
    target_link_libraries(blu2usb_hat PUBLIC pico_stdlib hardware_gpio)
endif()

# Attach implemented libraries to the frozen architecture facade targets.
target_link_libraries(blu2usb_module_profiles INTERFACE blu2usb_profiles)
target_link_libraries(blu2usb_module_interaction INTERFACE blu2usb_interaction)
target_link_libraries(blu2usb_module_ux_model INTERFACE blu2usb_ux_model)
target_link_libraries(blu2usb_module_renderer INTERFACE blu2usb_renderer)
target_link_libraries(blu2usb_module_hat INTERFACE blu2usb_hat)

if(BLU2USB_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_executable(blu2usb_test_bootstrap tests/test_bootstrap.c)
    target_link_libraries(blu2usb_test_bootstrap PRIVATE blu2usb_contract)
    add_test(NAME bootstrap_contract COMMAND blu2usb_test_bootstrap)

    add_subdirectory(tests)

    add_test(
        NAME architecture_contract
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_architecture.py
                ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(BLU2USB_BUILD_PICO)
    add_executable(blu2usb_picow src/app/main.c)
    target_link_libraries(blu2usb_picow PRIVATE
        pico_stdlib
        blu2usb_contract
        blu2usb_module_app
    )

    pico_enable_stdio_usb(blu2usb_picow 0)
    pico_enable_stdio_uart(blu2usb_picow 0)
    pico_add_extra_outputs(blu2usb_picow)
endif()
