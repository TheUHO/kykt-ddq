#include "processor_dispatch.h"
int get_dsp(struct processor_dispatch_share *ps){
    int i;
    for(i=0; (i<MAX_DSP) && (ps->dsp_status[i] == dsp_status_busy); i++);
    if(i == MAX_DSP) return -1;
   
    return i;
}