 

add_executable(blink_led_serial
    blink_led_serial.c
)

target_link_libraries(blink_led_serial pico_stdlib)

pico_enable_stdio_usb(blink_led_serial 0)
pico_enable_stdio_uart(blink_led_serial 1)

pico_add_extra_outputs(blink_led_serial)
