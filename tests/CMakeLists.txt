add_compile_options(-UNDEBUG)

add_executable(blu2usb_test_g07_keyboard test_g07_keyboard.c)
target_link_libraries(blu2usb_test_g07_keyboard PRIVATE
    blu2usb_keyboard_transport blu2usb_hid_aggregator blu2usb_renderer blu2usb_ux_model)
add_test(NAME g07_keyboard_ownership_ui COMMAND blu2usb_test_g07_keyboard)

set(BLU2USB_BTSTACK_ROOT "" CACHE PATH "Pinned BTstack source for adapter regression tests")
if(BLU2USB_BTSTACK_ROOT)
    execute_process(COMMAND git -C "${BLU2USB_BTSTACK_ROOT}" rev-parse HEAD
                    OUTPUT_VARIABLE btstack_revision OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT btstack_revision STREQUAL "501e6d2b86e6c92bfb9c390bcf55709938e25ac1")
        message(FATAL_ERROR "G07 tests require the Pico SDK 2.2.0 BTstack revision")
    endif()
    add_executable(blu2usb_test_g07_classic
        test_g07_classic_adapter.c
        ../src/classic_hid/classic_hid_pico.c
        ../src/classic_hid/classic_hid_parser.c
        "${BLU2USB_BTSTACK_ROOT}/src/btstack_hid_parser.c"
        "${BLU2USB_BTSTACK_ROOT}/src/btstack_util.c"
    )
    target_include_directories(blu2usb_test_g07_classic PRIVATE
        ../include "${BLU2USB_BTSTACK_ROOT}/src"
        "${BLU2USB_BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include"
        "${BLU2USB_BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include"
        "${BLU2USB_BTSTACK_ROOT}/3rd-party/yxml")
    target_compile_definitions(blu2usb_test_g07_classic PRIVATE ENABLE_CLASSIC=1 ENABLE_BLE=1)
    target_compile_options(blu2usb_test_g07_classic PRIVATE -Wall -Wextra -Werror
                           -ffunction-sections -fdata-sections)
    target_link_options(blu2usb_test_g07_classic PRIVATE -Wl,--gc-sections)
    target_link_libraries(blu2usb_test_g07_classic PRIVATE blu2usb_bt_runtime blu2usb_keyboard_transport)
    foreach(scenario success cancel bond-failure timeout overflow)
        add_test(NAME g07_classic_${scenario} COMMAND blu2usb_test_g07_classic ${scenario})
    endforeach()
    add_test(NAME g07_synchronous_start_negative_control
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_g07_negative_control.py
                ${PROJECT_SOURCE_DIR} ${BLU2USB_BTSTACK_ROOT})
endif()

add_executable(blu2usb_test_g02 test_g02.c)
target_link_libraries(blu2usb_test_g02 PRIVATE
    blu2usb_profiles
    blu2usb_ux_model
)
add_test(NAME g02_interaction_ux COMMAND blu2usb_test_g02)

add_executable(blu2usb_test_g03_renderer test_g03_renderer.c)
target_link_libraries(blu2usb_test_g03_renderer PRIVATE
    blu2usb_renderer
    blu2usb_hat
    blu2usb_ux_model
)
add_test(NAME g03_renderer_hat COMMAND blu2usb_test_g03_renderer)

add_executable(blu2usb_test_g04_usb test_g04_usb.c)
target_link_libraries(blu2usb_test_g04_usb PRIVATE
    blu2usb_usb_hid
)
add_test(NAME g04_fixed_usb_hid COMMAND blu2usb_test_g04_usb)

add_executable(blu2usb_test_g05_hid_ownership test_g05_hid_ownership.c)
target_link_libraries(blu2usb_test_g05_hid_ownership PRIVATE
    blu2usb_hid_aggregator
)
add_test(NAME g05_canonical_hid_ownership COMMAND blu2usb_test_g05_hid_ownership)

add_executable(blu2usb_test_g05_ble_mouse test_g05_ble_mouse.c)
target_link_libraries(blu2usb_test_g05_ble_mouse PRIVATE
    blu2usb_ble_hogp
    blu2usb_bt_runtime
    blu2usb_hid_aggregator
    blu2usb_usb_hid
)
add_test(NAME g05_ble_hogp_mouse_passthrough COMMAND blu2usb_test_g05_ble_mouse)

add_executable(blu2usb_test_g06_profiles_remap_hidpp test_g06_profiles_remap_hidpp.c)
target_link_libraries(blu2usb_test_g06_profiles_remap_hidpp PRIVATE
    blu2usb_profiles
    blu2usb_remap
    blu2usb_hid_aggregator
    blu2usb_logitech_hidpp
)
add_test(NAME g06_profiles_remap_hidpp COMMAND blu2usb_test_g06_profiles_remap_hidpp)

add_executable(blu2usb_test_g06_storage test_g06_storage.c)
target_link_libraries(blu2usb_test_g06_storage PRIVATE
    blu2usb_profiles
    blu2usb_storage
)
add_test(NAME g06_product_persistence COMMAND blu2usb_test_g06_storage)

add_executable(blu2usb_test_g06_profile_ui test_g06_profile_ui.c)
target_link_libraries(blu2usb_test_g06_profile_ui PRIVATE
    blu2usb_renderer
    blu2usb_ux_model
)
add_test(NAME g06_profile_ui_regressions COMMAND blu2usb_test_g06_profile_ui)

add_test(
    NAME g06_architecture
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_g06_architecture.py
            ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME screen_contract
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_screen_contract.py
            ${PROJECT_SOURCE_DIR}
)
