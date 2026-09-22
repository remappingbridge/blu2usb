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


add_executable(blu2usb_test_ux_v1 test_ux_v1.c)
target_link_libraries(blu2usb_test_ux_v1 PRIVATE blu2usb_renderer blu2usb_profiles blu2usb_storage blu2usb_ble_hogp)
add_test(NAME experimental_ux_v1 COMMAND blu2usb_test_ux_v1)

set(BLU2USB_BTSTACK_TEST_PATH "" CACHE PATH "Pinned BTstack source for production radio lifecycle test")
if(BLU2USB_BTSTACK_TEST_PATH)
    add_executable(blu2usb_test_radio_lifecycle
        test_radio_lifecycle.c
        ../src/ble_hogp/ble_hogp_pico.c
        ${BLU2USB_BTSTACK_TEST_PATH}/src/ad_parser.c
        ${BLU2USB_BTSTACK_TEST_PATH}/src/btstack_util.c)
    target_include_directories(blu2usb_test_radio_lifecycle PRIVATE
        radio_stubs ${BLU2USB_BTSTACK_TEST_PATH}/src)
    target_compile_definitions(blu2usb_test_radio_lifecycle PRIVATE ENABLE_BLE)
    target_link_libraries(blu2usb_test_radio_lifecycle PRIVATE blu2usb_ble_hogp blu2usb_bt_runtime)
    add_test(NAME production_radio_lifecycle COMMAND blu2usb_test_radio_lifecycle)
endif()
