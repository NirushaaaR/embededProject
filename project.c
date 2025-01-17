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
//!#FUSES INTRC_IO 

#use delay(clock=8M)
#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8)
#define RUN_BUTTON   PIN_B7
#define CERVO  PIN_B4
#define MOTORGO  PIN_B6
#define MOTORSTOP PIN_B5
#define ADD_BUTTON PIN_B3
#define POP_BUTTON PIN_B2

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


const int period = 150;
const int16 start = 65486;

int cervo_work = 0;
int spining_wheel = 0;
int level = 17;
int counter = 0;
int sensor_value = 0;
int start_working = 0;
int start_count = 0;
int n = 0;
int motor_level = 30;


#INT_TIMER1
void timer1_isr() {
   set_timer1(start); 
   counter++;
   if(cervo_work){
       if (counter == period) { counter = 0;}
      if (counter == 0) { output_high(CERVO);} 
      if (counter == level) {
         output_low(CERVO);
      }
   }else if(spining_wheel) {
      
       if (counter == 250) {counter = 0;}
      if (counter == 0) { output_high(MOTORGO);} 
      if (counter == motor_level) {
         output_low(MOTORGO);
      }
   }
   
}


#INT_RB
void ext_isr() { 
   if (!input(RUN_BUTTON)) {
      //start_count = 1;
      if(n > 1){
         start_count = 2;
      }else{
         start_count = 1;
      }
      //start_working = 0;
      spining_wheel = 1;
      cervo_work = 0;
   }
}


int find_survivor(int number);
int infared_input(int& man,int& n,int kill_each,int& point_at, int* people,int16 predict_survivor);
void servo_control();
void stop_wheel();
void start_wheel();

void restore(int* people){
   int i;
   for(i = 0; i<n; i++){
      people[i] = 1;
   }
   for(i=n; i<12; i++){
      people[i] = 0;
   }
}

void main() {
   
   //set up infared sensor
    setup_adc_ports(sAN0); // setup PIN A0 as analog input 
    setup_adc( ADC_CLOCK_INTERNAL ); 
    set_adc_channel( 0 ); // set the ADC chaneel to read 
    delay_us(100); // wait for the sensor reading to finish
    
   // use button start button
   enable_interrupts(INT_RB7);
   enable_interrupts(GLOBAL);
   
   //set up timer
   //timer interrupt
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_4);
   set_timer1(start);

   cervo_work = 1;
   level = 9;
   enable_interrupts(INT_TIMER1);
   delay_ms(1000);
   cervo_work = 0;
   disable_interrupts(INT_TIMER1);
   counter = 0;
   
   int man = 0;
   int kill_each = 2;
   // how many people are there
 
   
   enable_interrupts(INT_TIMER1);
  
   int16 predict_survivor = 0;
   int people_survive[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
   
   int point_at = -1;
   int end = 0;
   start_count = 0;
   n=0;
   show_on7Sec(n);
   show_on7sec(sensor_value);
   
   start_working = 0;
   sensor_value = read_adc();
   
   
   while (!end) {
         //check infared
         //show_on7sec(read_adc());
         if(start_working || start_count>0){
         
               if(read_adc() >= sensor_value+13 && spining_wheel && !cervo_work){
                  stop_wheel();
                  delay_ms(150);
                  if(read_adc() >= sensor_value+18){
                  
                     if(start_count==1){
                        n++;
                        show_on7sec(n);
//!                        if(read_adc() <= sensor_value+315){
                           start_wheel(); 
                           delay_ms(450);
//!                        }else{
//!                           start_count = 2;
//!                        }
                        if(n==12){
                           start_count = 2;
                        }
                     }else if(start_count == 2){
                        start_count = 0;
                        restore(people_survive);
                        predict_survivor = 0;
                        show_on7sec(0+predict_survivor);
                        start_working=1;
                     }
                     
                     if(start_working){ 
                        end = infared_input(man, n, kill_each, point_at, people_survive,predict_survivor);
                     }
                     
                  }else{
                      start_wheel();     
                   }
               }
           
         }else{
            show_on7sec(read_adc());
            output_low(MOTORGO);
         }
         
         if(end){
            // stop working
            sleep();
            delay_ms(1000);
            end = 0;
            n = 12;
            restore(people_survive);
            point_at = -1;
            man = 0;
            start_working = 0;
            n=0;
            start_wheel();
         }
         
    }
}

int infared_input(int& man, int& n, int kill_each,int& point_at, int* people, int16 predict_survivor){
         
         stop_wheel();
         man++;
         
         do{
            point_at++;
            if(point_at == 12) point_at = 0;
         }while(people[point_at] == 0);
         
         show_on7Sec(point_at+1+predict_survivor);
         
         if(n <= 1){
            //stop working
            start_working = 0;
            return 1;
         }
            
            if(man < kill_each){
               // continue spinning
              start_wheel();
              delay_ms(430);
                  
            }else{
               //reset man
               man = 0;
              
               //show_on7Sec(n);
               do{
                  servo_control();
                  delay_ms(100);
               }while(read_adc() >= sensor_value+10);
                  
               // decreae people
               n--;
               start_wheel();
               people[point_at] = 0;
            }
         return 0;
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

      //set_timer1(start);
      cervo_work=1;
      enable_interrupts(INT_TIMER1);
      level = 14;
      delay_ms(1000);
      level = 9;
      delay_ms(1000);
      cervo_work = 0;
}

int find_survivor(int number){

   int msb = 1;
   while((msb << 1) <= number){
      msb <<= 1;
   }
   
   return (number-msb) * 2 + 1;

}

void start_wheel(){
//start wheel
   spining_wheel = 1;
   motor_level = 40;
   enable_interrupts(INT_TIMER1);
   delay_ms(150);
   motor_level = 30;
   enable_interrupts(INT_TIMER1);
                        
}
