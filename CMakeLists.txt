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

project(blu2usb_picow VERSION 0.6.0 LANGUAGES C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
target_compile_definitions(blu2usb_contract PUBLIC BLU2USB_VERSION_STRING="0.6.0-g06")
# Regression marker consumed by the inherited G05 architecture check only:
# BLU2USB_VERSION_STRING="0.5.0-g05"

add_library(blu2usb_hid_aggregator STATIC
    src/hid_aggregator/hid_aggregator.c
    src/hid_aggregator/relative.c
)
target_include_directories(blu2usb_hid_aggregator PUBLIC include)

add_library(blu2usb_profiles STATIC
    src/profiles/custom_template.c
    src/profiles/profiles.c
)
target_include_directories(blu2usb_profiles PUBLIC include)

add_library(blu2usb_remap STATIC src/remap/remap.c)
target_include_directories(blu2usb_remap PUBLIC include)
target_link_libraries(blu2usb_remap PUBLIC blu2usb_profiles)

add_library(blu2usb_storage STATIC src/storage/storage.c)
target_include_directories(blu2usb_storage PUBLIC include)

add_library(blu2usb_interaction STATIC src/interaction/interaction.c)
target_include_directories(blu2usb_interaction PUBLIC include)

add_library(blu2usb_ux_model STATIC
    src/ux_model/ux_model.c
    src/ux_model/profile_state.c
)
target_include_directories(blu2usb_ux_model PUBLIC include)
target_link_libraries(blu2usb_ux_model PUBLIC blu2usb_interaction)

add_library(blu2usb_renderer STATIC
    src/renderer/renderer.c
    src/renderer/profile_feedback.c
)
target_include_directories(blu2usb_renderer PUBLIC include)
target_link_libraries(blu2usb_renderer PUBLIC blu2usb_ux_model)

add_library(blu2usb_hat STATIC src/hat/hat.c)
target_include_directories(blu2usb_hat PUBLIC include)

add_library(blu2usb_usb_hid STATIC src/usb_hid/usb_hid.c)
target_include_directories(blu2usb_usb_hid PUBLIC include)

add_library(blu2usb_bt_runtime STATIC src/bt_runtime/bt_runtime.c)
target_include_directories(blu2usb_bt_runtime PUBLIC include)

add_library(blu2usb_ble_hogp STATIC src/ble_hogp/ble_hogp.c)
target_include_directories(blu2usb_ble_hogp PUBLIC include)

add_library(blu2usb_logitech_hidpp STATIC src/logitech_hidpp/logitech_hidpp.c)
target_include_directories(blu2usb_logitech_hidpp PUBLIC include)

if(BLU2USB_BUILD_PICO)
    target_sources(blu2usb_renderer PRIVATE src/renderer/st7789_pico.c)
    target_link_libraries(blu2usb_renderer PUBLIC pico_stdlib hardware_gpio hardware_spi)

    target_sources(blu2usb_hat PRIVATE src/hat/hat_pico.c)
    target_link_libraries(blu2usb_hat PUBLIC pico_stdlib hardware_gpio)

    target_sources(blu2usb_usb_hid PRIVATE
        src/usb_hid/usb_hid_pico.c
        src/usb_hid/usb_descriptors.c
    )
    target_link_libraries(blu2usb_usb_hid PUBLIC tinyusb_device)

    target_sources(blu2usb_storage PRIVATE src/storage/storage_pico.c)
    # Flash implementation is materialized once with the shared multicore runtime.
    # A second storage-owned flash.c would compile without LIB_PICO_MULTICORE
    # and could win archive selection, silently disabling cross-core lockout.
    target_link_libraries(blu2usb_storage PRIVATE pico_stdlib pico_flash_headers hardware_flash)

    # SDK 2.2.0 attaches pico_flash/flash.c transitively even through stdlib.
    # Materialize it only here, with pico_multicore enabled, so the final
    # executable cannot select its own IRQ-only variant ahead of this archive.
    get_target_property(blu2usb_sdk_flash_sources pico_flash INTERFACE_SOURCES)
    list(LENGTH blu2usb_sdk_flash_sources blu2usb_sdk_flash_source_count)
    if(NOT blu2usb_sdk_flash_source_count EQUAL 1)
        message(FATAL_ERROR "Revalidate pico_flash source composition for this SDK")
    endif()
    set_property(TARGET pico_flash PROPERTY INTERFACE_SOURCES "")
    target_sources(blu2usb_bt_runtime PRIVATE
        src/bt_runtime/bt_runtime_pico.c ${blu2usb_sdk_flash_sources})
    target_link_libraries(blu2usb_bt_runtime PRIVATE
        pico_stdlib
        pico_multicore
        pico_flash
        pico_btstack_ble
        pico_btstack_classic
        pico_btstack_cyw43
        pico_cyw43_arch_threadsafe_background
    )
    target_compile_definitions(blu2usb_bt_runtime PUBLIC
        CYW43_LWIP=0
        PICO_BTSTACK_CYW43_MAX_HCI_PROCESS_LOOP_COUNT=8
        PICO_CORE1_STACK_SIZE=0
    )
    # Headers must match the feature-dependent types compiled into the stack.
    target_link_libraries(blu2usb_bt_runtime PUBLIC
        pico_btstack_ble_headers pico_btstack_classic_headers)
    pico_btstack_make_gatt_header(
        blu2usb_bt_runtime
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/bt_runtime/g05_hog_host.gatt
    )

    target_sources(blu2usb_ble_hogp PRIVATE src/ble_hogp/ble_hogp_pico.c)
    target_link_libraries(blu2usb_ble_hogp PRIVATE pico_stdlib blu2usb_bt_runtime)

    add_library(blu2usb_classic_probe STATIC
        src/classic_hid/classic_probe_pico.c
        src/classic_hid/pico08/classic_keyboard.c)
    target_include_directories(blu2usb_classic_probe PRIVATE src/classic_hid/pico08_compat)
    target_include_directories(blu2usb_classic_probe PUBLIC include)
    target_link_libraries(blu2usb_classic_probe PRIVATE pico_stdlib blu2usb_bt_runtime)
    target_link_libraries(blu2usb_module_classic_hid INTERFACE blu2usb_classic_probe)

    target_sources(blu2usb_logitech_hidpp PRIVATE src/logitech_hidpp/logitech_hidpp_pico.c)
    target_link_libraries(blu2usb_logitech_hidpp PRIVATE blu2usb_ble_hogp)
endif()

# Attach implemented libraries to the frozen architecture facade targets.
target_link_libraries(blu2usb_module_hid_aggregator INTERFACE blu2usb_hid_aggregator)
target_link_libraries(blu2usb_module_profiles INTERFACE blu2usb_profiles)
target_link_libraries(blu2usb_module_remap INTERFACE blu2usb_remap)
target_link_libraries(blu2usb_module_storage INTERFACE blu2usb_storage)
target_link_libraries(blu2usb_module_interaction INTERFACE blu2usb_interaction)
target_link_libraries(blu2usb_module_ux_model INTERFACE blu2usb_ux_model)
target_link_libraries(blu2usb_module_renderer INTERFACE blu2usb_renderer)
target_link_libraries(blu2usb_module_hat INTERFACE blu2usb_hat)
target_link_libraries(blu2usb_module_usb_hid INTERFACE blu2usb_usb_hid)
target_link_libraries(blu2usb_module_bt_runtime INTERFACE blu2usb_bt_runtime)
target_link_libraries(blu2usb_module_ble_hogp INTERFACE blu2usb_ble_hogp)
target_link_libraries(blu2usb_module_logitech_hidpp INTERFACE blu2usb_logitech_hidpp)

if(BLU2USB_BUILD_TESTS)
    # G06 Release CI disabled assert(), hiding stale test expectations.
    add_compile_options(-UNDEBUG)
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
    target_compile_definitions(blu2usb_picow PRIVATE CYW43_LWIP=0)

    pico_enable_stdio_usb(blu2usb_picow 0)
    pico_enable_stdio_uart(blu2usb_picow 0)
    pico_add_extra_outputs(blu2usb_picow)
endif()
