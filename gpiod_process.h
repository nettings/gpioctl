#ifndef GPIOD_PROCESS_H
#define GPIOD_PROCESS_H

void setup_gpiod_rotary(int clk, int dt, void (*user_callback)); 
void setup_gpiod_switch(int sw, void (*callback));
void setup_gpiod_handler();
void shutdown_gpiod();

#endif
