// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include	"basic_types.h"

//MT3000 GSM
#ifdef enable_processor_dsp
#include	"hthread_device.h"
extern __gsm__ int_t signals_gsm[24];
#endif

#endif // GLOBALS_H