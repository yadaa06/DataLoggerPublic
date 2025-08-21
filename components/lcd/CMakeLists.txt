idf_component_register(SRCS "lcd_i2c.c" "lcd_task.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver esp_timer dht11 webserver esp_http_server)