
#ifndef _VDP_H_
#define _VDP_H_

/* Global variables */
extern uint8 sat[0x400];
extern uint8 vram[0x10000];
extern uint8 cram[0x80];               
extern uint8 vsram[0x80];              
extern uint8 reg[0x20];
extern uint16 addr;
extern uint16 addr_latch;
extern uint8 code;
extern uint8 pending;
extern uint16 status;
extern uint16 ntab;                    
extern uint16 ntbb;                    
extern uint16 ntwb;                    
extern uint16 satb;                    
extern uint16 hscb;                    
extern uint16 sat_base_mask;
extern uint16 sat_addr_mask;
extern uint8 border;                   
extern uint8 bg_name_dirty[0x800];     
extern uint16 bg_name_list[0x800];     
extern uint16 bg_list_index;           
extern uint8 bg_pattern_cache[0x80000];
extern uint8 playfield_shift;          
extern uint8 playfield_col_mask;       
extern uint16 playfield_row_mask;      
extern uint32 y_mask;                  
extern uint8 hint_pending;
extern uint8 vint_pending;
extern uint8 vint_triggered;
extern int8 hvint_updated;
extern int16 h_counter;
extern int16 hc_latch;
extern uint16 v_counter;
extern uint8 im2_flag;
extern uint8 dmafill;
extern uint32 dma_endCycles;
extern uint8 dma_type;
extern uint32 dma_length;
extern int32 fifo_write_cnt;
extern uint32 fifo_lastwrite;
extern uint8 fifo_latency;

extern void (*color_update)(int index, uint16 data);

extern uint8 dmatiming;
extern uint8 vdptiming;
extern uint8 vdp_pal;

/* Function prototypes */
void vdp_init(void);
void vdp_reset(void);
void vdp_shutdown(void);
void vdp_ctrl_w(uint16 data);
uint16 vdp_ctrl_r(void);
void vdp_data_w(uint16 data);
uint16 vdp_data_r(void);
void vdp_reg_w(uint8 r, uint8 d);
uint16 vdp_hvc_r(void);
void dma_update();
void fifo_update();
void vdp_test_w(uint16 value);

#endif /* _VDP_H_ */
