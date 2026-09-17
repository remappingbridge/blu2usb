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
