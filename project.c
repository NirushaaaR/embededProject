#include <16F886.h>
#device ADC=10 *=16

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES PUT                      //Power Up Timer
#FUSES NOMCLR                   //Master Clear pin not enabled
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOCPD                    //No EE protection
#FUSES BROWNOUT                 //Brownout reset
#FUSES IESO                     //Internal External Switch Over mode enabled
#FUSES FCMEN                    //Fail-safe clock monitor enabled
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES NOWRT                    //Program memory not write protected
#FUSES BORV40                   //Brownout reset at 4.0V
#FUSES RESERVED                 //Used to set the reserved FUSE bits
#FUSES INTRC_IO 

#use delay(clock=8M)
#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)
#define RUN_BUTTON   PIN_B7
#define CERVO  PIN_B4
#define MOTORGO  PIN_B6
#define MOTORSTOP PIN_B5
#define ADD_BUTTON PIN_B3
#define POP_BUTTON PIN_B2

const int period = 15;
const int16 start = 65036;

int cervo_work = 0;
int spining_wheel = 0;
int level = 1;
int counter = 0;
// for cervo control
int start_working = 0;
int status = 0; // 1 = counting 2 = kill people


// people number
int add_people = 0;
int pop_people = 0;

#define Slave_7Sec  0xB0
#use i2c(MASTER, I2C1, FORCE_HW) // configure the i2c port
void show_on7Sec(int16 number){

   int upper_byte = number >> 8;
   int lower_byte = number & 0xff;
   
   i2c_start();
   i2c_write(Slave_7Sec);
   i2c_write(2);  // set register pointer to location 2
   i2c_write(upper_byte); // send high byte
   i2c_write(lower_byte); // send low byte
   i2c_stop();
}

#INT_RB
void ext_isr() { 
   if (!input(RUN_BUTTON)) {
      start_working = 1;
      spining_wheel = spining_wheel == 1 ? 0 : 1;
      cervo_work = 0;
   }
   
   if(!input(ADD_BUTTON) && !start_working){
      add_people = 1;
   }
   
   if(!input(POP_BUTTON) && !start_working){
      pop_people = 1;
   }
}

#INT_TIMER1
void timer1_isr() {
   set_timer1(start); // re-set the timer1 counter to maintain a stable period
   counter++;
   if(cervo_work){
       if (counter == period) { counter = 0;}
      if (counter == 0) { output_high(CERVO);}  // beginning of a new period
      if (counter == level) {
         output_low(CERVO);
      }
   }else if(spining_wheel) {
       if (counter == 21) { counter = 0;}
      if (counter == 0) { output_high(MOTORGO);}  // beginning of a new period
      if (counter == 3) {
         output_low(MOTORGO);
      }
   }
  
}

int find_survivor(int number);
void count_people();
void infared_input(int& man,int& n,int kill_each);
void wheel_spin_control();
void servo_control();
void stop_wheel();

void main() {
   
   //set up infared sensor
    setup_adc_ports(sAN0); // setup PIN A0 as analog input 
    setup_adc( ADC_CLOCK_INTERNAL ); 
    set_adc_channel( 0 ); // set the ADC chaneel to read 
    delay_us(100); // wait for the sensor reading to finish
    
   // use button start button
   enable_interrupts(INT_RB7);
   enable_interrupts(INT_RB3);
   enable_interrupts(GLOBAL);
   
   //set up timer
   //timer interrupt
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_4);
   set_timer1(start);
   enable_interrupts(INT_TIMER1);
   
   
   
   int man = 0;
   int kill_each = 2;
   // how many people are there
   int n = 4;
   show_on7Sec(n);
   
   while (1) {
         //check infared
         if(start_working){
            //read_adc() > 20
            if( spining_wheel && !cervo_work)
               infared_input(man, n, kill_each);
            
            // control servo
            if(cervo_work)
               servo_control();

         }else{
            output_low(MOTORGO);
         }
         
         show_on7Sec(read_adc());
         
         if(add_people){
            n++;
            if(n > 16) n = 16;
            show_on7Sec(n);
            add_people = 0;
         }
         
         if(pop_people){
            n--;
            if(n < 1) n = 1;
            show_on7Sec(n);
            pop_people = 0;
         }
    }
}

void infared_input(int& man, int& n, int kill_each){

         //stop motor
         stop_wheel();
         if(n <= 1){
            //stop working
            start_working = 0;
            printf("stop working\r\n");
         }
         man++;
         printf("items: %d \r\n", man);
         delay_ms(1000);
         
         if(man < kill_each){
            // continue spinning
           wheel_spin_control();
             // delay a bit
            delay_ms(1000);
         }else{
            //reset man
            man = 0;
            //work cervo
            cervo_work = 1;
            // decreae people
            n--;
            show_on7Sec(n);
            printf("%d people left \r\n", n);
            if(n <= 1){
               //stop
               start_working = 0;
            }
         }
}

void wheel_spin_control(){
    spining_wheel = 1;
    output_high(MOTORGO);
    enable_interrupts(INT_TIMER1);
}

void stop_wheel(){
   disable_interrupts(INT_TIMER1);
   output_low(MOTORGO);
   output_high(MOTORSTOP);
   spining_wheel = 0;
   delay_ms(500);
   output_low(MOTORSTOP);
}

void servo_control(){

      set_timer1(start);
      enable_interrupts(INT_TIMER1);
      level = 1;
      delay_ms(1000);
      level = 2;
      delay_ms(1000);
      level = 1;
      delay_ms(1000);
      spining_wheel = 1;
      cervo_work = 0;
      output_high(MOTORGO);
      enable_interrupts(INT_TIMER1);
}

void count_people(){
   
}

int find_survivor(int number){
   
   while(number>>1 > 0){
      number >>= 1;
   }
   
   return number * 2 + 1;

}
