#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include "error.h"

void* new_tianhe_thread(){
    return malloc(sizeof(tianhe_thread_t));
}
void del_tianhe_thread(void *p){
    free(p);
}