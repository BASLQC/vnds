#include <nds.h>
#include "as_lib7.h"
#include "../common/fifo.h"

#ifdef SUPPORT_WIFI
#include <dswifi7.h>
#endif

u8 isOldDS = 0;
u8 backlight = 1;

void vCountHandler() {
	inputGetAndSend();
}

void vBlankHandler() {
   	AS_SoundVBL(); // Update AS_Lib

#ifdef SUPPORT_WIFI
   	Wifi_Update(); //Update WIFI
#endif
}

int main(int argc, char** argv) {
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();
	SetYtrigger(80);

#ifdef SUPPORT_WIFI
	installWifiFIFO();
#endif

	installSoundFIFO();
	//mmInstall(FIFO_MAXMOD);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, vCountHandler);
	irqSet(IRQ_VBLANK, vBlankHandler);

    //Setup IRQ
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

	//Send backlight state
	isOldDS = !(readPowerManagement(4) & BIT(6));
	backlight = isOldDS ? 3 : readPowerManagement(4);
	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, MSG_TOGGLE_BACKLIGHT);
	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, backlight);

	//Enable sound
	REG_SOUNDCNT = SOUND_ENABLE | SOUND_VOL(0x7F);
	swiWaitForVBlank();

	do {
    	if (fifoCheckValue32(TCOMMON_FIFO_CHANNEL_ARM7)) {
        	u32 value = fifoGetValue32(TCOMMON_FIFO_CHANNEL_ARM7);

        	switch (value) {
#ifdef SUPPORT_BACKLIGHT
        	case MSG_TOGGLE_BACKLIGHT:
            	//Toggle backlight
            	if (isOldDS) {
            		u32 power = readPowerManagement(PM_CONTROL_REG) & ~(PM_BACKLIGHT_TOP|PM_BACKLIGHT_BOTTOM);
            		if (backlight) {
            			backlight = 0;
            		} else {
            			backlight = 3;
            			power = power | PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM;
            		}
            		writePowerManagement(PM_CONTROL_REG, power);
            	} else {
            		backlight = ((backlight+1) & 0x3); //b should be in range 0..3
            		writePowerManagement(4, backlight);
            	}
            	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, MSG_TOGGLE_BACKLIGHT);
            	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, backlight);
            	break;
#endif
        	case MSG_INIT_SOUND_ARM7:
        	    while (!fifoCheckValue32(TCOMMON_FIFO_CHANNEL_ARM7));
        	    int v = fifoGetValue32(TCOMMON_FIFO_CHANNEL_ARM7);
        	   	AS_Init((IPC_SoundSystem*)v);
        	   	break;
        	}
        }

		swiWaitForVBlank();
        AS_MP3Engine();
    } while(1);
}
