#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"

#include "prawn_do.pio.h"
#include "serial.h"

#define LED_PIN 25

#define MAX_DO_CMDS 32768
uint32_t do_cmds[MAX_DO_CMDS];
uint32_t do_cmd_count;

#define SERIAL_BUFFER_SIZE 256
char serial_buf[SERIAL_BUFFER_SIZE];

#define STATUS_OFF 0
#define STATUS_STARTING 1
#define STATUS_RUNNING 2
#define STATUS_ABORTING 3
int status;

void start_sm(PIO pio, uint sm, uint dma_chan, uint offset){
	pio_sm_restart(pio, sm);
	pio_sm_exec(pio, sm, pio_encode_jmp(offset));

	dma_channel_config dma_config = dma_channel_get_default_config(dma_chan);
	channel_config_set_read_increment(&dma_config, true);
	channel_config_set_write_increment(&dma_config, false);
	channel_config_set_dreq(&dma_config, pio_get_dreq(pio, sm, true));
	dma_channel_configure(dma_chan, &dma_config, &pio->txf[sm], do_cmds, do_cmd_count, true);

	pio_sm_set_enabled(pio, sm, true);	
}

void stop_sm(PIO pio, uint sm, uint dma_chan){
	dma_channel_abort(dma_chan);
	pio_sm_set_enabled(pio, sm, false);
	pio_sm_clear_fifos(pio, sm);
}

int main(){
	stdio_init_all();
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	gpio_put(LED_PIN, 1);
	sleep_ms(2500);
	printf("Prawn Digital Output online\n");
	gpio_put(LED_PIN, 0);

	status = STATUS_OFF;

	PIO pio = pio0;
	uint sm = pio_claim_unused_sm(pio, true);
	uint dma_chan = dma_claim_unused_channel(true);
	uint offset = pio_add_program(pio, &prawn_do_program);
	pio_sm_config pio_config = prawn_do_program_init(pio, sm, 10.f, offset);

	while(1){
		printf("> ");
		gpio_put(LED_PIN, 1);
		unsigned int buf_len = readline(serial_buf, SERIAL_BUFFER_SIZE);
		gpio_put(LED_PIN, 0);
		if(buf_len < 3){
			printf("Invalid command: %s\n", serial_buf);
			continue;
		}
		if(strncmp(serial_buf, "abt", 3) == 0){
			// Abort
			if(status != STATUS_OFF){
				stop_sm(pio, sm, dma_chan);
				status = STATUS_OFF;
			}
		}
		else if(strncmp(serial_buf, "cls", 3) == 0){
			// Clear
			if(status == STATUS_OFF){
				do_cmd_count = 0;
			}
			else{
				printf("Unable to clear while running, please abort (abt) first\n");
			}
		}
		else if(strncmp(serial_buf, "run", 3) == 0){
			// Run
			if(status == STATUS_OFF){
				start_sm(pio, sm, dma_chan, offset);
				status = STATUS_RUNNING;
			}
			else{
				printf("Unable to (restart) run while running, please abort (abt) first\n");
			}
		}
		else if(strncmp(serial_buf, "add", 3) == 0){
			// Add (DO command)
			if(status == STATUS_OFF){
				while(do_cmd_count < MAX_DO_CMDS-1){
					buf_len = readline(serial_buf, SERIAL_BUFFER_SIZE);

					if(buf_len >= 3){
						if(strncmp(serial_buf, "end", 3) == 0){
							break;
						}
					}

					do_cmds[do_cmd_count] = strtoul(serial_buf, NULL, 0);
					do_cmd_count++;
				}
				if(do_cmd_count == MAX_DO_CMDS-1){
					printf("Too many DO commands (%d). Please use resources more efficiently or increase MAX_DO_CMDS and recompile.\n", MAX_DO_CMDS);
				}
			}
			else{
				printf("Unable to add while running, please abort (abt) first\n");
			}
		}
		else if(strncmp(serial_buf, "dmp", 3) == 0){
			// Dump
			for(int i = 0; i < do_cmd_count; i++){
				printf("do_cmd #%d\n", i);
				printf("\t0x%08x\n", do_cmds[i]);
			}
		}
		else{
			printf("Invalid command: %s\n", serial_buf);
		}
	}
}
