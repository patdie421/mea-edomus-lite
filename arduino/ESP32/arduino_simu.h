#ifndef arduino_simu_h
#define arduino_simu_h

#define false 0
#define true 1
#define bool int

#define SERIAL_AVAILABLE serial_available
#define SERIAL_READ serial_read
#define DISPLAY(x) printf("%s", (x))

int serial_available(void);
int serial_read(void);

#endif