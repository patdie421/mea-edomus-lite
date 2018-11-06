#define MAX_PULSES 5

struct pulse_context_s
{
   unsigned long prev_millis;
   unsigned int  duration;
   unsigned char pin;
   unsigned char front;
};

extern struct pulse_context_s pulses_context_list[MAX_PULSES];
extern unsigned char nb_active_pulses;

unsigned char pin_direction(unsigned char pin);
void init_pulses();
int pulseUp(unsigned char pin, unsigned int duration, unsigned char reactivable);
int pulse(unsigned char pin, unsigned int duration, unsigned char reactivable, unsigned char front);
void pulses();

