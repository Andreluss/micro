#include "buffer.h"
#include "led.h"

static int min(int a, int b) {
    return a < b ? a : b;
}

void Buffer_init(Buffer* b) {
    b->count = 0;
    b->first = 0;
}
void Buffer_clear(Buffer* b) {
    Buffer_init(b);
}
bool Buffer_empty(Buffer* b) {
    return b->count == 0;
}
void Buffer_push(Buffer* b, char c) {
    int pos = (int)(((long long)b->first + b->count) % BUFF_SIZE);
    b->buff[pos] = c;
    b->count++;
}

int Buffer_push_bytes(Buffer* b, const uint8_t* bytes, int length) {
    int LIMIT = BUFF_SIZE - b->count; 
    int i = 0;
    while ((LIMIT--) && i < length) {
        Buffer_push(b, bytes[i]);
        i++;
    }
    if (i != length) {
        led_green_off();
    }
    return i != length; // 1 if not all bytes were pushed
}

int Buffer_push_string(Buffer* b, const char* c) {
    int len = strlen(c);
    return Buffer_push_bytes(b, (const uint8_t*)c, len);
}
void Buffer_get_segment(Buffer* b, char** buff_p, int* count_p) {
    *buff_p = b->buff + b->first;
    *count_p = min(b->count, BUFF_SIZE - b->first);
}
void Buffer_pop(Buffer* b, int count) {
    if (count <= 0) return;
    count = min(count, b->count);
    b->first = (b->first + count) % BUFF_SIZE;
    b->count -= count;
}