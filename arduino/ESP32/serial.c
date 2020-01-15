#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

//#define TXD_PIN (GPIO_NUM_4)
//#define RXD_PIN (GPIO_NUM_5)
#define TXD_PIN (UART_PIN_NO_CHANGE)
#define RXD_PIN (UART_PIN_NO_CHANGE)

#define UART_NUM UART_NUM_1

char *buf[RX_BUF_SIZE];
int buf_ptr=0;


inline unsigned long my_diff_millis(unsigned long chrono, unsigned long now)
{
  return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
}


void serial_init() {
   const uart_config_t uart_config = {
      .baud_rate  = 115200,
      .data_bits  = UART_DATA_8_BITS,
      .parity     = UART_PARITY_DISABLE,
      .stop_bits  = UART_STOP_BITS_1,
      .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
   };

   uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
   uart_param_config(UART_NUM, &uart_config);
   uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


int sendData(char *data, int l_data)
{
   return uart_write_bytes(UART_NUM, data, l_data);
}


int receiveData(char *data, int l_data, int timeout)
{
   int t_ref=(unsigned long)millis();
   char c;

   data_ptr=0;
   while(data_ptr<l_data-1) {
      if(my_diff_millis(t_ref, (unsigned long)millis()) > timeout) {
         return -1;
      }
      int nb=uart_read_bytes(UART_NUM, &c, 1, 10);
      if(nb) {
         if(c=='\n') {
            break;
         }
         data[data_ptr++]=c;
      }
   }
   data[data_ptr]=0;
   return data_ptr;
}
