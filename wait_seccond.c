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

int counter=0;
//start timer at 4 hz == 0.25 seccond
const int16 startTimer=3036;
int start_count = 0;

#INT_TIMER1
void timer1_isr() {
   set_timer1(startTimer); // re-set the timer1 counter to maintain a stable period
   counter++;
}

#INT_RB
void ext_isr() { 
   if (!input(PIN_B7)) {
      if(!start_count){
         start_count = 1;
      }
   }
}

void main()
{
   // setup timer
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
   set_timer1(startTimer);
//!   enable_interrupts(INT_TIMER2);
   enable_interrupts(GLOBAL);
      
   int i=0;
  
   //! io interrupt
   enable_interrupts(INT_RB7);
   enable_interrupts(GLOBAL);
   //press to start counting

   while(1) {
   
        if(start_count){
            enable_interrupts(INT_TIMER1);
            start_count=0;
        }
         
        if(counter == 4){
            counter = 0;
            printf("time: %d \r\n",++i);
            if(i == 5 ){
               printf("stop time at: %d \r\n",i);
               disable_interrupts(INT_TIMER1);
               i=0;
           }
        }  
        
        
   }

}
