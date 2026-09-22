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

add_executable(blu2usb_test_mux01_device_registry test_mux01_device_registry.c)
target_link_libraries(blu2usb_test_mux01_device_registry PRIVATE
    blu2usb_device_registry
)
add_test(NAME mux01_device_registry COMMAND blu2usb_test_mux01_device_registry)

add_test(
    NAME mux01_architecture
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mux01_architecture.py
            ${PROJECT_SOURCE_DIR}
)

add_executable(blu2usb_test_mux02_connection_coordinator test_mux02_connection_coordinator.c)
target_link_libraries(blu2usb_test_mux02_connection_coordinator PRIVATE
    blu2usb_connection_coordinator
)
add_test(NAME mux02_connection_coordinator COMMAND blu2usb_test_mux02_connection_coordinator)

add_test(
    NAME mux02_architecture
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mux02_architecture.py
            ${PROJECT_SOURCE_DIR}
)

add_executable(blu2usb_test_mux03_ui_v1 test_mux03_ui_v1.c)
target_link_libraries(blu2usb_test_mux03_ui_v1 PRIVATE
    blu2usb_ux_model
    blu2usb_renderer
    blu2usb_device_registry
)
add_test(NAME mux03_ui_v1_navigation COMMAND blu2usb_test_mux03_ui_v1)

add_executable(blu2usb_test_mux03_projection test_mux03_projection.c)
target_link_libraries(blu2usb_test_mux03_projection PRIVATE
    blu2usb_ux_model
    blu2usb_renderer
    blu2usb_device_registry
)
target_compile_definitions(blu2usb_test_mux03_projection PRIVATE
    BLU2USB_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
add_test(NAME mux03_ui_v1_projection COMMAND blu2usb_test_mux03_projection)

add_test(
    NAME mux03_architecture
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mux03_architecture.py
            ${PROJECT_SOURCE_DIR}
)

add_executable(blu2usb_test_mux04_storage test_mux04_storage.c)
target_link_libraries(blu2usb_test_mux04_storage PRIVATE
    blu2usb_storage
    blu2usb_device_registry
    blu2usb_profiles
)
add_test(NAME mux04_durable_product_storage COMMAND blu2usb_test_mux04_storage)

add_test(
    NAME mux04_architecture
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_mux04_architecture.py
            ${PROJECT_SOURCE_DIR}
)

add_executable(blu2usb_test_mux05_session_roles test_mux05_session_roles.c)
target_link_libraries(blu2usb_test_mux05_session_roles PRIVATE
    blu2usb_ble_hogp
)
add_test(NAME mux05_session_roles COMMAND blu2usb_test_mux05_session_roles)
