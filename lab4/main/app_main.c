#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"

char *TAG = "Plavy Server";
uint8_t ble_addr_type;
void ble_app_advertise(void);

// Variables for characteristic 1
static int counterBLE = 0;
static int writeBLE = 2;
uint16_t counter_handle;

// Variables for charateristic 2
static const int readBLE = 22334;

static int read_counter(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char counter_str[10];
    sprintf(counter_str, "%d", counterBLE);

    os_mbuf_append(ctxt->om, &counter_str, strlen(counter_str));
    return 0;
}

static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char read_str[20];
    sprintf(read_str, "%d", readBLE);
    os_mbuf_append(ctxt->om, &read_str, strlen(read_str));
    return 0;
}

static int device_write(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    char write_str[10];
    sprintf(write_str, "%.*s", ctxt->om->om_len, ctxt->om->om_data);
    writeBLE = atoi(write_str);
    if (writeBLE < 1 || writeBLE > 10)
    {
        writeBLE = 1;
    }
    ESP_LOGI(TAG, "Updated writeBLE to %d", writeBLE);
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x2334), // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0x2335), // Define UUID for counter
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          .access_cb = read_counter,
          .val_handle = &counter_handle},
         {.uuid = BLE_UUID16_DECLARE(0x2336), // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0x2337), // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);
    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or nonconnectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                      ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}
void host_task(void *param)
{
    nimble_port_run();
}

void app_main()
{
    nvs_flash_init();                            // 1 - Initialize NVS flash using
    esp_nimble_hci_and_controller_init();        // 2 - Initialize ESP controller
    nimble_port_init();                          // 3 - Initialize the host stack
    ble_svc_gap_device_name_set("Plavy Server"); // 4 - Initialize NimBLE configuration - server name
    ble_svc_gap_init();                          // 4 - Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                         // 4 - Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);              // 4 - Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);               // 4 - Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;        // 5 - Initialize application
    nimble_port_freertos_init(host_task);        // 6 - Run the thread

    while (1)
    {
        ble_gatts_chr_updated(counter_handle);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        counterBLE += writeBLE;
    }
}