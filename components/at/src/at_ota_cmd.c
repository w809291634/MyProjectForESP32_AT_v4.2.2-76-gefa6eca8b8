/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP32 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <string.h>
#include "malloc.h"
#include "stdlib.h"

#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "freertos/timers.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "esp_netif.h"

#include "esp_at.h"

#ifdef CONFIG_AT_OTA_SUPPORT
#include "at_ota.h"

typedef enum {
    AT_UPGRADE_SYSTEM_FIRMWARE = 0,         /**< upgrade type is system firmware */
    AT_UPGRADE_CUSTOM_PARTITION,            /**< upgrade type is custom partition */
} at_upgrade_type_t;

typedef enum {
    ESP_AT_OTA_STATE_IDLE = 0,              /**< not in upgrade */
    ESP_AT_OTA_STATE_FOUND_SERVER,          /**< the upgrade server found */
    ESP_AT_OTA_STATE_CONNECTED_TO_SERVER,      /**< connected to the server */
    ESP_AT_OTA_STATE_GOT_VERSION,           /**< got the version information to be upgraded */
    ESP_AT_OTA_STATE_DONE,                  /**< upgrade succeeded */
    ESP_AT_OTA_STATE_FAILED = -1,           /**< upgrade failed */
} esp_at_ota_state_t;

#ifdef CONFIG_AT_OTA_SSL_SUPPORT
#include "esp_tls.h"
#endif

#define TEXT_BUFFSIZE 1024

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

#define ESP_AT_OTA_TIMEOUT_MS               (60*3*1000)

static xTimerHandle esp_at_ota_timeout_timer = NULL;
static bool esp_at_ota_timeout_flag = false;
static int esp_at_ota_socket_id = -1;
static esp_at_ota_state_t s_ota_status = ESP_AT_OTA_STATE_IDLE;

#define ESP_AT_OTA_DEBUG  printf

#define ESP_AT_VERSION_LEN_MAX                64
#define ESP_AT_PARTITION_NAME_LEN_MAX         64
#define NB_OTA_TASK_STACK_SIZE              5120  // for non-blocking ota

typedef struct {
    int32_t ota_mode;
    char version[ESP_AT_VERSION_LEN_MAX+1];
    char partition_name[ESP_AT_PARTITION_NAME_LEN_MAX+1];
} ota_param_t;

static bool s_esp_at_ota_started = false;

static void esp_at_set_upgrade_state(esp_at_ota_state_t status)
{
    s_ota_status = status;
}

static esp_at_ota_state_t esp_at_get_upgrade_state(void)
{
    return s_ota_status;
}

static void esp_at_ota_timeout_callback( TimerHandle_t xTimer )
{
    ESP_AT_OTA_DEBUG("ota timeout!\r\n");
    esp_at_ota_timeout_flag = true;
    if (esp_at_ota_socket_id >= 0) {
        close(esp_at_ota_socket_id);
        esp_at_ota_socket_id = -1;
    }
}

bool esp_at_upgrade_process(esp_at_ota_mode_type ota_mode, uint8_t *version, const char *partition_name)
{
    bool pkg_body_start = false;
    struct sockaddr_in sock_info;
    ip_addr_t ip_address;
    struct hostent* hptr = NULL;
    uint8_t* http_request = NULL;
    uint8_t* data_buffer = NULL;
    uint8_t* pStr = NULL;
    esp_partition_t* partition_ptr = NULL;
    esp_partition_t partition;
    const esp_partition_t* next_partition = NULL;
    esp_ota_handle_t out_handle = 0;
    int buff_len = 0;
    int sockopt = 1;
    int total_len = 0;
    int recv_len = 0;
    bool ret = false;
    int result = -1;
    char* server_ip = NULL;
    uint16_t server_port = 0;
    const char* ota_key = NULL;
    uint32_t module_id = esp_at_get_module_id();
    at_upgrade_type_t upgrade_type = 0;
    const esp_partition_t *at_custom_partition = NULL;

#ifdef CONFIG_AT_OTA_SSL_SUPPORT
    esp_tls_t *tls = NULL;
    esp_tls_cfg_t *tls_cfg = NULL;
#endif

    if (memcmp(partition_name, "ota", strlen("ota")) == 0) {
        upgrade_type = AT_UPGRADE_SYSTEM_FIRMWARE;
    } else {
        upgrade_type = AT_UPGRADE_CUSTOM_PARTITION;
    }

    ESP_AT_OTA_DEBUG("ota_mode:%d\r\n",ota_mode);
    if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
        server_ip = CONFIG_AT_OTA_SERVER_IP;
        server_port = CONFIG_AT_OTA_SERVER_PORT;
    }
#ifdef CONFIG_AT_OTA_SSL_SUPPORT
    else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
        server_ip = CONFIG_AT_OTA_SSL_SERVER_IP;
        server_port = CONFIG_AT_OTA_SSL_SERVER_PORT;
    }
#endif
    else {
        return ret;
    }
    
    ota_key = esp_at_get_ota_token_by_id(module_id, ota_mode);
    if (esp_at_ota_timeout_timer != NULL) {
        xTimerStop(esp_at_ota_timeout_timer,portMAX_DELAY);
        xTimerDelete(esp_at_ota_timeout_timer,portMAX_DELAY);
        esp_at_ota_timeout_timer = NULL;
    }
    esp_at_ota_timeout_flag = false;
    esp_at_ota_timeout_timer = xTimerCreate("ota_timer",
                ESP_AT_OTA_TIMEOUT_MS/portTICK_PERIOD_MS,
                pdFAIL,
                NULL,
                esp_at_ota_timeout_callback);
    xTimerStart(esp_at_ota_timeout_timer,portMAX_DELAY);
    ip_address.u_addr.ip4.addr = inet_addr(server_ip);

    if ((ip_address.u_addr.ip4.addr == IPADDR_NONE) && (strcmp(server_ip,"255.255.255.255") != 0)) {
        if((hptr = gethostbyname(server_ip)) == NULL)
        {
            ESP_AT_OTA_DEBUG("gethostbyname fail\r\n");
            goto OTA_ERROR;
        }
        ip_address = *(ip_addr_t*)hptr->h_addr_list[0];
    }

    // set connect info
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = ip_address.u_addr.ip4.addr;
    sock_info.sin_port = htons(server_port);
    esp_at_set_upgrade_state(ESP_AT_OTA_STATE_FOUND_SERVER);
    esp_at_port_write_data((uint8_t*)"+CIPUPDATE:1\r\n",strlen("+CIPUPDATE:1\r\n"));

    http_request = (uint8_t*)malloc(TEXT_BUFFSIZE);
    if (http_request == NULL) {
        goto OTA_ERROR;
    }
    data_buffer = (uint8_t*)malloc(TEXT_BUFFSIZE);
    if (data_buffer == NULL) {
        goto OTA_ERROR;
    }

    if (version == NULL) {
        if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
            esp_at_ota_socket_id = socket(AF_INET, SOCK_STREAM, 0);
            if (esp_at_ota_socket_id < 0) {
                goto OTA_ERROR;
            }

            setsockopt(esp_at_ota_socket_id, SOL_SOCKET, SO_REUSEADDR,&sockopt, sizeof(sockopt));
            // connect to http server
            if (connect(esp_at_ota_socket_id, (struct sockaddr*)&sock_info, sizeof(sock_info)) < 0)
            {
                ESP_AT_OTA_DEBUG("connect to server failed!\r\n");
                goto OTA_ERROR;
            }
        }
    #ifdef CONFIG_AT_OTA_SSL_SUPPORT
        else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
            tls = esp_tls_init();

            tls_cfg = (esp_tls_cfg_t *)calloc(1, sizeof(esp_tls_cfg_t));
            if (tls_cfg == NULL) {
                goto OTA_ERROR;
            }

            if (esp_tls_conn_new_sync(server_ip, strlen(server_ip), server_port, tls_cfg, tls) < 0) {
                ESP_AT_OTA_DEBUG("Failed to open a new connection\r\n");
                goto OTA_ERROR;
            }
        }
    #endif
        esp_at_set_upgrade_state(ESP_AT_OTA_STATE_CONNECTED_TO_SERVER);
        esp_at_port_write_data((uint8_t*)"+CIPUPDATE:2\r\n",strlen("+CIPUPDATE:2\r\n"));

        snprintf((char*)http_request,TEXT_BUFFSIZE,"GET /v1/device/rom/?is_format_simple=true HTTP/1.0\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
                 IP2STR(&ip_address.u_addr.ip4),
                 server_port, ota_key);

        printf("http request length %d bytes\r\n",strlen((char*)http_request));
        /*send GET request to http server*/
        result = -1;
        if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
            result = write(esp_at_ota_socket_id, http_request, strlen((char*)http_request));
        }
    #ifdef CONFIG_AT_OTA_SSL_SUPPORT
        else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
            result = esp_tls_conn_write(tls, (const unsigned char *)http_request, strlen((char *)http_request));
        }
    #endif

        if(result != strlen((char*)http_request))
        {
            ESP_AT_OTA_DEBUG("send GET request to server failed\r\n");
            goto OTA_ERROR;
        }

        memset(data_buffer,0x0,TEXT_BUFFSIZE);

        result = -1;
        if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
            result = read(esp_at_ota_socket_id, data_buffer, TEXT_BUFFSIZE);
        }
    #ifdef CONFIG_AT_OTA_SSL_SUPPORT
        else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
            result = esp_tls_conn_read(tls, data_buffer, TEXT_BUFFSIZE);
        }
        esp_tls_conn_delete(tls);
        tls = NULL;
    #endif
        close(esp_at_ota_socket_id);
        esp_at_ota_socket_id = -1;

        if (result < 0) {
            ESP_AT_OTA_DEBUG("recv data from server failed!\r\n");
            goto OTA_ERROR;
        }
        pStr = (uint8_t*)strstr((char*)data_buffer,"rom_version\": ");
        if (pStr == NULL) {
            ESP_AT_OTA_DEBUG("rom_version error!\r\n");
            goto OTA_ERROR;
        }
        pStr += strlen("rom_version\": \"");
        version = pStr;
    
        pStr = (uint8_t*)strstr((char*)version,"\",");
        if (pStr == NULL) {
            ESP_AT_OTA_DEBUG("rom_version tail error!\r\n");
            goto OTA_ERROR;
        }
        *pStr = '\0';
        esp_at_set_upgrade_state(ESP_AT_OTA_STATE_GOT_VERSION);
        esp_at_port_write_data((uint8_t*)"+CIPUPDATE:3\r\n",strlen("+CIPUPDATE:3\r\n"));
    }
    printf("version:%s\r\n",version);

    snprintf((char*)http_request,TEXT_BUFFSIZE,
        "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s.bin HTTP/1.1\r\nHost: "IPSTR":%d\r\n"pheadbuffer"",
        (char*)version, partition_name, IP2STR(&ip_address.u_addr.ip4), server_port, ota_key);

    // search partition
    if (upgrade_type == AT_UPGRADE_SYSTEM_FIRMWARE) {  // search ota partition
        partition_ptr = (esp_partition_t*)esp_ota_get_boot_partition();
        if (partition_ptr == NULL) {
            ESP_AT_OTA_DEBUG("boot partition NULL!\r\n");
            goto OTA_ERROR;
        }
        if (partition_ptr->type != ESP_PARTITION_TYPE_APP)
        {
            ESP_AT_OTA_DEBUG("esp_current_partition->type != ESP_PARTITION_TYPE_APP\r\n");
            goto OTA_ERROR;
        }

        if (partition_ptr->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
            partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        } else {
            next_partition = esp_ota_get_next_update_partition(partition_ptr);

            if (next_partition) {
                partition.subtype = next_partition->subtype;
            } else {
                partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
            }
        }

        partition.type = ESP_PARTITION_TYPE_APP;

        partition_ptr = (esp_partition_t*)esp_partition_find_first(partition.type,partition.subtype,NULL);
        if (partition_ptr == NULL) {
            ESP_AT_OTA_DEBUG("partition NULL!\r\n");
            goto OTA_ERROR;
        }

        memcpy(&partition,partition_ptr,sizeof(esp_partition_t));
        if (esp_ota_begin(&partition, OTA_SIZE_UNKNOWN, &out_handle) != ESP_OK) {
            ESP_AT_OTA_DEBUG("esp_ota_begin failed!\r\n");
            goto OTA_ERROR;
        }
        ESP_AT_OTA_DEBUG("ready to upgrade system firmware.\r\n");
    } else {    // custom partition
        at_custom_partition = esp_at_custom_partition_find(0x0, 0x0, partition_name);
        if (at_custom_partition == NULL) {
            ESP_AT_OTA_DEBUG("no custom partition: %s\r\n", partition_name);
            goto OTA_ERROR;
        }
        ESP_AT_OTA_DEBUG("ready to upgrade partition: \"%s\" type:0x%x subtype:0x%x addr:0x%x size:0x%x encrypt:%d\r\n",
        at_custom_partition->label, at_custom_partition->type, at_custom_partition->subtype,
        at_custom_partition->address, at_custom_partition->size, at_custom_partition->encrypted);
        if (esp_partition_erase_range(at_custom_partition, 0, at_custom_partition->size) != ESP_OK) {
            ESP_AT_OTA_DEBUG("esp_partition_erase_range failed!\r\n");
            goto OTA_ERROR;
        }
    }

    if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
        esp_at_ota_socket_id = socket(AF_INET, SOCK_STREAM, 0);
        if (esp_at_ota_socket_id < 0) {
            goto OTA_ERROR;
        }
        setsockopt(esp_at_ota_socket_id, SOL_SOCKET, SO_REUSEADDR,&sockopt, sizeof(sockopt));
        if (connect(esp_at_ota_socket_id, (struct sockaddr*)&sock_info, sizeof(sock_info)) < 0) {
            ESP_AT_OTA_DEBUG("connect to server2 failed!\r\n");
            goto OTA_ERROR;
        }
    }
#ifdef CONFIG_AT_OTA_SSL_SUPPORT
    else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
        if (tls) {
            esp_tls_conn_delete(tls);
        }
        tls = esp_tls_init();
        if (!tls_cfg) {
            tls_cfg = (esp_tls_cfg_t *)calloc(1, sizeof(esp_tls_cfg_t));
        } else {
            memset(tls_cfg, 0x0, sizeof(esp_tls_cfg_t));
        }
        if (tls == NULL || tls_cfg == NULL) {
            goto OTA_ERROR;
        }

        if (esp_tls_conn_new_sync(server_ip, strlen(server_ip), server_port, tls_cfg, tls) < 0) {
            ESP_AT_OTA_DEBUG("Failed to open a new connection\r\n");
            goto OTA_ERROR;
        }
    }
#endif
    result = -1;
    if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
        result = write(esp_at_ota_socket_id, http_request, strlen((char*)http_request));
    }
#ifdef CONFIG_AT_OTA_SSL_SUPPORT
    else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
        result = esp_tls_conn_write(tls, (const unsigned char *)http_request, strlen((char *)http_request));
    }
#endif

    if(result != strlen((char*)http_request))
    {
        ESP_AT_OTA_DEBUG("send GET bin to server failed\r\n");
        goto OTA_ERROR;
    }
    /*deal with all receive packet*/
    for (;;)
    {
        memset(data_buffer, 0x0, TEXT_BUFFSIZE);
        
        buff_len = -1;
        if (ota_mode == ESP_AT_OTA_MODE_NORMAL) {
            buff_len = read(esp_at_ota_socket_id, data_buffer, TEXT_BUFFSIZE);
        }
#ifdef CONFIG_AT_OTA_SSL_SUPPORT
        else if (ota_mode == ESP_AT_OTA_MODE_SSL) {
            buff_len = esp_tls_conn_read(tls, data_buffer, TEXT_BUFFSIZE);
        }
#endif
        if (buff_len < 0) {
            ESP_AT_OTA_DEBUG("receive data error!\r\n");
            goto OTA_ERROR;
        } else if (buff_len > 0 && !pkg_body_start) {
            // search "\r\n\r\n"
            pStr = (uint8_t*)strstr((char*)data_buffer,"Content-Length: ");
            if (pStr == NULL) {
                break;
            }
            pStr += strlen("Content-Length: ");
            total_len = atoi((char*)pStr);
            ESP_AT_OTA_DEBUG("total_len=%d!\r\n",total_len);
            pStr = (uint8_t*)strstr((char*)data_buffer,"\r\n\r\n");
            if (pStr) {
                pkg_body_start = true;
                pStr += 4; // skip "\r\n"
                if (upgrade_type == AT_UPGRADE_SYSTEM_FIRMWARE) {
                    if (pStr[0] != 0xE9) {
                        ESP_AT_OTA_DEBUG("OTA Write Header format Check Failed! first byte is %02x\r\n", pStr[0]);
                        goto OTA_ERROR;
                    }
                }
                // pStr += 2;
                buff_len -= (pStr - data_buffer);

                if (upgrade_type == AT_UPGRADE_SYSTEM_FIRMWARE) {
                    if(esp_ota_write(out_handle, (const void*)pStr, buff_len) != ESP_OK)
                    {
                        ESP_AT_OTA_DEBUG("esp_ota_write failed!\r\n");
                        goto OTA_ERROR;
                    }
                } else {
                    if (esp_partition_write(at_custom_partition, recv_len, pStr, buff_len) != ESP_OK) {
                        ESP_AT_OTA_DEBUG("esp_partition_write failed!\r\n");
                        goto OTA_ERROR;
                    }
                }

                recv_len += buff_len;
            }
        } else if (buff_len > 0 && pkg_body_start) {
            if (upgrade_type == AT_UPGRADE_SYSTEM_FIRMWARE) {
                if(esp_ota_write( out_handle, (const void*)data_buffer, buff_len) != ESP_OK) {
                    ESP_AT_OTA_DEBUG("esp_ota_write failed!\r\n");
                    goto OTA_ERROR;
                }
            } else {
                if (esp_partition_write(at_custom_partition, recv_len, data_buffer, buff_len) != ESP_OK) {
                    ESP_AT_OTA_DEBUG("esp_partition_write failed!\r\n");
                    goto OTA_ERROR;
                }
            }

            recv_len += buff_len;
        } else if (buff_len == 0) {
            ESP_AT_OTA_DEBUG("receive all packet over!\r\n");
            if (recv_len != total_len) {
                goto OTA_ERROR;
            }
        } else {
            ESP_AT_OTA_DEBUG("Warning: uncontolled event!\r\n");
        }
        ESP_AT_OTA_DEBUG("total_len=%d(%d), %0.1f%%!\r\n",total_len,recv_len,(recv_len*1.0)*100/total_len);
        if (recv_len == total_len) {
            break;
        }
    }

    if (upgrade_type == AT_UPGRADE_SYSTEM_FIRMWARE) {
        if(esp_ota_end(out_handle) != ESP_OK)
        {
            ESP_AT_OTA_DEBUG("esp_ota_end failed!\r\n");
            goto OTA_ERROR;
        }

        if(esp_ota_set_boot_partition(&partition) != ESP_OK)
        {
            ESP_AT_OTA_DEBUG("esp_ota_set_boot_partition failed!\r\n");
            goto OTA_ERROR;
        }
    }
    esp_at_set_upgrade_state(ESP_AT_OTA_STATE_DONE);
    esp_at_port_write_data((uint8_t*)"+CIPUPDATE:4\r\n",strlen("+CIPUPDATE:4\r\n"));

    ret = true;
OTA_ERROR:
    if (esp_at_ota_timeout_timer != NULL) {
        xTimerStop(esp_at_ota_timeout_timer,portMAX_DELAY);
        xTimerDelete(esp_at_ota_timeout_timer,portMAX_DELAY);
        esp_at_ota_timeout_timer = NULL;
    }
    
    if (esp_at_ota_socket_id >= 0) {
        close(esp_at_ota_socket_id);
        esp_at_ota_socket_id = -1;
    }
    
    if (http_request) {
        free(http_request);
        http_request = NULL;
    }

    if (data_buffer) {
        free(data_buffer);
        data_buffer = NULL;
    }
    
#ifdef CONFIG_AT_OTA_SSL_SUPPORT
    if (tls_cfg) {
        free(tls_cfg);
    }

    esp_tls_conn_delete(tls);
    tls = NULL;
#endif
    return ret;
}

static uint8_t at_exeCmdCipupgrade(uint8_t *cmd_name)
{   
    if (s_esp_at_ota_started) {
        printf("ALREADY IN OTA\r\n");
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (esp_at_upgrade_process(ESP_AT_OTA_MODE_NORMAL, NULL, "ota")) {
        esp_at_response_result(ESP_AT_RESULT_CODE_OK);
        esp_at_port_wait_write_complete(ESP_AT_PORT_TX_WAIT_MS_MAX);
        esp_restart();
        for(;;){
        }
    }

    esp_at_set_upgrade_state(ESP_AT_OTA_STATE_FAILED);
    return ESP_AT_RESULT_CODE_ERROR;
}

static void non_blocking_upgrade_task(void *parameter)
{
    s_esp_at_ota_started = true;
    ota_param_t *p = (ota_param_t *)parameter;

    if (esp_at_upgrade_process((esp_at_ota_mode_type)(p->ota_mode), (uint8_t *)(p->version), p->partition_name)) {
        esp_at_port_wait_write_complete(ESP_AT_PORT_TX_WAIT_MS_MAX);
    } else {
        esp_at_set_upgrade_state(ESP_AT_OTA_STATE_FAILED);
        esp_at_port_write_data((uint8_t*)"+CIPUPDATE:-1\r\n", strlen("+CIPUPDATE:-1\r\n"));
    }

    free(p);
    p = NULL;
    s_esp_at_ota_started = false;
    vTaskDelete(NULL);
}

static uint8_t at_setupCmdCipupgrade(uint8_t para_num)
{
#define ESP_AT_NONBLOCKING_MODE_MAX     2
    ota_param_t *param = NULL;
    uint8_t *version = NULL, *partition_name = NULL;
    int32_t cnt = 0, ota_mode = 0, nonblocking = 0, version_len = 0;

    if (esp_at_get_para_as_digit(cnt++, &ota_mode) != ESP_AT_PARA_PARSE_RESULT_OK) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if ((ota_mode < 0) || (ota_mode >= ESP_AT_OTA_MODE_MAX)) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (cnt < para_num) {
        if (esp_at_get_para_as_str(cnt++, &version) == ESP_AT_PARA_PARSE_RESULT_FAIL) {
            return ESP_AT_RESULT_CODE_ERROR;
        }

        if (version != NULL) {
            version_len = strlen((const char *)version);
            if (version_len > ESP_AT_VERSION_LEN_MAX) {
                return ESP_AT_RESULT_CODE_ERROR;
            }
        }
    }

    if (cnt < para_num) {
        if (esp_at_get_para_as_str(cnt++, &partition_name) == ESP_AT_PARA_PARSE_RESULT_FAIL) {
            return ESP_AT_RESULT_CODE_ERROR;
        }
    }

    if ((partition_name == NULL) || (strlen((const char *)partition_name) == 0)) {
        partition_name = (uint8_t *)"ota";
    } else if ((strlen((const char *)partition_name)) > ESP_AT_PARTITION_NAME_LEN_MAX) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (cnt < para_num) {
        if (esp_at_get_para_as_digit(cnt++, &nonblocking) != ESP_AT_PARA_PARSE_RESULT_OK) {
            return ESP_AT_RESULT_CODE_ERROR;
        }
    }

    if ((nonblocking < 0) || (nonblocking >= ESP_AT_NONBLOCKING_MODE_MAX)) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (cnt != para_num) {
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (s_esp_at_ota_started) {
        printf("ALREADY IN OTA\r\n");
        return ESP_AT_RESULT_CODE_ERROR;
    }

    if (nonblocking == 1) {
        param = (ota_param_t *)calloc(1, sizeof(ota_param_t));
        if (param == NULL) {
            return ESP_AT_RESULT_CODE_ERROR;
        }

        param->ota_mode = ota_mode;

        if (version_len > 0) {
            memcpy(param->version, version, version_len);
        } else {
            version = NULL;
        }

        memcpy(param->partition_name, partition_name, strlen((const char *)partition_name));

        if (xTaskCreate(non_blocking_upgrade_task, "nb-ota", NB_OTA_TASK_STACK_SIZE, (void *)param, 5, NULL) != pdPASS) {
            free(param);
            param = NULL;
            return ESP_AT_RESULT_CODE_ERROR;
        }
        return ESP_AT_RESULT_CODE_OK;
    }

    if (esp_at_upgrade_process(ota_mode, version, (const char *)partition_name)) {
        if (memcmp(partition_name, "ota", strlen("ota")) == 0) {
            esp_at_response_result(ESP_AT_RESULT_CODE_OK);
            esp_at_port_wait_write_complete(ESP_AT_PORT_TX_WAIT_MS_MAX);
            esp_restart();
            for(;;){
            }
        } else {
            return ESP_AT_RESULT_CODE_OK;
        }
    }

    esp_at_set_upgrade_state(ESP_AT_OTA_STATE_FAILED);
    return ESP_AT_RESULT_CODE_ERROR;
}

static uint8_t at_queryCmdCipupgrade(uint8_t *cmd_name)
{
    uint8_t buffer[32] = {0};
    esp_at_ota_state_t state = esp_at_get_upgrade_state();
    snprintf((char *)buffer, sizeof(buffer), "%s:%d\r\n", cmd_name, state);
    esp_at_port_write_data(buffer, strlen((char *)buffer));
    return ESP_AT_RESULT_CODE_OK;
}

static esp_at_cmd_struct at_upgrade_cmd[] = {
    {"+CIUPDATE", NULL, at_queryCmdCipupgrade, at_setupCmdCipupgrade, at_exeCmdCipupgrade},
};

bool esp_at_ota_cmd_regist(void)
{
    return esp_at_custom_cmd_array_regist(at_upgrade_cmd, sizeof(at_upgrade_cmd)/sizeof(at_upgrade_cmd[0]));
}
#endif
