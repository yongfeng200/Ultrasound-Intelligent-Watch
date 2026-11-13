#ifndef _AUTOFHL_H_
#define _AUTOFHL_H_

int fhl_init(uint8_t vref, uint8_t dac_range, uint8_t debug);
int fhl_check(int debug);
int fhl_init_lite(uint8_t vref_sel, uint8_t dac_range, uint8_t debug);
int fhl_check_lite(int debug);
int get_wavep_lite(uint8_t dir, uint8_t vref, uint8_t debug, int *order, int *peak);

#endif