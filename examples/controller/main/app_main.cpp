/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_controller_client.h>
#include <esp_matter_console.h>
#include <esp_matter_controller_console.h>
#include <esp_matter_controller_utils.h>
#include <esp_matter_ota.h>
#if CONFIG_OPENTHREAD_BORDER_ROUTER
#include <esp_matter_thread_br_cluster.h>
#include <esp_matter_thread_br_console.h>
#include <esp_matter_thread_br_launcher.h>
#include <esp_ot_config.h>
#endif // CONFIG_OPENTHREAD_BORDER_ROUTER
#include <common_macros.h>
#include <app_reset.h>

#include <app/server/Server.h>
#include <credentials/FabricTable.h>

#include "memfault/esp_port/core.h"
#include "memfault/components.h"

static const char *TAG = "app_main";
uint16_t switch_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::PublicEventTypes::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;
    case chip::DeviceLayer::DeviceEventType::kESPSystemEvent:
        if (event->Platform.ESPSystemEvent.Base == IP_EVENT &&
            event->Platform.ESPSystemEvent.Id == IP_EVENT_STA_GOT_IP) {
#if CONFIG_OPENTHREAD_BORDER_ROUTER
            esp_openthread_platform_config_t config = {
                .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
                .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
                .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
            };
            printf("init thread br\n");
            esp_matter::thread_br_init(&config);
#endif
        }
        break;
    default:
        break;
    }
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    // Memfault init
    memfault_boot();
    memfault_device_info_dump();

    /* Initialize the ESP NVS layer */
    nvs_flash_init();
#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();
#if CONFIG_ESP_MATTER_CONTROLLER_ENABLE
    esp_matter::console::controller_register_commands();
#endif // CONFIG_ESP_MATTER_CONTROLLER_ENABLE
#if CONFIG_OPENTHREAD_BORDER_ROUTER && CONFIG_OPENTHREAD_CLI
    esp_matter::console::thread_br_cli_register_command();
#endif // CONFIG_OPENTHREAD_BORDER_ROUTER && CONFIG_OPENTHREAD_CLI
#endif // CONFIG_ENABLE_CHIP_SHELL
    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    // Set up initial on-boot metrics:
    MEMFAULT_METRIC_SET_STRING(commissioning_status, "Commissioned");
    MEMFAULT_METRIC_SET_UNSIGNED(commissioning_attempts, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(commissioning_successes, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(commissioning_failures, 0);
    MEMFAULT_METRIC_SET_STRING(matter_current_mode, "Online");
    MEMFAULT_METRIC_SET_STRING(matter_controller_or_end_device, "Controller");
    MEMFAULT_METRIC_SET_STRING(matter_phys_iface, "WiFi");
    MEMFAULT_METRIC_SET_UNSIGNED(num_subscriptions, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(sum_subscriptions, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(matter_group_id, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(matter_keyset_id, 1);
    MEMFAULT_METRIC_SET_STRING(matter_spec_ver, "1.3");
    MEMFAULT_METRIC_SET_STRING(matter_node_name, "CTLR");
    MEMFAULT_METRIC_SET_UNSIGNED(matter_ble_commissioning, 1);
    MEMFAULT_METRIC_SET_UNSIGNED(matter_cluster_count, 7);

#if CONFIG_ESP_MATTER_COMMISSIONER_ENABLE
    esp_matter::lock::chip_stack_lock(portMAX_DELAY);
    esp_matter::controller::matter_controller_client::get_instance().init(112233, 1, 5580);
    esp_matter::controller::matter_controller_client::get_instance().setup_commissioner();
    esp_matter::lock::chip_stack_unlock();
#endif // CONFIG_ESP_MATTER_COMMISSIONER_ENABLE

    int counter = 0;
    while(true) {
        if (counter++ % 300 == 0) {  // do a thing
            ESP_LOGI(TAG, "Updating matter metrics...");
            MEMFAULT_METRIC_SET_UNSIGNED(matter_cluster_count, esp_random() % 3);
            MEMFAULT_METRIC_SET_UNSIGNED(num_subscriptions, esp_random() % 5);
            MEMFAULT_METRIC_SET_UNSIGNED(sum_subscriptions, esp_random() % 5);
            MEMFAULT_METRIC_SET_UNSIGNED(commissioning_attempts, esp_random() % 50);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
