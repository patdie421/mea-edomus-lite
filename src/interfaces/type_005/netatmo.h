#ifndef __netatmo_h
#define __netatmo_h

#include <inttypes.h>

enum netatmo_setpoint_mode_e
{
   NONE=-1,
   PROGRAM=0,
   AWAY,
   HG,
   MANUAL,
   OFF,
   MAX,
};

extern char *netatmo_thermostat_mode[];

struct netatmo_token_s
{
   char access[256];
   char refresh[256]; 
   time_t expire_in;
};

struct netatmo_thermostat_data_s
{
   int battery_vp;
   int therm_relay_cmd;
   double temperature;
   enum netatmo_setpoint_mode_e setpoint;
   double setpoint_temp;
};


enum netatmo_data_types_e { TEMPERATURE=0, CO2, HUMIDITY, NOISE, PRESSURE, RAIN, WINDANGLE, WINDSTRENGTH, NETATMO_MAX_DATA };

#define TEMPERATURE_BIT (1 << TEMPERATURE)
#define CO2_BIT         (1 << CO2)
#define HUMIDITY_BIT    (1 << HUMIDITY)
#define NOISE_BIT       (1 << NOISE)
#define PRESSURE_BIT    (1 << PRESSURE)
#define RAIN_BIT        (1 << RAIN)
#define WINDANGLE_BIT   (1 << WINDANGLE)
#define WINDSTRENGTH_BIT (1 << WINDSTRENGTH)

struct netatmo_data_s
{
   int dataTypeFlags;
   float data[NETATMO_MAX_DATA];
};

struct netatmo_module_s
{
   char id[18];
   char name[256];
   char type[256];
   int battery;
   struct netatmo_data_s data;
};

struct netatmo_station_data_s
{
   char id[18];
   char name[25§];
   struct netatmo_data_s data;
   int nb_modules;
   struct netatmo_module_s modules_data[5];
};


int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_refresh_token(char *client_id, char *client_secret, struct netatmo_token_s *netatmo_token, char *err, int l_err);
int netatmo_set_thermostat_setpoint(char *access_token, char *relay_id, char *thermostat_id, enum netatmo_setpoint_mode_e mode, uint32_t delay, double temp, char *err, int l_err);
int netatmo_get_thermostat_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err);
int netatmo_get_station_data(char *access_token, char *station_id, struct netatmo_station_data_s *station_data, char *err, int l_err);

#endif

