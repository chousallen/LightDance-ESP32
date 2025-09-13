idf_component_register(SRCS 
                            "src/player_content.c"
                            "src/pattern_table.c"
                            "src/sdcard.c"
                            "src/player.c"
                            "src/testing.c"
                    INCLUDE_DIRS "inc"
                    REQUIRES fatfs driver esp_timer LedDriver)
