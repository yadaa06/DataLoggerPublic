idf_component_register(SRCS "webserver.cpp"
                       INCLUDE_DIRS "." 
                       PRIV_REQUIRES "esp_https_server" "dht11" "speaker" "lcd" "driver"
                       EMBED_FILES "index.html" "style.css" "script.js")