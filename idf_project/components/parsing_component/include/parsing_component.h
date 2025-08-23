#ifndef PARSING_COMPONENT_H
#define PARSING_COMPONENT_H

#ifdef __cplusplus
extern "C"
{
#endif

    void process_line(int sock, char *line);
    void send_sync_with_t1(int sock);

#ifdef __cplusplus
}
#endif

#endif // PARSING_COMPONENT_H