#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/display.h"
#include "application.h"
#include "button.h"
#include "led/circular_strip.h"
#include "config.h"
#include "power_save_timer.h"
#include "adc_battery_monitor.h"
#include "press_to_talk_mcp_tool.h"

#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_mac.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "Ai04Board"

// Latch the power switch as early as possible, before app_main, so the
// device stays on after the power button is released while on battery.
static void Ai04HoldPower() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << POWER_HOLD_GPIO) | (1ULL << AUX_ENABLE_GPIO);
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    gpio_set_level(POWER_HOLD_GPIO, 1);
    gpio_set_level(AUX_ENABLE_GPIO, 1);
}

// The factory firmware already registered this device's MAC with the cloud
// server under another account. Use a locally administered MAC derived from
// the eFuse MAC so the device can be activated as a new device.
static void Ai04UseOwnMac() {
    uint8_t mac[6];
    if (esp_efuse_mac_get_default(mac) != ESP_OK) {
        return;
    }
    mac[0] |= 0x02;   // locally administered
    mac[0] &= ~0x01;  // unicast
    esp_base_mac_addr_set(mac);
}

__attribute__((constructor)) static void Ai04EarlyInit() {
    Ai04HoldPower();
    Ai04UseOwnMac();
}

class Ai04Board : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_ = nullptr;
    Display* display_ = nullptr;
    Button function_button_;
    Button power_button_;
    CircularStrip* led_strip_ = nullptr;
    PowerSaveTimer* power_save_timer_ = nullptr;
    AdcBatteryMonitor* adc_battery_monitor_ = nullptr;
    PressToTalkMcpTool* press_to_talk_tool_ = nullptr;

    void PowerOff() {
        ESP_LOGI(TAG, "Power off");
        gpio_set_level(AUDIO_CODEC_PA_PIN, 0);
        gpio_set_level(AUX_ENABLE_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(POWER_HOLD_GPIO, 0);
        // Still powered (USB plugged in): wait for the button to be released,
        // then sleep until it is pressed again.
        vTaskDelay(pdMS_TO_TICKS(500));
        while (gpio_get_level(POWER_BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        esp_deep_sleep_start();
    }

    void InitializePowerManager() {
        adc_battery_monitor_ = new AdcBatteryMonitor(BATTERY_ADC_UNIT, BATTERY_ADC_CHANNEL, 100000, 100000, BATTERY_CHARGING_GPIO);
        adc_battery_monitor_->OnChargingStatusChanged([this](bool is_charging) {
            power_save_timer_->SetEnabled(!is_charging);
        });
    }

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(160, 300);
        power_save_timer_->SetEnabled(true);
    }

    void InitializeCodecI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));

        if (i2c_master_probe(codec_i2c_bus_, 0x18, 1000) != ESP_OK) {
            ESP_LOGE(TAG, "ES8311 not found on I2C bus");
        }
    }

    void InitializeButtons() {
        function_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            if (!press_to_talk_tool_ || !press_to_talk_tool_->IsPressToTalkEnabled()) {
                app.ToggleChatState();
            }
        });
        function_button_.OnPressDown([this]() {
            if (power_save_timer_) {
                power_save_timer_->WakeUp();
            }
            if (press_to_talk_tool_ && press_to_talk_tool_->IsPressToTalkEnabled()) {
                Application::GetInstance().StartListening();
            }
        });
        function_button_.OnPressUp([this]() {
            if (press_to_talk_tool_ && press_to_talk_tool_->IsPressToTalkEnabled()) {
                Application::GetInstance().StopListening();
            }
        });

        power_button_.OnPressDown([this]() {
            if (power_save_timer_) {
                power_save_timer_->WakeUp();
            }
        });
        power_button_.OnClick([this]() {
            Application::GetInstance().ToggleChatState();
        });
        power_button_.OnLongPress([this]() {
            PowerOff();
        });
    }

    void InitializeTools() {
        press_to_talk_tool_ = new PressToTalkMcpTool();
        press_to_talk_tool_->Initialize();
    }

public:
    Ai04Board() :
        function_button_(FUNCTION_BUTTON_GPIO, false, 0, 0, true),
        power_button_(POWER_BUTTON_GPIO, false, 1500, 0, false) {
        Ai04HoldPower();
        display_ = new NoDisplay();
        InitializePowerManager();
        InitializePowerSaveTimer();
        InitializeCodecI2c();
        InitializeButtons();
        InitializeTools();
    }

    virtual Led* GetLed() override {
        if (led_strip_ == nullptr) {
            led_strip_ = new CircularStrip(LED_STRIP_GPIO, LED_STRIP_COUNT);
        }
        return led_strip_;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        charging = adc_battery_monitor_->IsCharging();
        discharging = adc_battery_monitor_->IsDischarging();
        level = adc_battery_monitor_->GetBatteryLevel();
        return true;
    }

    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        if (level != PowerSaveLevel::LOW_POWER) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveLevel(level);
    }
};

DECLARE_BOARD(Ai04Board);
