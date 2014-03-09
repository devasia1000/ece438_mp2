#ifndef CUSTOM_STRUCTS
#define CUSTOM_STRUCTS

typedef struct data{
    int sender_id;
    int ttl;
    int top[MAX_NODE_COUNT][MAX_NODE_COUNT];
} DATA;
typedef struct update{
    char neighbours[MAX_NODE_COUNT][20];
    int top[MAX_NODE_COUNT][MAX_NODE_COUNT];
} UPDATE;

#endif

