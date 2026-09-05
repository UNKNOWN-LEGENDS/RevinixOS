#ifndef PMM_H
#define PMM_H

#include<stdint.h>

#define FRAME_SIZE 4096

void pmm_init(uint64_t mb2_info);
uint64_t pmm_alloc_frame(void);         //returns physical addr, or 0 if OOM
void pmm_free_frame(uint64_t addr);
uint64_t pmm_total_frames(void);
uint64_t pmm_used_frames(void);

#endif