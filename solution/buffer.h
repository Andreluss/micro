#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define BUFF_SIZE 10
typedef struct Buffer {
    char buff[BUFF_SIZE];
    int count;
    int first;
} Buffer;

void Buffer_init(Buffer* b);
void Buffer_clear(Buffer* b);
bool Buffer_empty(Buffer* b);
void Buffer_push(Buffer* b, char c);
int Buffer_push_bytes(Buffer* b, const uint8_t* bytes, int length);
int Buffer_push_string(Buffer* b, const char* c);
void Buffer_get_segment(Buffer* b, char** buff_p, int* count_p);
void Buffer_pop(Buffer* b, int count);
