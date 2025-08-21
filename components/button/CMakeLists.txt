idf_component_register(SRCS "button_task.cpp" "button.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES dht11 driver lcd)