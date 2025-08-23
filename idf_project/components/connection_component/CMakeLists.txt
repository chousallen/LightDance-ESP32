idf_component_register(
    SRCS "connection_component.c"
    INCLUDE_DIRS "include"
    # Public deps: other components this one calls into (headers & linking)
    REQUIRES parsing_component esp_wifi esp_event esp_netif lwip nvs_flash
    # Private deps: used only inside this .c (headers/macros not exposed by our public headers)
    PRIV_REQUIRES log esp_timer
)