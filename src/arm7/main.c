#include <nds.h>
#include <dswifi7.h>

#include "as_lib7.h"
#include "../common/fifo.h"


u8 isOldDS = 0;
//u8 backlight = 1;

void vCountHandler() {
    u16 pressed = REG_KEYXY;

    inputGetAndSend();

    //Sleep mode
    if (pressed & BIT(7)) {
        systemSleep();
    }
}

void vBlankHandler() {
    AS_SoundVBL();
    Wifi_Update();
}

/*void FIFOBacklight(u32 value, void* data)
{
    int backlight = value;
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
    writePowerManagement(4, value+1);
    //fifoSendValue32(FIFO_BACKLIGHT, MSG_TOGGLE_BACKLIGHT);
    fifoSendValue32(FIFO_BACKLIGHT, backlight);
}*/

void FIFOAudio(u32 value, void* data)
{
    AS_Init((IPC_SoundSystem*)value); 
}



int main(int argc, char** argv) {
    irqInit();
    fifoInit();
    
    //fifoSetValue32Handler(FIFO_BACKLIGHT, &FIFOBacklight, NULL);
    fifoSetValue32Handler(FIFO_AUDIO, &FIFOAudio, NULL);
    
    readUserSettings();
    initClockIRQ();
    SetYtrigger(80);

    //Setup FIFO
    installWifiFIFO();
    installSystemFIFO();

    //installSoundFIFO();
    //mmInstall(FIFO_MAXMOD);

    //Setup IRQ
    irqSet(IRQ_VCOUNT, vCountHandler);
    irqSet(IRQ_VBLANK, vBlankHandler);
    irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

    //Send backlight state
    /*isOldDS = !(readPowerManagement(4) & BIT(6));
    int backlight = isOldDS ? 3 : readPowerManagement(4);
    fifoSendValue32(FIFO_BACKLIGHT, MSG_TOGGLE_BACKLIGHT);
    fifoSendValue32(FIFO_BACKLIGHT, backlight);*/
    

    //Enable sound
    REG_SOUNDCNT = SOUND_ENABLE | SOUND_VOL(0x7F);

    do {
        swiWaitForVBlank();
        AS_MP3Engine();
    } while(1);
}
