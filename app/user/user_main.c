#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "at.h"
#include "os_type.h"
#include "gpio.h"
#include "user_config.h"
#include "user_interface.h"

extern uint8_t at_wifiMode;
unsigned char relais_state;
unsigned char led_state;
volatile unsigned int counter_host;

extern unsigned long timer_val;

static volatile os_timer_t my_timer;

volatile unsigned char mytimer_seconds = 0;
volatile unsigned char mytimer_minutes = 0;
volatile unsigned char mytimer_hours = 0;
volatile unsigned int  mytimer_days = 0;

unsigned char check_ip(void);
void my_timerfunc(void *arg);
//unsigned char buff[64];


/*******************************************************************************     
* Timer 1 Sekunde allgemein
* 
* 
******************************************************************************** */ 
void my_timerfunc(void *arg)
{        
    mytimer_seconds++;
    if (mytimer_seconds > 59) 
    {
        mytimer_seconds = 0;
        mytimer_minutes++;
        if (mytimer_minutes > 59) 
        {
            mytimer_minutes = 0;
            mytimer_hours++;
            if (mytimer_hours > 23)
            {
                mytimer_hours = 0;
                mytimer_days++;
            }
        }
    }
        
    // Check ob IP erhalten aller 10 Sekunden
    if (mytimer_seconds % 10 == 0)  
    {
        if (check_ip()) toggleLED();            // im StationAP Mode
        else            setLED(relais_state);   // LED dem Relais folgen lassen              
    }
    
    // SetTimer herunterzaehlen   
    if (timer_val > 0) 
    {
        timer_val--;
        if (timer_val == 1)
        {
            setRelais(off);        
            setLED(off);          
        }
    }
    
    // alive ?
    if (--counter_host == 0) system_restart();
}

/*******************************************************************************     
* Check auf Station Mode und gueltige IP Adresse, 
* fallback zu StationAP Mode fuer Eingabe WLAN Parameter
* 
* return 0|1  (Station = 0, StationAP = 1)
******************************************************************************** */ 
unsigned char check_ip(void)
{
    static unsigned char status;
    
    status = wifi_station_get_connect_status();
    at_wifiMode = wifi_get_opmode();

    switch(at_wifiMode)
    {
        case STATION_MODE:   if (status != STATION_GOT_IP) wifi_set_opmode(STATIONAP_MODE); break;
        case STATIONAP_MODE: if (status == STATION_GOT_IP) wifi_set_opmode(STATION_MODE);   break;
    }
    
    at_wifiMode = wifi_get_opmode(); // nicht rauskuerzen!
       
    if (at_wifiMode == STATIONAP_MODE) return 1;
    else                               return 0;
}


/*******************************************************************************     
* user init
* 
* 
******************************************************************************** */ 
void user_init(void)
{    
    //Set GPIO 13 to output mode (Relais))
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
    setRelais(off);

    //Set GPIO 14 to output mode (LED))
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
    setLED(on);
    
    counter_host = 60 * 60; // alive zaehler
    mytimer_seconds = 0;
    mytimer_minutes = 0;
    mytimer_hours   = 0;
    mytimer_days    = 0;
    timer_val = 0;

    // fuer Debugzwecke
    uart_init(BIT_RATE_9600, BIT_RATE_9600);

    wifi_set_opmode( STATION_MODE );
    at_wifiMode = wifi_get_opmode();

    at_init();

    at_setupCmdE(15,"0");                 // Echo aus
    at_setupCmdCipmux(15,"=1");           // mehrfache Verbindungen zulassen
    at_setupCmdCipserver(15,"=1,80");     // Servermode, Port 80
    at_setupCmdCipsto(15,"=15");          // Timeout 15 Sekunden

    // allgemeiner Sekunden-Timer
    // https://github.com/esp8266/source-code-examples/blob/master/blinky/user/user_main.c
    
    os_timer_disarm(&my_timer);
    os_timer_setfn(&my_timer, (os_timer_func_t *)my_timerfunc, NULL);
    os_timer_arm(&my_timer, 1000, 1);


    
    // letzten Zustand wieder herstellen, LED wird dann im Timer gesetzt     
    //spi_flash_read(0x3C000, (uint32 *)buff, 64);        
    
    //if (buff[0] == 0) setRelais(on);
    //if (buff[0] == 1) setRelais(off);
    //if (buff[0] == 2) setRelais(buff[3]);
    
}

