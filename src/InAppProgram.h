#include <mbed.h>
#include "root_ca_cert.h"
#include "FlashIAPBlockDevice.h"
#include "error.h"

#define MBED_CONF_APP_USE_TLS_SOCKET    1

// #define MBED_CONF_APP_HOSTNAME          "firmware-update-manager.vercel.app"
#define MASTER_FIRMWARE                 1


class InAppProgram{
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 128;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 443; // tls port
#else
    static constexpr size_t REMOTE_PORT = 80; // standard HTTP port
#endif 

    uint32_t  ADDR_SECONDARY_BANK       =   0x08100000;
    uint32_t  BLOCK_DEVICE_SIZE         =   512 * 1024;
    int       CHUNK_SIZE                =   128; 

    typedef enum
    {
        HTTP_REQ_STATE_INIT                = 0,
        HTTP_REQ_STATE_FORMAT_HEADER       = 1,
        HTTP_REQ_STATE_SEND_HEADER         = 2,
        HTTP_REQ_STATE_FORMAT_BODY         = 3,
        HTTP_REQ_STATE_SEND_BODY           = 4,
        HTTP_REQ_STATE_SEND_CHUNK_SIZE     = 5,
        HTTP_REQ_STATE_SEND_CHUNK_DATA     = 6,
        HTTP_REQ_STATE_FORMAT_TRAILER      = 7,
        HTTP_REQ_STATE_SEND_TRAILER        = 8,
        HTTP_REQ_STATE_RECEIVE_STATUS_LINE = 9,
        HTTP_REQ_STATE_RECEIVE_HEADER      = 10,
        HTTP_REQ_STATE_PARSE_HEADER        = 11,
        HTTP_REQ_STATE_RECEIVE_BODY        = 12,
        HTTP_REQ_STATE_PARSE_IMAGE_HEADER  = 13,
        // HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE  = 13,
        // HTTP_REQ_STATE_RECEIVE_CHUNK_DATA  = 14,
        // HTTP_REQ_STATE_PARSE_BODY          = 15,
        HTTP_REQ_STATE_RECEIVE_TRAILER     = 14,
        HTTP_REQ_STATE_PARSE_TRAILER       = 15,
        HTTP_REQ_STATE_COMPLETE            = 16
    } HttpRequestState;

    typedef struct
    {
        uint8_t     httpVersion;    // 100 --> 1.0,     101 --> 1.1
        uint32_t    statusCode;     // 200, 300, 308, 400, etc
    }http_status_t;

    typedef struct
    {
        uint32_t    headerVersion;
        uint32_t    imgIndex;
        uint8_t     imgType;
        uint32_t    dataPadding;
        uint32_t    dataSize;
        uint32_t    dataVersion;
        uint64_t    imgTime;
        uint8_t     reserved[31];
        uint32_t    headCRC;
    }image_header_t;

    private:
        NetworkInterface *_net;
        FlashIAPBlockDevice flash_bd;

#if MBED_CONF_APP_USE_TLS_SOCKET
        TLSSocket _socket;
#else
        TCPSocket _socket;
#endif // MBED_CONF_APP_USE_TLS_SOCKET
    
        error_t receive_http_response();
        error_t send_http_request(char *host_name, char *uri);
        void http_client_parse_status_line(char *line, size_t length); 
        // bool resolve_hostname(SocketAddress &address);
        void print_network_info();
        error_t init_flash();
        error_t erase_flash();
        
        error_t parse_http_status(char *http_status, uint8_t http_status_len);
        error_t parse_http_header(char *http_header, uint32_t http_header_len);
        error_t parse_image_header(uint8_t *img_header, uint8_t img_header_len);

        void swap_bank();
        uint8_t get_current_bank();
        
        void mcu_system_reset();

        void debug_array(char *buffer, uint32_t buffer_len);
        void debug_uint8_array(uint8_t *buffer, uint8_t buffer_len);

        HttpRequestState requsetState = HTTP_REQ_STATE_INIT;

        bd_size_t flash_size;
        bd_size_t flash_readable_block_size;
        bd_size_t flash_programmable_block_size;
        bd_size_t flash_erasable_block_size;

        http_status_t   httpStatus;
        image_header_t  imageHeader;

    public:
        InAppProgram();
        ~InAppProgram();

        void iapUpdate(char *host_name, char *uri);
};