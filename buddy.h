#ifndef BUDDY_H
#define BUDDY_H

void buddy_init();
void *buddy_alloc(int size);
void buddy_free(void *addr);
void split(int order);
void merge(void* addr);
void buddy_dump();

#endif // BUDDY_H
