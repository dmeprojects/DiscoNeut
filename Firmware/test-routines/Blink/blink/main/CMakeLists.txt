#set(EXTRA_COMPONENT_DIRS "./inc" "./src")
idf_component_register(SRCS "blink_example_main.c" "led_patterns.c"
                       INCLUDE_DIRS ".")

