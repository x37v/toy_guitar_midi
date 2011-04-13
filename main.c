#include "midi_usb.h"
#include "avr/io.h"

//this example simply echos midi back, for now

void fallthrough_callback(MidiDevice * device, uint16_t cnt, uint8_t byte0, uint8_t byte1, uint8_t byte2){
   midi_send_data(device, cnt, byte0, byte1, byte2);
}

#define HIST_LEN 4

int main(void) {
   MidiDevice usb_midi;
	bool trig_hist[HIST_LEN];
	bool trig_last = false;
	uint8_t hist = 0;
	uint8_t i;
	for(i = 0; i < HIST_LEN; i++) {
		trig_hist[i] = 0;
	}

	//e0 as an output and ground
	DDRE |= _BV(PE0);
	PORTE &= ~(_BV(PE0));

	//c7 is input with pullup
	DDRC &= ~(_BV(PC7));
	PORTC |= _BV(PC7);

   midi_usb_init(&usb_midi);
   //midi_register_fallthrough_callback(&usb_midi, fallthrough_callback);

	hist = 0;
   while(1){
		trig_hist[hist] = (bool)((~PINC) & _BV(PC7));
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

		hist += 1;
		if (hist >= HIST_LEN)
			hist = 0;
      midi_device_process(&usb_midi);
   }

   return 0;
}
