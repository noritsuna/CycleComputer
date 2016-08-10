/*
   Cycle Computer for mbed
   Copyright (c) 2006-2016 SIProp Project http://www.siprop.org/

   This software is provided 'as-is', without any express or implied warranty.
   In no event will the authors be held liable for any damages arising from the use of this software.
   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it freely,
   subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.
*/

#include "mbed.h"
#include "TextLCD_20X4.h"

static int max_lap = 15;
static int tire_inchi = 26;
static float tire_flange = 18.0;
static float gear_ratio = 4.3;

AnalogIn voltmeter(PA_7);
AnalogIn ammeter_total(PA_6);
AnalogIn ammeter_cell(PA_5);
InterruptIn lap_sw(D8);
//Serial serialCtrl(USBTX, USBTX);
Serial serialCtrl(PC_10, PC_11);
TextLCD_20X4 lcd(D7, D6, D5, D4, D3, D2); // rs, e, d0, d1, d2, d3

//Serial pc(USBTX, USBRX);
static time_t start_time;
static uint8_t lap_cont;
static uint32_t lap_1_min;
static uint32_t lap_1_sec;
static uint32_t lap_2_min;
static uint32_t lap_2_sec;
static time_t lap_progress_start_time;

static time_t lap_last_irq_time;

static uint32_t rpm;
static float circumference;

void display_total_time(uint32_t min, uint32_t sec) {
    char buf[10];
    if(min >= 100) {
        min = 99;
    }
    sprintf(buf, "TT:%2d.%2d,", min, sec);
    lcd.locate(0,0);
    lcd.printf(buf);
}

void display_lap_progress(uint32_t min, uint32_t sec) {
    char buf[10];
    if(min >= 100) {
        min = 99;
    }
    sprintf(buf, "LT:%2d.%2d", min, sec);
    lcd.locate(10,0);
    lcd.printf(buf);
}

void display_lap_1(uint32_t min, uint32_t sec) {
    char buf[10];
    if(min >= 100) {
        min = 99;
    }
    sprintf(buf, "L1:%2d.%2d,", min, sec);
    lcd.locate(0,1);
    lcd.printf(buf);
}

void display_lap_2(uint32_t min, uint32_t sec) {
    char buf[10];
    if(min >= 100) {
        min = 99;
    }
    sprintf(buf, "L2:%2d.%2d", min, sec);
    lcd.locate(10,1);
    lcd.printf(buf);
}

void display_lap_cont(uint32_t cont) {
    char buf[2];
    int carry = 0;
    if(cont >= 20) {
        cont = 9;
    } else if(cont >= 10) {
        cont = cont - 10;
        carry = 1;
    }
    sprintf(buf, "%1d", cont);
    lcd.locate(19,0);
    lcd.printf(buf);
    
    if(carry == 0) {    
        lcd.locate(19,1);
        lcd.printf(" ");
    } else {
        lcd.locate(19,1);
        lcd.printf("+");
    }
        
}

void display_voltmeter(float voltage) {
    char buf[10];
    if(voltage > 99.9f) {
        voltage = 99.9;
    }
    sprintf(buf, "%4.1fV,", voltage);
    lcd.locate(0,2);
    lcd.printf(buf);
}

void display_ammeter_total(float current) {
    char buf[10];
    if(current > 99.9f) {
        current = 99.9;
    }
    sprintf(buf, "T:%4.1f,", current);
    lcd.locate(6,2);
    lcd.printf(buf);
}

void display_ammeter_cell(float current) {
    char buf[10];
    if(current > 99.9f) {
        current = 99.9;
    }
    sprintf(buf, "C:%4.1fA", current);
    lcd.locate(13,2);
    lcd.printf(buf);
}

void display_rpm(uint32_t rpm) {
    char buf[10];
    if(rpm >= 10000) {
        rpm = 9999;
    }
    sprintf(buf, "%4dRPM,", rpm);
    lcd.locate(0,3);
    lcd.printf(buf);
}

void display_speed(float speed) {
    char buf[12];
    if(speed > 99.99f) {
        speed = 99.99;
    }
    sprintf(buf, "S:%5.2fkm/h", speed);
    lcd.locate(9,3);
    lcd.printf(buf);
}

void display_init() {    
    display_total_time(0,0);
    display_lap_progress(0,0);
    display_lap_1(0,0);
    display_lap_2(0,0);
    display_lap_cont(0);
    display_voltmeter(0.0f);
    display_ammeter_total(0.0f);
    display_ammeter_cell(0.0f);
    display_rpm(0);
    display_speed(0.0f);
}

void ctrl_rx_interrupt() {
    char rx_data;
    char t_data[20];
    int cnt = 0;
    int ret = 0;

    do {
        rx_data = serialCtrl.getc();
        if(rx_data != '\r'){
            t_data[cnt] = rx_data;
        } else {
            // CRLF ("\r\n")
            rx_data = serialCtrl.getc();
            if(rx_data == '\n') {
                break;
            }
        }
        cnt++;
    } while(cnt < 10);
    t_data[cnt] = '\0';

    // str to int    
    ret = atoi(t_data);
    if(ret > 0 && ret < 9999) {
        rpm = ret;
    }
    
    // init
    for(int i=0; i<20; i++) {
        t_data[i] = '\0';
    }
    cnt = 0;
}


void lap_interrupt() {
    // Chattering prevention code
    time_t current_time;
    current_time = time(NULL);
    if(current_time - lap_last_irq_time > 1) {
        lap_last_irq_time = current_time;
    } else {
        return;
    }
    
    if(lap_cont > max_lap) {
        start_time = time(NULL);
        lap_1_min = 0;
        lap_1_sec = 0;
        lap_2_min = 0;
        lap_2_sec = 0;
        lap_cont = 0;
    } else if(lap_cont == 0) {
        start_time = time(NULL);
        lap_1_min = 0;
        lap_1_sec = 0;
        lap_2_min = 0;
        lap_2_sec = 0;
        lap_cont = 0;
        lap_cont++;
    } else {
        lap_2_min = lap_1_min;
        lap_2_sec = lap_1_sec;
        
        time_t progress_time;
        progress_time = current_time - lap_progress_start_time;
        uint32_t progress_min;
        progress_min = progress_time / 60;
        uint32_t progress_sec;
        progress_sec = progress_time % 60;
        lap_1_min = progress_min;
        lap_1_sec = progress_sec;
        lap_cont++;
    }
    lap_progress_start_time = time(NULL);
}


void show_lap() {
    if(lap_cont == 0) {
        display_lap_cont(lap_cont);
        return;
    }
    
    time_t current_time;
    current_time = time(NULL);
    
    time_t total_time;
    total_time = current_time - start_time;
    
    uint32_t total_min;
    total_min = total_time / 60;
    uint32_t total_sec;
    total_sec = total_time % 60;


    time_t progress_time;
    progress_time = current_time - lap_progress_start_time;
    
    uint32_t progress_min;
    progress_min = progress_time / 60;
    uint32_t progress_sec;
    progress_sec = progress_time % 60;

    display_total_time(total_min, total_sec);
    display_lap_progress(progress_min, progress_sec);
    display_lap_1(lap_1_min, lap_1_sec);
    display_lap_2(lap_2_min, lap_2_sec);
    display_lap_cont(lap_cont);
}

void show_voltmeter(){
    // 0(0V)~1.0(3.3V) from ADC
    float voltage = voltmeter.read() * 3.3f;
    // convert ratio.
    voltage = voltage * 19.0f;

    display_voltmeter(voltage);
}
void show_ammeter(){
    // 0(0V)~1.0(3.3V) from ADC
    float current = ammeter_total.read() * 3.3f;
    // convert ratio.
    current = current * 5.0f;

    display_ammeter_total(current);

    // 0(0V)~1.0(3.3V) from ADC
    current = ammeter_cell.read() * 3.3f;
    // convert ratio.
    current = current * 5.0f;

    display_ammeter_cell(current);
}

void show_rpm() {
    float tire_rpm;
    float speed;

    tire_rpm = (float)rpm / gear_ratio;
    // convert min to hour
    speed = circumference * tire_rpm * 60.0f; 
    
    display_rpm(rpm);
    display_speed(speed);
}
 
int main() {
    
    // initialize    
    serialCtrl.baud(115200);
//    pc.baud(115200);
    wait(1.000000f);
    display_init();
    lap_sw.mode(PullUp);
    start_time = time(NULL);
    lap_progress_start_time = time(NULL);
    lap_last_irq_time = time(NULL);
    lap_cont = 0;
    lap_1_min = 0;
    lap_1_sec = 0;
    lap_2_min = 0;
    lap_2_sec = 0;
    // convert mm to km.
    circumference = (3.14f * (((float)tire_inchi * 25.4f) + (tire_flange * 2.0f))) / 1000000.0f;

    // set interrupter
    lap_sw.rise(&lap_interrupt);
    serialCtrl.attach(&ctrl_rx_interrupt, Serial::RxIrq);
 
      
    // main loop(wait=1.0s)
    while(1) {
        show_voltmeter();
        show_ammeter();
        show_lap();
        show_rpm();
        
        wait(1.000000f);
    }

}
