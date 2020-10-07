// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "driver/i2s.h"

#include "driver/gpio.h"
#include "soc/io_mux_reg.h"


/* event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

/* handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param);


void app_main()
{
    /* Initialize NVS 窶� it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // for I2S Master mode
	#define I2S_MASTER_MODE
	#ifdef I2S_MASTER_MODE
    i2s_config_t i2s_config = {
#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
#else
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
#endif
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
		//.communication_format = I2S_COMM_FORMAT_I2S_MSB,
		.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,

		//.dma_buf_count = 6,
		// changed by nishi
        //.dma_buf_count = 12,
        //.dma_buf_count = 18,
        .dma_buf_count = 20,
        //.dma_buf_len = 60,							// APTX 1 デコードサイズ  2,688 バイト / 1フレーム
		// changed by nishi
        //.dma_buf_len = 120,
        //.dma_buf_len = 240,
        //.dma_buf_len = 480,
        .dma_buf_len = 672,
        //.dma_buf_len = 896,
        //.intr_alloc_flags = 0,                        //Default interrupt priority
		// changed by nishi
		//.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 		// high interrupt priority
		//.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2, 		// high interrupt priority
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL3, 		// high interrupt priority
		//.intr_alloc_flags = ESP_INTR_FLAG_LEVEL4, 			// high interrupt priority
		//.intr_alloc_flags = ESP_INTR_FLAG_NMI,
        .tx_desc_auto_clear = true,                        //Auto clear tx descriptor on underflow
		// changed by nishi
        //.tx_desc_auto_clear = false,                        //Auto clear tx descriptor on underflow
		// add by nishi
		//.use_apll = false
		.fixed_mclk=32,
		.use_apll = 1				// CLK_OUT1 enabled
    };
	#else
    // for I2S Slave mode
    i2s_config_t i2s_config = {
#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
#else
        //.mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
		.mode =  I2S_MODE_SLAVE | I2S_MODE_TX,
#endif
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
		//.communication_format = I2S_COMM_FORMAT_I2S_MSB,
		.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,

		//.dma_buf_count = 6,
		// changed by nishi
        //.dma_buf_count = 12,
        //.dma_buf_count = 18,
        .dma_buf_count = 20,
        //.dma_buf_len = 60,							// APTX 1 デコードサイズ  2,688 バイト / 1フレーム
		// changed by nishi
        //.dma_buf_len = 120,
        //.dma_buf_len = 240,
        //.dma_buf_len = 480,
        .dma_buf_len = 672,
        //.dma_buf_len = 896,
        //.intr_alloc_flags = 0,                        //Default interrupt priority
		// changed by nishi
		//.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 		// high interrupt priority
		//.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2, 		// high interrupt priority
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL3, 			// high interrupt priority
        .tx_desc_auto_clear = true,                        //Auto clear tx descriptor on underflow
		// changed by nishi
        //.tx_desc_auto_clear = false                        //Auto clear tx descriptor on underflow
		// add by nishi
		.use_apll = false
    };
	#endif


    i2s_driver_install(0, &i2s_config, 0, NULL);
#ifdef CONFIG_A2DP_SINK_OUTPUT_INTERNAL_DAC
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    i2s_set_pin(0, NULL);
#else
    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S_BCK_PIN,				// 26   BCK (clock)
        .ws_io_num = CONFIG_I2S_LRCK_PIN,				// 22   LRCK / Word select
        .data_out_num = CONFIG_I2S_DATA_PIN,			// 25	DIN (data)
        .data_in_num = -1                                                       //Not used
    };

    i2s_set_pin(0, &pin_config);

	 // CLK_OUT1 ->  GPIO0  ->  SCK pin on PCM5102A
	 // https://www.esp32.com/viewtopic.php?t=1778
	 PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
	 WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) & 0xFFFFFFF0);

#endif

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #1 esp_bluedroid_init() call!",__func__);

    if ((err = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"main.c : #2 esp_bluedroid_enable() call!");

    if ((err = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }


    // Test by nishi
    /* bt_target.h
#if (AVRC_INCLUDED == TRUE)

#ifndef AVDT_DEBUG
#define AVDT_DEBUG  FALSE
#endif
	*/

#if (AVRC_INCLUDED == TRUE)
    // debug by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #3 AVRC_INCLUDED == TRUE",__func__);
#else
    ESP_LOGI(BT_AV_TAG,"%s(): #3.2 AVRC_INCLUDED == FALSE",__func__);
#endif
    //ESP_LOGI(BT_AV_TAG,"main.c : #4 AVDT_SetTraceLevel(0xff)=%x",AVDT_SetTraceLevel(0xff));

    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #4 bt_app_task_start_up() call!",__func__);
    /* create application task */
    bt_app_task_start_up();


    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #5 bt_app_work_dispatch() call!",__func__);
    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);


    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #6 esp_bt_gap_set_security_param() call!",__func__);

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));



    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #7 esp_bt_gap_set_pin() call!",__func__);

    /*
     * Set default parameters for Legacy Pairing
     * Use fixed pin code
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    // DEBUG by nishi
    ESP_LOGI(BT_AV_TAG,"%s(): #8 exit",__func__);
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    // add by nishi start
    //ESP_BT_GAP_DISC_RES_EVT = 0,                    /*!< device discovery result event */
    //ESP_BT_GAP_DISC_STATE_CHANGED_EVT,              /*!< discovery state changed event */
    //ESP_BT_GAP_RMT_SRVCS_EVT,                       /*!< get remote services event */
    //ESP_BT_GAP_RMT_SRVC_REC_EVT,                    /*!< get remote service record event */
    //ESP_BT_GAP_AUTH_CMPL_EVT,                       /*!< AUTH complete event */
    //ESP_BT_GAP_PIN_REQ_EVT,                         /*!< Legacy Pairing Pin code request */
    //ESP_BT_GAP_CFM_REQ_EVT,                         /*!< Simple Pairing User Confirmation request. */
    //ESP_BT_GAP_KEY_NOTIF_EVT,                       /*!< Simple Pairing Passkey Notification */
    //ESP_BT_GAP_KEY_REQ_EVT,                         /*!< Simple Pairing Passkey request */
    //ESP_BT_GAP_READ_RSSI_DELTA_EVT,                 /*!< read rssi event */
    //ESP_BT_GAP_EVT_MAX,
    // add by nishi end

	switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {					/*!< AUTH complete event */
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_AV_TAG, "%s(): authentication success: %s", __func__,param->auth_cmpl.device_name);
            esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(BT_AV_TAG, "%s(): authentication failed, status:%d", __func__,param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_CFM_REQ_EVT:						/*!< Simple Pairing User Confirmation request. */
        ESP_LOGI(BT_AV_TAG, "%s(): ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", __func__,param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:						/*!< Simple Pairing Passkey Notification */
        ESP_LOGI(BT_AV_TAG, "%s(): ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", __func__,param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:						/*!< Simple Pairing Passkey request */
        ESP_LOGI(BT_AV_TAG, "%s(): ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!",__func__);
        break;
    default: {
        ESP_LOGI(BT_AV_TAG, "%s(): event: %d", __func__,event);
        break;
    }
    }
    return;
}
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s(): evt %d", __func__, event);
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        /* set up device name */
        char *dev_name = "ESP_SPEAKER";
        esp_bt_dev_set_device_name(dev_name);

        esp_bt_gap_register_callback(bt_app_gap_cb);
        /* initialize A2DP sink */
        esp_a2d_register_callback(&bt_app_a2d_cb);
        esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
        esp_a2d_sink_init();

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s(): unhandled evt %d", __func__, event);
        break;
    }
}
