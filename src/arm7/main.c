#include <nds.h>
#include <dswifi7.h>

#include "as_lib7.h"
#include "../common/fifo.h"

u8 isOldDS = 0;
u8 backlight = 1;

void vCountHandler() {
	u16 pressed = REG_KEYXY;

	inputGetAndSend();

    //Sleep mode
    if (pressed & BIT(7)) {
    	systemSleep();
    }
}

void vBlankHandler() {
   	AS_SoundVBL(); // Update AS_Lib
    Wifi_Update();  //Update WIFI
}

int main(int argc, char** argv) {
	irqInit();
	fifoInit();

	readUserSettings();
	initClockIRQ();
	SetYtrigger(80);

	//Setup FIFO
	installSystemFIFO();
	installWifiFIFO();
	//installSoundFIFO();
	//mmInstall(FIFO_MAXMOD);

    //Setup IRQ
	irqSet(IRQ_VBLANK, vBlankHandler);
	irqSet(IRQ_VCOUNT, vCountHandler);

	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

	//Send backlight state
	isOldDS = !(readPowerManagement(4) & BIT(6));
	backlight = isOldDS ? 3 : readPowerManagement(4);
	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, backlight);

	//Enable sound
	REG_SOUNDCNT = SOUND_ENABLE | SOUND_VOL(0x7F);

	do {
        if (fifoCheckValue32(TCOMMON_FIFO_CHANNEL_ARM7)) {
        	u32 value = fifoGetValue32(TCOMMON_FIFO_CHANNEL_ARM7);

        	switch (value) {
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
            	fifoSendValue32(TCOMMON_FIFO_CHANNEL_ARM9, backlight);
            	break;
        	case MSG_INIT_SOUND_ARM7:
        	    ipcSound->chan[0].cmd = SNDCMD_ARM7READY;

        	    while (!fifoCheckValue32(TCOMMON_FIFO_CHANNEL_ARM7));
        	   	AS_Init((IPC_SoundSystem*)fifoGetValue32(TCOMMON_FIFO_CHANNEL_ARM7));
        	   	break;
        	}
        }

		AS_MP3Engine();
		swiWaitForVBlank();
    } while(1);
}
