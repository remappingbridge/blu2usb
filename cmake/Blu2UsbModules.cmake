# Canonical module graph for BLU2USB-G01.
# These are architecture contracts only. Later gates replace INTERFACE scaffolds
# with real libraries without changing ownership casually.

set(BLU2USB_MODULES
    domain
    hid_aggregator
    profiles
    remap
    device_registry
    connection_coordinator
    bt_runtime
    ble_hogp
    classic_hid
    keyboard_transport
    logitech_hidpp
    usb_hid
    storage
    interaction
    ux_model
    renderer
    hat
    app
)

set(BLU2USB_DEPS_domain "")
set(BLU2USB_DEPS_hid_aggregator "domain")
set(BLU2USB_DEPS_profiles "domain")
set(BLU2USB_DEPS_remap "domain;profiles")
set(BLU2USB_DEPS_device_registry "domain")
set(BLU2USB_DEPS_connection_coordinator "domain;device_registry")
set(BLU2USB_DEPS_bt_runtime "domain")
set(BLU2USB_DEPS_ble_hogp "domain;bt_runtime")
set(BLU2USB_DEPS_classic_hid "domain;bt_runtime")
set(BLU2USB_DEPS_keyboard_transport "domain;ble_hogp;classic_hid")
set(BLU2USB_DEPS_logitech_hidpp "domain;bt_runtime;ble_hogp")
set(BLU2USB_DEPS_usb_hid "domain;hid_aggregator")
set(BLU2USB_DEPS_storage "domain")
set(BLU2USB_DEPS_interaction "domain")
set(BLU2USB_DEPS_ux_model "domain;interaction;device_registry")
set(BLU2USB_DEPS_renderer "ux_model")
set(BLU2USB_DEPS_hat "domain")
set(BLU2USB_DEPS_app "domain;hid_aggregator;profiles;remap;device_registry;connection_coordinator;bt_runtime;ble_hogp;classic_hid;keyboard_transport;logitech_hidpp;usb_hid;storage;interaction;ux_model;renderer;hat")

function(blu2usb_declare_contract_modules)
    foreach(module IN LISTS BLU2USB_MODULES)
        add_library(blu2usb_module_${module} INTERFACE)
    endforeach()

    foreach(module IN LISTS BLU2USB_MODULES)
        foreach(dep IN LISTS BLU2USB_DEPS_${module})
            if(NOT dep STREQUAL "")
                target_link_libraries(blu2usb_module_${module} INTERFACE blu2usb_module_${dep})
            endif()
        endforeach()
    endforeach()
endfunction()
