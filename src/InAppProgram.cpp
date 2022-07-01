#include "InAppProgram.h"

InAppProgram::InAppProgram()
    :_net(NetworkInterface::get_default_instance()),
    flash_bd(ADDR_SECONDARY_BANK, BLOCK_DEVICE_SIZE){}

InAppProgram::~InAppProgram()
{
    if(_net)
    {
        printf("[debug][network] disconected\r\n");
        _net->disconnect();
    }
}    

void InAppProgram::iapUpdate(char *host_name, char *uri)
{
    SocketAddress address;

    error_t error;
    error = NO_ERROR;

    // error = init_flash();

    // erase_flash();

    do
    {
        if(!_net)
        {
            printf("[error][network] no network interface found.\r\n");
            error = ERROR_NET_NO_INTERFACE;
            break;
        }

        printf("[debug][network] connecting to the network...\r\n");

        nsapi_size_or_error_t result = _net->connect();
        if (result != NSAPI_ERROR_OK) 
        {
            printf("[error][network] _net->connect() returned: %d\r\n", result);
            error = ERROR_NET_CONNECT;
            break;
        }
        print_network_info();

        result = _net->gethostbyname(host_name, &address);
        if (result != NSAPI_ERROR_OK) 
        {
            printf("[error][network] DNS resolution for %s failed with returned: %d\r\n", host_name, result);
            error = ERROR_NET_GET_HOST_IP;
            break;
        }

        address.set_port(REMOTE_PORT);

        result = _socket.open(_net);
        if (result != NSAPI_ERROR_OK) 
        {
            printf("[error][network] _socket.open() returned: %d\r\n", result);
            error = ERROR_SOCKET_OPEN;
            break;
        }

        _socket.set_hostname(host_name);

#if MBED_CONF_APP_USE_TLS_SOCKET
        result = _socket.set_root_ca_cert(root_ca_cert);
        if (result != NSAPI_ERROR_OK) 
        {
            printf("[error][network] _socket.set_root_ca_cert() returned %d\n", result);
            error = ERROR_SOCKET_SET_CERTIFICATE;
            break;
        }        
#endif

        printf("[debug][network] opening connection to remote port %d\r\n", REMOTE_PORT);
    
        result = _socket.connect(address);
        if (result != NSAPI_ERROR_OK) 
        {
            printf("[error][network] _socket.connect() returned: %d\r\n", result);
            error = ERROR_SOCKET_CONNECT;
            break;
        }

        error = send_http_request(host_name, uri);
        if(error != NO_ERROR)
        {
            break;
        }

        error = receive_http_response();
        if(error != NO_ERROR)
        {
            break;
        }
        
        // if (!send_http_request(host_name, uri)) 
        // {
        //     error = ERROR_HTTP_SEND;
        //     break;
        // }

        // if (!receive_http_response()) 
        // {
        //     error = ERROR_HTTP_RECEIVE;
        //     break;
        // }

        printf("[debug][network] new firmware installed successfully \r\n");

        // swap_bank();

        // printf("[debug][system] Restarting MCU.... \r\n");
        // mcu_system_reset();

    }while(0);



    /*
    if(!_net)
    {
        printf("[error][network] no network interface found.\r\n");
        return;
    }

    printf("[debug][network] connecting to the network...\r\n");

    nsapi_size_or_error_t result = _net->connect();
    if (result != NSAPI_ERROR_OK) 
    {
        printf("[error][network] _net->connect() returned: %d\r\n", result);
        return;
    }

    print_network_info();

    result = _net->gethostbyname(host_name, &address);
    if (result != NSAPI_ERROR_OK) 
    {
        printf("[error][network] DNS resolution for ifconfig.io failed with %d\n", result);
    }
    address.set_port(REMOTE_PORT);

    result = _socket.open(_net);
    if (result != NSAPI_ERROR_OK) 
    {
        printf("[error][network] _socket.open() returned: %d\r\n", result);
        return;
    }

    _socket.set_hostname(host_name);

#if MBED_CONF_APP_USE_TLS_SOCKET
    result = _socket.set_root_ca_cert(root_ca_cert);
    if (result != NSAPI_ERROR_OK) 
    {
        printf("[error][network] _socket.set_root_ca_cert() returned %d\n", result);
        return;
    }        
#endif

    printf("[debug][network] opening connection to remote port %d\r\n", REMOTE_PORT);
    
    result = _socket.connect(address);
    if (result != 0) 
    {
        printf("[error][network] _socket.connect() returned: %d\r\n", result);
        return;
    }

    if (!send_http_request(host_name, uri)) 
    {
        return;
    }

    if (!receive_http_response()) 
    {
        return;
    }

    printf("[debug][network] socket successfully \r\n");

    swap_bank();

    printf("[debug][system] Restarting MCU.... \r\n");
    mcu_system_reset();
    */
}

void InAppProgram::print_network_info()
{
    /* print the network info */
    SocketAddress a;
    _net->get_ip_address(&a);
    printf("[debug][network] IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    _net->get_netmask(&a);
    printf("[debug][network] netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    _net->get_gateway(&a);
    printf("[debug][network] gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
}

error_t InAppProgram::send_http_request(char *host_name, char *uri)
{
    char request_buffer[256];
    sprintf(request_buffer, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", uri, host_name);

    nsapi_size_t bytes_to_send = strlen(request_buffer);
    nsapi_size_or_error_t bytes_sent = 0;
    error_t error;

    printf("\n[debug][network] sending http request: \r\n%s", request_buffer);
    error = NO_ERROR;

    while (bytes_to_send) 
    {   
        bytes_sent = _socket.send(request_buffer + bytes_sent, bytes_to_send);
        if (bytes_sent < 0) 
        {
            printf("[error][network] _socket.send() returned: %d\r\n", bytes_sent);
            // return false;
            error = ERROR_HTTP_SEND;
            break;
        } 
        else 
        {
            printf("[debug][network] sent %d bytes\r\n", bytes_sent);
        }

        bytes_to_send -= bytes_sent;
    }

    printf("[debug][network] complete message sent\r\n");

    requsetState = HTTP_REQ_STATE_RECEIVE_STATUS_LINE;
    return error;
}

error_t InAppProgram::receive_http_response()
{
    char buffer[1024];
    nsapi_size_or_error_t result = 0;
    int processed_data_len = 0;
    int firmware_len = 0;

    int error_flash = 0;

    error_t error;

    error = NO_ERROR;
    /*
    while(1)
    {
        result = _socket.recv(buffer, 1024);

        if(result < 0)
        {
            printf("Error! _socket.recv() return: %d\r\n", result);
            return false;
        }
        else
        {
            for(int c = 0; c < result; c++){
                if((c % 16) == 0)
                {
                    printf("\r\n");
                }
                printf("%02X ", buffer[c]);
            }
        }
    }
    */
    while(1)
    {
        result = _socket.recv(buffer, 1024);
        
        if (result < 0) 
        {
            printf("[error][network] _socket.recv() return: %d\r\n", result);
            error = ERROR_HTTP_RECEIVE;
            break;
            // return false;
        }
        else
        {   
            while(1)
            {
                if(processed_data_len == result)
                {
                    printf("[debug][network] data packet done\r\n");
                    processed_data_len = 0;

                    if(requsetState == HTTP_REQ_STATE_PARSE_TRAILER)
                    {
                        printf("[debug][network] full image received \r\n");
                        requsetState = HTTP_REQ_STATE_COMPLETE;
                    } 
                    break;
                }
                else if(processed_data_len < result)
                {
                    if(requsetState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE)
                    {
                        printf("[debug][network] received length: %d\r\n", result);
                        // get status code
                        char target_string[] = "\r\n";
                        char *ptr_substring;
                        int status_code_len = 0;

                        ptr_substring = strstr(buffer, target_string);

                        if(ptr_substring)
                        {
                            // line end identified
                            status_code_len = (ptr_substring - buffer) + 2;
                            char status_code_buffer[status_code_len];

                            memmove(status_code_buffer, buffer, status_code_len * sizeof(char));

                            error = parse_http_status(status_code_buffer, status_code_len);
                            
                            if(error != NO_ERROR)
                            {
                                break;
                            }
                            // error_t error;
                            // error = NO_ERROR;
                            // error = init_flash();
                            // erase_flash();
                        }
                        else
                        {
                            printf("[debug][network] no status code found \r\n");
                        }

                        processed_data_len = processed_data_len + status_code_len;
                        requsetState = HTTP_REQ_STATE_RECEIVE_HEADER;
                    }
                    else if(requsetState == HTTP_REQ_STATE_RECEIVE_HEADER)
                    {
                        // get http header
                        char target_string[] = "\r\n\r\n";
                        char *ptr_substring;
                        uint32_t header_len = 0;

                        ptr_substring = strstr(buffer, target_string);

                        if(ptr_substring)
                        {
                            // Empty line identified
                            header_len = ((ptr_substring - buffer) + 4) - processed_data_len;
                            printf("\n[debug][network] header length: %d\r\n", header_len);
                            char header_buffer[header_len];

                            memmove(header_buffer, buffer + (processed_data_len * sizeof(char)), header_len * sizeof(char));
                            error = parse_http_header(header_buffer, header_len);
                            // for(int i = 0; i < header_len; i++)
                            // {
                            //     if((i % 16) == 0)
                            //     {
                            //         printf("\r\n");
                            //     }
                            //     printf("%02X ", header_buffer[i]);
                            // }
                            // printf("\n[debug][network] header found \r\n");
                        }
                        else
                        {
                            printf("[debug][network] no headers found \r\n");
                        }

                        processed_data_len = processed_data_len + header_len;
                        requsetState = HTTP_REQ_STATE_PARSE_IMAGE_HEADER;
                    }
                    else if(requsetState == HTTP_REQ_STATE_PARSE_IMAGE_HEADER)
                    {
                        // get image header
                        uint8_t img_header_len = 64;
                        uint8_t img_header[img_header_len];

                        memmove(img_header, buffer + (processed_data_len * sizeof(char)), img_header_len * sizeof(char));

                        // decode image header
                        error = parse_image_header(img_header, img_header_len);
                        if(error != NO_ERROR){
                            break;
                        }
                        processed_data_len = processed_data_len + img_header_len;
                        
                        // initiate flash 
                        error = init_flash();
                        if(error != NO_ERROR){
                            break;
                        }

                        error = erase_flash();
                        if(error != NO_ERROR){
                            break;
                        }

                        requsetState = HTTP_REQ_STATE_RECEIVE_BODY;
                    }
                    else if(requsetState == HTTP_REQ_STATE_RECEIVE_BODY)
                    {
                        int firmware_chunk_count = 0;
                        int firmware_last_chunk  = 0;
                        char temp_firmware_buffer[CHUNK_SIZE];
                        char temp_last_chunk_buffer[firmware_last_chunk];

                        if(firmware_len + result > imageHeader.dataSize)
                        {
                            printf("[debug][network] final data set\n");
                            firmware_chunk_count = (result - processed_data_len - 4) / CHUNK_SIZE;
                            firmware_last_chunk  = (result - processed_data_len - 4) % CHUNK_SIZE;
                        }
                        else
                        {
                            firmware_chunk_count = (result - processed_data_len) / CHUNK_SIZE;
                            firmware_last_chunk  = (result - processed_data_len) % CHUNK_SIZE;
                        }

                        printf("\n[debug][network] chunk count: %d, last chunk size: %d\n", firmware_chunk_count, firmware_last_chunk);

                        for(int c = 0; c < firmware_chunk_count; c++)
                        {
                            printf("[debug][network] processed len: %d\n", processed_data_len);
                            memmove(temp_firmware_buffer, buffer + processed_data_len * sizeof(uint8_t), CHUNK_SIZE * sizeof(uint8_t));
                            // debug_array(temp_firmware_buffer, CHUNK_SIZE);
                            
                            error_flash = flash_bd.program(temp_firmware_buffer, firmware_len, CHUNK_SIZE);

                            if(error_flash)
                            {
                                printf("[debug][flash] Program Error, %d\r\n", error_flash);
                                error = ERROR_FLASH_PROGRAM;
                                break;
                            }
                            
                            // error_flash = flash_bd.program(temp_firmware_buffer, firmware_len, CHUNK_SIZE);

                            // if(error_flash){
                            //     printf("[IAP-BDevice] Program Error, %d\r\n", error_flash);
                            // }
                            // else
                            // {
                            //     printf("[IAP-BDevice] Programmed\r\n");
                            // }
                            
                            processed_data_len = processed_data_len + CHUNK_SIZE;
                            firmware_len = firmware_len + CHUNK_SIZE;
                            printf("[debug][network] firmware len: %d\n", firmware_len);
                        }
                        // move firmware to 128 chunks
                        if(error == NO_ERROR)
                        {
                            if(firmware_last_chunk > 0)
                            {
                                printf("[debug][network] processed last chunk len: %d\n", processed_data_len);
                                memmove(temp_last_chunk_buffer, buffer + (processed_data_len) * sizeof(uint8_t), firmware_last_chunk * sizeof(uint8_t));
                                // debug_array(temp_last_chunk_buffer, firmware_last_chunk);

                                error_flash = flash_bd.program(temp_last_chunk_buffer, firmware_len, firmware_last_chunk);

                                if(error_flash)
                                {
                                    printf("[debug][flash] Program Error last chunk, %d\r\n", error_flash);
                                    error = ERROR_FLASH_PROGRAM;
                                    break;
                                }
                                // if(error_flash){
                                //     printf("[IAP-BDevice] Program Error Last Chunk, %d\r\n", error_flash);
                                // }
                                // else
                                // {
                                //     printf("[IAP-BDevice] Programmed Last Chunk\r\n");
                                // }

                                processed_data_len = processed_data_len + firmware_last_chunk;
                                firmware_len = firmware_len + firmware_last_chunk;
                                printf("[debug][network] firmware len: %d\n", firmware_len);
                            }
                        }
                        else
                        {
                            printf("[debug][flash] Error! terminate - flash program , %d\r\n", error_flash);
                            break;
                        }

                        // firmware_len = firmware_len + processed_data_len;
                        // printf("[debug] firmware len: %d\n", firmware_len);

                        if(firmware_len == imageHeader.dataSize){
                            printf("[debug][network] full firmware received\n");
                            requsetState = HTTP_REQ_STATE_RECEIVE_TRAILER;
                        }
                    }
                    else if(requsetState == HTTP_REQ_STATE_RECEIVE_TRAILER)
                    {
                        // get Integrity tag
                        uint8_t image_integrity_len = 4;
                        char image_integrity_tag[image_integrity_len];

                        memmove(image_integrity_tag, buffer + (processed_data_len * sizeof(char)), image_integrity_len * sizeof(char));
                        
                        for(int i = 0; i < image_integrity_len; i++)
                        {
                            if((i % 16) == 0)
                            {
                                printf("\r\n");
                            }
                            printf("%02X ", image_integrity_tag[i]);
                        }
                        printf("\n[debug][network] image integrity tag found \r\n");

                        processed_data_len = processed_data_len + image_integrity_len;
                        requsetState = HTTP_REQ_STATE_PARSE_TRAILER;
                    
                    }
                }
            }

            if(requsetState == HTTP_REQ_STATE_COMPLETE)
            {
                printf("[debug][network] complete http receive \r\n");
                error = NO_ERROR;
                break;
            }

            if(error != NO_ERROR){
                printf("[debug][network] terminate IAP \r\n");
                break;
            }

        }   
    }
        
    return error;
}

error_t InAppProgram::init_flash()
{
    error_t error;
    error = NO_ERROR;
    
    do
    {
        if(!flash_bd.init())
        {
            printf("[debug][flash] init\r\n");

            flash_size                      = flash_bd.size();
            flash_readable_block_size       = flash_bd.get_read_size();
            flash_programmable_block_size   = flash_bd.get_program_size();
            flash_erasable_block_size       = flash_bd.get_erase_size();

            printf("[debug][flash] size: %llu\n",         flash_size);
            printf("[debug][flash] read size: %llu\n",    flash_readable_block_size);
            printf("[debug][flash] program size: %llu\n", flash_programmable_block_size);
            printf("[debug][flash] erase size: %llu\n",   flash_erasable_block_size);
        }else
        {
            printf("[debug][flash] init error\r\n");
            error = ERROR_FLASH_INIT;
            break;
        }
    
    }while(0);

    return error;
    /*
    if(!flash_bd.init())
    {
        printf("[debug][flash] init\r\n");

        flash_size                      = flash_bd.size();
        flash_readable_block_size       = flash_bd.get_read_size();
        flash_programmable_block_size   = flash_bd.get_program_size();
        flash_erasable_block_size       = flash_bd.get_erase_size();

        printf("[debug][flash] size: %llu\n",         flash_size);
        printf("[debug][flash] read size: %llu\n",    flash_readable_block_size);
        printf("[debug][flash] program size: %llu\n", flash_programmable_block_size);
        printf("[debug][flash] erase size: %llu\n",   flash_erasable_block_size);

        return NO_ERROR;
    }
    else
    {
        printf("[debug][flash] init error\r\n");
        return ERROR_FLASH_INIT;
    }
    */
}

error_t InAppProgram::erase_flash()
{
    error_t error;
    error = NO_ERROR;

    do
    {
        if(!flash_bd.erase(0, flash_size))
        {
            printf("[debug][flash] Erased\r\n");
        }
        else
        {
            printf("[debug][flash] Erase Error\r\n");
            error = ERROR_FLASH_ERASE;
            break;   
        }
    } while (0);
    
    return error;
    // if(!flash_bd.erase(0, flash_size))
    // {
    //     printf("[debug][flash] Erased\r\n");
    //     return NO_ERROR;
    // }
    // else
    // {
    //     printf("[debug][flash] Erase Error\r\n");
    //     return ERROR_FLASH_ERASE;
    // }
}

error_t InAppProgram::parse_http_status(char *http_status, uint8_t http_status_len)
{
    char *token;
    char *p;
    error_t error;

    error = NO_ERROR;

    printf("\n[debug][network] status code found \r\n");
    debug_array(http_status, http_status_len);
    // for(int i = 0; i < http_status_len; i++)
    // {
    //     if((i % 16) == 0)
    //     {
    //         printf("\r\n");
    //     }
    //     printf("%02X ", http_status[i]);
    // }

    // parse http-version field
    do
    {
        token = strtok(http_status, " ");

        if(!strcmp(token, "HTTP/1.0"))
        {
            httpStatus.httpVersion = 100;
            // printf("[debug][http] HTTP/1.0\r\n");
        }
        else if(!strcmp(token, "HTTP/1.1"))
        {
            httpStatus.httpVersion = 101;
            // printf("[debug][http] HTTP/1.1\r\n");
        }
        else
        {
            error = ERROR_HTTP_INVALID_VERSION;
            printf("[debug][http] invalid HTTP version\r\n");
            break;
        }

        // parse status code
        token = strtok(NULL, " ");
        httpStatus.statusCode = strtoul(token, &p, 10);
        printf("[debug][http] HTTP Status Code: %d \r\n", httpStatus.statusCode);

        if(httpStatus.statusCode != 200)
        {
            printf("[debug][http] invalid HTTP Status Code\r\n");
            error = ERROR_HTTP_INVALID_STATUS_CODE;
            break;
        }

    }while(0);
    // parse status code
    // token = strtok(NULL, " ");
    // httpStatus.statusCode = strtol(token, &p, 10);
    // printf("[debug] HTTP Status Code: %d \r\n", httpStatus.statusCode);

    return error;
}

error_t InAppProgram::parse_http_header(char *http_header, uint32_t http_header_len)
{
    error_t error;
    error = NO_ERROR;

    printf("\n[debug][network] header found \r\n");
    debug_array(http_header, http_header_len);

    return error;
}

error_t InAppProgram::parse_image_header(uint8_t *img_header, uint8_t img_header_len)
{
    error_t error;
    error = NO_ERROR;

    printf("\n[debug][network] image header found \r\n");
    debug_uint8_array(img_header, img_header_len);
    
    imageHeader.headerVersion   = (img_header[3] << 24) + (img_header[2] << 16) + (img_header[1] << 8) +  img_header[0];
    imageHeader.imgIndex        = (img_header[7] << 24) + (img_header[6] << 16) + (img_header[5] << 8) +  img_header[4];
    imageHeader.imgType         = img_header[8];
    imageHeader.dataPadding     = (img_header[12] << 24) + (img_header[11] << 16) + (img_header[10] << 8) +  img_header[9];
    imageHeader.dataSize        = (img_header[16] << 24) + (img_header[15] << 16) + (img_header[14] << 8) +  img_header[13];
    imageHeader.dataVersion     = (img_header[20] << 24) + (img_header[19] << 16) + (img_header[18] << 8) +  img_header[17];
    imageHeader.imgTime         = (img_header[28] << 56) + (img_header[27] << 48) + (img_header[26] << 40) +  (img_header[25] << 32) + (img_header[24] << 24) + (img_header[23] << 16) + (img_header[22] << 8) +  img_header[21];
    imageHeader.headCRC         = (img_header[63] << 24) + (img_header[62] << 16) + (img_header[61] << 8) +  img_header[60];
    printf("[debug][flash] data size: 0x%x  data version: 0x%x\r\n ", imageHeader.dataSize, imageHeader.dataVersion);
    // printf("\n[debug][flash] CRC : 0x%x  timestamp: 0x%x\r\n ", imageHeader.headCRC, imageHeader.imgTime);

    // Add CRC check and assign errors

    return error;
}

uint8_t InAppProgram::get_current_bank()
{
    uint32_t swp_fb  = READ_BIT(SYSCFG->MEMRMP, 0x1 << 8);   
    return swp_fb == 0 ? 1 : 2;
}

void InAppProgram::swap_bank()
{
#if MBED_CONF_TARGET_FLASH_DUAL_BANK
    FLASH_OBProgramInitTypeDef    OBInit;
    HAL_StatusTypeDef             status;
    uint8_t                       f_currentbank_id;

    printf("[debug][swap] swaping device flash bank... \r\n");

    //Get current flash bank ID
    f_currentbank_id = get_current_bank();

    //Start of exception handling block
  do
  {
    //Allow access to Flash control registers and user False
    status = HAL_FLASH_Unlock();

    if(status != HAL_OK)
    {
      printf("[debug][swap] flash control register unlock fail\r\n");
      break;
    }

    //Start of exception handling block
    do
    {
      //Allow Access to option bytes sector
      status = HAL_FLASH_OB_Unlock();
      //Is any error?
      if (status != HAL_OK)
      {
        //Debug message
        printf("[debug][swap] flash option control registers unlock failed!\r\n");
        break;
      }

      //Get the Dual boot configuration status
      HAL_FLASHEx_OBGetConfig(&OBInit);

      //Swap in flash bank 2
      if(f_currentbank_id == 1)
      {
        printf("[debug][swap] swaping from flask bank 1 to flash bank 2...\r\n");

        //Configure option bytes to swap on flash bank 2
        OBInit.OptionType = OPTIONBYTE_BOOTADDR_0 | OPTIONBYTE_BOOTADDR_1;
        OBInit.BootAddr0  = __HAL_FLASH_CALC_BOOT_BASE_ADR(0x08100000);
        OBInit.BootAddr1  = __HAL_FLASH_CALC_BOOT_BASE_ADR(0x08000000);
      //Swap in flash bank 1
      }
      else
      {
        printf("[debug][swap] swaping from flask bank 2 to flash bank 1...\r\n");
        
        //Configure option bytes to swap on flash bank 1
        OBInit.OptionType = OPTIONBYTE_BOOTADDR_0 | OPTIONBYTE_BOOTADDR_1;
        OBInit.BootAddr0  = __HAL_FLASH_CALC_BOOT_BASE_ADR(0x08000000);
        OBInit.BootAddr1  = __HAL_FLASH_CALC_BOOT_BASE_ADR(0x08100000);
      }

      //Start of exception handling block
      do
      {
        //Start the Option Bytes programming process
        status = HAL_FLASHEx_OBProgram(&OBInit);

        if(status != HAL_OK)
        {
          printf("[debug][swap] option bytes programming process failed!\r\n");
          break;
        }

        //Launch the option byte loading
        status = HAL_FLASH_OB_Launch();

        if (status != HAL_OK)
        {
          printf("[debug][swap] option byte loading failed!\r\n");
        }
      } while (0);
      
      //Prevent Access to option bytes sector
      if(HAL_FLASH_OB_Lock() != HAL_OK)
      {
        printf("[debug][swap] flash option control register lock failed!\r\n");
      }
    } while (0);
    
    //Disable the Flash option control register access (recommended to protect 
    //the option Bytes against possible unwanted operations)
    if(HAL_FLASH_Lock() != HAL_OK)
    {
      printf("[debug][swap] flash control register lock failed!\r\n");
    }
  }while(0);

#elif
    printf("[Error][swap] enable MBED_CONF_TARGET_FLASH_DUAL_BANK")
#endif
}

void InAppProgram::mcu_system_reset()
{
    NVIC_SystemReset();
}

void InAppProgram::debug_array(char *buffer, uint32_t buffer_len)
{
    for(int i = 0; i < buffer_len; i++)
    {
        if((i > 0) && ((i % 16) == 0))
        {
            printf("\r\n");
        }
        printf("%02X ", buffer[i]);
    }
    printf("\r\n");
}

void InAppProgram::debug_uint8_array(uint8_t *buffer, uint8_t buffer_len)
{
    for(int i = 0; i < buffer_len; i++)
    {
        if((i > 0) && ((i % 16) == 0))
        {
            printf("\r\n");
        }
        printf("%02X ", buffer[i]);
    }
    printf("\r\n");
}
