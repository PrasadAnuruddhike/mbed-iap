#include <mbed.h>
#include "InAppProgram.h"

#define LED_D5              PF_12
#define LED_D6              PB_4
#define LED_D7              PD_15
#define LED_D8              PA_4
#define LED_D9              PD_14

#define BUTTON              PC_13


DigitalOut  led(LED_D7);
DigitalOut  firmware_up_led(LED_D5);

DigitalIn     button(BUTTON);
Thread led_thread;
Thread button_thread;

char firmware_server_host_name[]  = "firmware-update-manager.vercel.app";
char firmware_uri[]               = "download-simple-blink";
// char firmware_uri[]               = "download";

void led_blink_task()
{
  while(true)
  {
    led = 1;
    wait_us(500000);
    led = 0;
    wait_us(500000);
  }
}

void button_press_handler()
{
  while(1)
  {
    if(button.read())
    {
      firmware_up_led = 1;
      
      printf("\r\nStarting In Application Programming \r\n\r\n");
      InAppProgram *iap = new InAppProgram();
      iap->iapUpdate(firmware_server_host_name, firmware_uri);

      while(button.read());
    }
    else
    {
      firmware_up_led = 0;
    }
    wait_us(100000);
  }
}

int main(){
  button_thread.start(button_press_handler);
  // led_thread.start(led_blink_task);
  // printf("\r\nStarting In Application Programming \r\n\r\n");
  // InAppProgram *iap = new InAppProgram();

  // iap->iapUpdate();

  // return 0;

  while(1)
  {
    led = 1;
    wait_us(200000);
    led = 0;
    wait_us(1200000);
  }

  
}

// int main(){

//   int error = flash_bd.init();

//   if(error){
//     printf("[IAP-BDevice] Init Error, %d\r\n", error);
//   }else{
//     printf("[IAP-BDevice] Init\r\n");
//   }

//   bd_size_t flash_size = flash_bd.size();
//   bd_size_t flash_readable_block_size = flash_bd.get_read_size();
//   bd_size_t flash_programmable_block_size = flash_bd.get_program_size();
//   bd_size_t flash_erasable_block_size = flash_bd.get_erase_size();

//   printf("[IAP-BDevice] size: %llu\n",         flash_size);
//   printf("[IAP-BDevice] read size: %llu\n",    flash_readable_block_size);
//   printf("[IAP-BDevice] program size: %llu\n", flash_programmable_block_size);
//   printf("[IAP-BDevice] erase size: %llu\n",   flash_erasable_block_size);

//   // error = flash_bd.erase(0, flash_size);

//   if(error){
//     printf("[IAP-BDevice] Erase Error, %d\r\n", error);
//   }else{
//     printf("[IAP-BDevice] Erased\r\n");
//   }

//   bd_size_t firmware_buffer_size = sizeof(firmware)/sizeof(uint8_t);
//   int       firmware_chunk_count = firmware_buffer_size / CHUNK_SIZE;
//   uint8_t   firmware_last_chunk  = firmware_buffer_size % CHUNK_SIZE;
//   printf("[IAP-BDevice] firmware size: %llu, chunk count: %d, last chunk: %d\n", firmware_buffer_size, firmware_chunk_count, firmware_last_chunk);


//   uint8_t temp_buffer[CHUNK_SIZE];
//   uint8_t temp_last_chunk_buffer[firmware_last_chunk];

//   for(int c = 0; c < firmware_chunk_count; c ++){
//     memmove(temp_buffer, firmware + c * CHUNK_SIZE * sizeof(uint8_t), CHUNK_SIZE * sizeof(uint8_t));

//     // for (int i = 0; i < CHUNK_SIZE; i ++){
//     //   if((i % 16) == 0){
//     //     printf("\r\n");
//     //   }
//     //   printf("%02X ", temp_buffer[i]);
//     // }
//     // printf("\r\n");

//     error = flash_bd.program(temp_buffer, (c * CHUNK_SIZE), CHUNK_SIZE);
//     if(error){
//       printf("[IAP-BDevice] Program Error, %d\r\n", error);
//     }else{
//       printf("[IAP-BDevice] Programmed\r\n");
//     }

//   }

//   if(firmware_last_chunk > 0){
//     printf("%d \r\n", (firmware_chunk_count * CHUNK_SIZE) * sizeof(uint8_t));
//     memmove(temp_last_chunk_buffer, firmware + (firmware_chunk_count * CHUNK_SIZE) * sizeof(uint8_t), firmware_last_chunk * sizeof(uint8_t));

//     // for(int i = 0; i < firmware_last_chunk; i++){
//     //   if((i % 16) == 0){
//     //     printf("\r\n");
//     //   }
//     //   printf("%02X ", temp_last_chunk_buffer[i]);
//     // }
//     // printf("\r\n");

//     error = flash_bd.program(temp_last_chunk_buffer, (firmware_chunk_count * CHUNK_SIZE), firmware_last_chunk);
//     if(error){
//       printf("[IAP-BDevice] Program Error Last Chunk, %d\r\n", error);
//     }else{
//       printf("[IAP-BDevice] Programmed Last Chunk\r\n");
//     }
//   }

//   // memmove(temp_buffer, firmware + 5 * 128 * sizeof(uint8_t), 128 * sizeof(uint8_t));
//   // // for(int i = 0; i < 128; i++){
//   // //   temp_buffer[i] = firmware[(4 * 128) + i];
//   // // }

//   // for (int i = 0; i < 128; i ++){
//   //   if((i % 16) == 0){
//   //     printf("\r\n");
//   //   }
//   //   printf("%02X ", temp_buffer[i]);
//   // }
//   // printf("\r\n");

//   // error = flash_bd.program(temp_buffer, 0, 128);
//   // if(error){
//   //   printf("[IAP-BDevice] Program Error, %d\r\n", error);
//   // }else{
//   //   printf("[IAP-BDevice] Programmed\r\n");
//   // }

//   while (1)
//   {

//   }
  
// }

/*
int main() {

printf("******************************************************************\r\n");
#if BOOT0_FIRMWARE
  printf("Green LED Firmware from 0x08000000\r\n");
#else
  printf("Red LED Firmware from 0x08100000\r\n");
#endif


#if MBED_CONF_TARGET_FLASH_DUAL_BANK
  printf("Dual Bank Enable\n");
#else
  printf("Dual Bank Disabe\n");
#endif

  uint8_t active_bank = getCurrentFlashBank();
  printf("Active Flash Bank: %d\n", active_bank);

  uint8_t boot_map = getBootMap();
  printf("Boot Map: %d\n", boot_map);

  led_thread.start(led_blink_task);

  button.rise(&flip);

  while(1) {
    wait_us(30000000);

    swapBanks();
    printf("Reboot....\r\n\r\n");
    mcuSystemReset();

  }
}
*/