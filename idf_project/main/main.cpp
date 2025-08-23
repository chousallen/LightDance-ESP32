#include <stdio.h>
#include "connection_component.h"
extern "C" void app_main();

void app_main(void)
{
    // start wifi connection, open socket, listen to socket and parse command
    connection_start();
}
