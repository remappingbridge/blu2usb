cmake_minimum_required(VERSION 3.20)

option(BLU2USB_BUILD_PICO "Build the Raspberry Pi Pico 2 W production scaffold" OFF)
option(BLU2USB_BUILD_TESTS "Build host tests" ON)

if(BLU2USB_BUILD_PICO)
    if(NOT DEFINED PICO_SDK_PATH OR PICO_SDK_PATH STREQUAL "")
        message(FATAL_ERROR "PICO_SDK_PATH is required when BLU2USB_BUILD_PICO=ON")
    endif()
    include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
endif()

project(blu2usb_picow VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(BLU2USB_BUILD_PICO)
    if(NOT PICO_BOARD STREQUAL "pico2_w")
        message(FATAL_ERROR "Production scaffold is intentionally fixed to PICO_BOARD=pico2_w")
    endif()
    pico_sdk_init()
endif()

include(cmake/Blu2UsbModules.cmake)
blu2usb_declare_contract_modules()

add_library(blu2usb_contract STATIC
    src/domain/version.c
)
target_include_directories(blu2usb_contract PUBLIC include)
target_compile_definitions(blu2usb_contract PUBLIC
    BLU2USB_VERSION_STRING="0.1.0-g01"
)

if(BLU2USB_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_executable(blu2usb_test_bootstrap tests/test_bootstrap.c)
    target_link_libraries(blu2usb_test_bootstrap PRIVATE blu2usb_contract)
    add_test(NAME bootstrap_contract COMMAND blu2usb_test_bootstrap)

    add_test(
        NAME architecture_contract
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_architecture.py
                ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(BLU2USB_BUILD_PICO)
    add_executable(blu2usb_picow
        src/app/main.c
    )
    target_link_libraries(blu2usb_picow PRIVATE
        pico_stdlib
        blu2usb_contract
        blu2usb_module_app
    )

    # Production firmware deliberately has no stdio/debug USB or UART surface.
    pico_enable_stdio_usb(blu2usb_picow 0)
    pico_enable_stdio_uart(blu2usb_picow 0)
    pico_add_extra_outputs(blu2usb_picow)
endif()
