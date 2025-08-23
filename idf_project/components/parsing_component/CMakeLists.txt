idf_component_register(
    SRCS "parsing_component.c"
    INCLUDE_DIRS "include"
    # Uses esp_timer_get_time() and ESP_LOG* only inside .c:
    PRIV_REQUIRES esp_timer log
    # If process_line (or other functions here) call into another custom component,
    # list it in REQUIRES so linking works. For example:
    # REQUIRES sync_component
)