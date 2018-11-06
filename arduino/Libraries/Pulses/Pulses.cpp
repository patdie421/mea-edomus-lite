#include <pulses.h>
#include <Arduino.h>

struct pulse_context_s pulses_context_list[MAX_PULSES];
unsigned char nb_active_pulses;

unsigned char pin_direction(unsigned char pin)
{
   unsigned char port;
   unsigned char output;
   unsigned char state;
   unsigned char mask;
   
   port=pin / 8;
   output=pin % 8;
   mask=1 << output;
   
   switch(port)
   {
      case 0:
         state = DDRD & mask;
         break;
      case 1:
         state = DDRB & mask;
         break;
      case 2:
         state = DDRC & mask;
         break;
   }
   if(state>0)
      return HIGH;
   else
      return LOW;
}


void init_pulses()
{
   for(int i=0;i<MAX_PULSES;i++)
      pulses_context_list[i].pin=0;
   nb_active_pulses=0;
}


int pulseUp(unsigned char pin, unsigned int duration, unsigned char reactivable)
{
   // réactivable or not
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==pin)
      {
         if(reactivable==1)
         {
            pulses_context_list[i].prev_millis=millis();
            return 0;
         }
         else
            return -1;
      }
   }
   
   if(nb_active_pulses==MAX_PULSES)
      return -1;
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==0)
      {
         digitalWrite(pin,HIGH);
         // toggle_output_pin(pulses_context_list[i].pin);
         nb_active_pulses++;
         pulses_context_list[i].pin=pin;
         pulses_context_list[i].duration=duration;
         pulses_context_list[i].prev_millis=millis();
         pulses_context_list[i].front=1; // pulsation positive
         break;
      }
   }
}


int pulse(unsigned char pin, unsigned int duration, unsigned char reactivable, unsigned char front)
{
   // réactivable or not
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==pin)
      {
         if(reactivable==1)
         {
            pulses_context_list[i].prev_millis=millis();
            return 0;
         }
         else
            return -1;
      }
   }
   
   if(nb_active_pulses==MAX_PULSES)
      return -1;
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==0)
      {
         if(front)
            digitalWrite(pin,HIGH);
         else
            digitalWrite(pin,LOW);

         nb_active_pulses++;
         pulses_context_list[i].pin=pin;
         pulses_context_list[i].duration=duration;
         pulses_context_list[i].prev_millis=millis();
         pulses_context_list[i].front=front;
         break;
      }
   }
}


void pulses()
{
   if(!nb_active_pulses)
      return;
   
   unsigned long now = millis();
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin!=0)
      {
         if( (now - pulses_context_list[i].prev_millis) > pulses_context_list[i].duration)
         {
            if(pulses_context_list[i].front==1)
               digitalWrite(pulses_context_list[i].pin, LOW);
            else
               digitalWrite(pulses_context_list[i].pin, HIGH);
               
            nb_active_pulses--;
            pulses_context_list[i].pin=0;
         }
      }
   }
}


