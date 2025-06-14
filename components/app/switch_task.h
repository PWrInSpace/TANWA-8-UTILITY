#ifndef SWITCH_TASK_H
#define SWITCH_TASK_H

// Initialize switch interrupts for all 8 switches
esp_err_t switch_interrupts_init(void);

// Command functions for switch press (clicked)
void cmd1(void);
void cmd2(void);
void cmd3(void);
void cmd4(void);
void cmd5(void);
void cmd6(void);
void cmd7(void);
void cmd8(void);

// Command functions for switch release (unclicked)
void cmd1_off(void);
void cmd2_off(void);
void cmd3_off(void);
void cmd4_off(void);
void cmd5_off(void);
void cmd6_off(void);
void cmd7_off(void);
void cmd8_off(void);

#endif // SW task