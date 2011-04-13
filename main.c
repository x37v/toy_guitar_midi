#include "midi_usb.h"
#include "avr/io.h"

//this example simply echos midi back, for now

void fallthrough_callback(MidiDevice * device, uint16_t cnt, uint8_t byte0, uint8_t byte1, uint8_t byte2){
   midi_send_data(device, cnt, byte0, byte1, byte2);
}

#define HIST_LEN 4
#define BNT_ROWS 3

int main(void) {
   MidiDevice usb_midi;
	bool trig_hist[HIST_LEN];
	bool trig_last = false;
	uint8_t thist_index = 0;
	uint8_t bhist_index = 0;
	uint8_t btn_row = 0;
	uint8_t i;
	uint8_t btn_hist[BNT_ROWS][HIST_LEN];
	uint8_t btn_last[BNT_ROWS];

	for(i = 0; i < HIST_LEN; i++) {
		trig_hist[i] = 0;
		for(btn_row = 0; btn_row < BNT_ROWS; btn_row++)
			btn_hist[btn_row][i] = 0;
		btn_last[i] = 0;
	}

	//e0 as an input with pullup
	DDRE &= ~(_BV(PE0));
	PORTE |= _BV(PE0);

	//porta is all inputs with pullups
	DDRA = 0;
	PORTA = 0xFF;

	//c7 is output pulled to ground
	DDRC = _BV(PC7);
	PORTC = 0;

   midi_usb_init(&usb_midi);
   //midi_register_fallthrough_callback(&usb_midi, fallthrough_callback);

	thist_index = 0;
	bhist_index = 0;
	btn_row = 0;
   while(1){
		DDRC = _BV(PC7) | 1 << (btn_row);
		PORTC = 0;

		trig_hist[thist_index] = (bool)((~PINE) & _BV(PE0));
		bool trig = trig_hist[0];
		bool consistent = true;
		for(i = 1; i < HIST_LEN; i++) {
			if (trig != trig_hist[i]) {
				consistent = false;
				break;
			}
		}

		if (consistent && trig_last != trig) {
			if (trig)
				midi_send_noteon(&usb_midi,0,64,127);
			else
				midi_send_noteoff(&usb_midi,0,64,127);
			trig_last = trig;
		}

      midi_device_process(&usb_midi);

		uint8_t col;
		btn_hist[btn_row][bhist_index] = ~PINA;

		for(col = 0; col < 8; col++) {
			consistent = true;
			uint8_t mask = (1 << col);
			bool down = (bool)(btn_hist[btn_row][0] & mask);
			for(i = 1; i < HIST_LEN; i++) {
				if(down != (bool)(btn_hist[btn_row][i] & mask)) {
					consistent = false;
					break;
				}
			}
			if (consistent && ((bool)(btn_last[btn_row] & mask)) != down) {
				midi_send_cc(&usb_midi, 0, btn_row * 8 + col, (down ? 1 : 0));
				if (down)
					btn_last[btn_row] |= mask;
				else
					btn_last[btn_row] &= ~mask;
			}
		}

		thist_index += 1;
		if (thist_index >= HIST_LEN)
			thist_index = 0;

		btn_row += 1;
		if (btn_row >= BNT_ROWS) {
			btn_row = 0;
			bhist_index += 1;
			if (bhist_index >= HIST_LEN)
				bhist_index = 0;
		}
   }

   return 0;
}
