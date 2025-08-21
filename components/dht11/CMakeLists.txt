idf_component_register(SRCS "dht11_task.cpp" "dht11.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver esp_timer speaker lcd)