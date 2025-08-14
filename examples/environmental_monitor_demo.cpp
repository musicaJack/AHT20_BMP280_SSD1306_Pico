/*
 * 环境监测器 - ILI9488显示版本
 * 
 * 功能：
 * - AHT20温湿度传感器数据采集
 * - BMP280气压、温度、海拔数据采集
 * - ILI9488 3.5寸彩色显示屏显示
 * - 数据局部刷新，避免全屏刷新
 * - 参考图片样式的界面设计
 *
 * 硬件配置：
 * - ILI9488 TFT-LCD显示屏 (3.5寸, 320x480分辨率)
 * - AHT20温湿度传感器 (I2C地址: 0x38)
 * - BMP280气压传感器 (I2C地址: 0x77)
 *
 * 引脚连接：
 * ILI9488显示屏 (SPI0):
 *   Pico GPIO20  -> ILI9488 DC (数据/命令)
 *   Pico GPIO15  -> ILI9488 RST (复位)
 *   Pico GPIO16 -> ILI9488 BL (背光)
 *   Pico GPIO17 -> ILI9488 CS (片选)
 *   Pico GPIO18 -> ILI9488 SCK (时钟)
 *   Pico GPIO19 -> ILI9488 MOSI (数据)
 *
 * 传感器 (I2C1):
 *   Pico GPIO6 -> SDA
 *   Pico GPIO7 -> SCL
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/display/ili9488_driver.hpp"
#include "config/ili9488_config.hpp"
#include "EnvironmentalMonitor.hpp"

// I2C配置
#define I2C_PORT i2c1
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
#define I2C_FREQ 100000  // 100kHz

// AHT20传感器配置
#define ATH20_SLAVE_ADDRESS    0x38
#define ATH20_INIT_CMD         0xBE
#define ATH20_SOFT_RESET_CMD   0xBA
#define ATH20_START_TEST_CMD   0xAC

// BMP280传感器配置
#define BMP280_SLAVE_ADDRESS   0x77

// BMP280寄存器地址
#define BMP280_DIG_T1_LSB_REG                0x88
#define BMP280_DIG_T1_MSB_REG                0x89
#define BMP280_DIG_T2_LSB_REG                0x8A
#define BMP280_DIG_T2_MSB_REG                0x8B
#define BMP280_DIG_T3_LSB_REG                0x8C
#define BMP280_DIG_T3_MSB_REG                0x8D
#define BMP280_DIG_P1_LSB_REG                0x8E
#define BMP280_DIG_P1_MSB_REG                0x8F
#define BMP280_DIG_P2_LSB_REG                0x90
#define BMP280_DIG_P2_MSB_REG                0x91
#define BMP280_DIG_P3_LSB_REG                0x92
#define BMP280_DIG_P3_MSB_REG                0x93
#define BMP280_DIG_P4_LSB_REG                0x94
#define BMP280_DIG_P4_MSB_REG                0x95
#define BMP280_DIG_P5_LSB_REG                0x96
#define BMP280_DIG_P5_MSB_REG                0x97
#define BMP280_DIG_P6_LSB_REG                0x98
#define BMP280_DIG_P6_MSB_REG                0x99
#define BMP280_DIG_P7_LSB_REG                0x9A
#define BMP280_DIG_P7_MSB_REG                0x9B
#define BMP280_DIG_P8_LSB_REG                0x9C
#define BMP280_DIG_P8_MSB_REG                0x9D
#define BMP280_DIG_P9_LSB_REG                0x9E
#define BMP280_DIG_P9_MSB_REG                0x9F

#define BMP280_CHIPID_REG                    0xD0
#define BMP280_RESET_REG                     0xE0
#define BMP280_STATUS_REG                    0xF3
#define BMP280_CTRLMEAS_REG                  0xF4
#define BMP280_CONFIG_REG                    0xF5
#define BMP280_PRESSURE_MSB_REG              0xF7
#define BMP280_PRESSURE_LSB_REG              0xF8
#define BMP280_PRESSURE_XLSB_REG             0xF9
#define BMP280_TEMPERATURE_MSB_REG           0xFA
#define BMP280_TEMPERATURE_LSB_REG           0xFB
#define BMP280_TEMPERATURE_XLSB_REG          0xFC

// BMP280工作模式
#define BMP280_SLEEP_MODE                    (0x00)
#define BMP280_FORCED_MODE                   (0x01)
#define BMP280_NORMAL_MODE                   (0x03)

// BMP280过采样设置
#define BMP280_OVERSAMP_SKIPPED              (0x00)
#define BMP280_OVERSAMP_1X                   (0x01)
#define BMP280_OVERSAMP_2X                   (0x02)
#define BMP280_OVERSAMP_4X                   (0x03)
#define BMP280_OVERSAMP_8X                   (0x04)
#define BMP280_OVERSAMP_16X                  (0x05)

#define BMP280_PRESSURE_OSR                  (BMP280_OVERSAMP_8X)
#define BMP280_TEMPERATURE_OSR               (BMP280_OVERSAMP_16X)
#define BMP280_MODE                          (BMP280_PRESSURE_OSR << 2 | BMP280_TEMPERATURE_OSR << 5 | BMP280_NORMAL_MODE)

// BMP280校准数据结构
typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int32_t t_fine;
} bmp280Calib;

// 全局变量
static bmp280Calib bmp280Cal;
static int32_t bmp280RawPressure = 0;
static int32_t bmp280RawTemperature = 0;

// 常量定义
#define CONST_PF 0.1902630958f
#define FIX_TEMP 25
#define FILTER_NUM 5
#define FILTER_A 0.1f

// 全局对象
ili9488::ILI9488Driver* g_lcd_driver = nullptr;
environmental_monitor::EnvironmentalMonitor* g_env_monitor = nullptr;

// 延时函数
void delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

// I2C初始化
void i2c_init_hardware() {
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    printf("[I2C] 初始化完成，频率: %d Hz\n", I2C_FREQ);
}

// 检测I2C设备
bool i2c_detect_device(uint8_t address) {
    uint8_t dummy = 0;
    int result = i2c_write_blocking(I2C_PORT, address, &dummy, 1, false);
    return (result == 1);
}

// I2C写寄存器
int i2c_write_register(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len) {
    uint8_t buf[32];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    
    int result = i2c_write_blocking(I2C_PORT, address, buf, len + 1, false);
    return (result == len + 1) ? 0 : -1;
}

// I2C读寄存器
int i2c_read_register(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len) {
    int result = i2c_write_blocking(I2C_PORT, address, &reg, 1, true);
    if (result != 1) return -1;
    
    result = i2c_read_blocking(I2C_PORT, address, data, len, false);
    return (result == len) ? 0 : -1;
}

// AHT20读取状态寄存器
uint8_t ATH20_Read_Status() {
    uint8_t status;
    if (i2c_read_register(ATH20_SLAVE_ADDRESS, 0x00, &status, 1) == 0) {
        return status;
    }
    return 0xFF;
}

// AHT20检查校准状态
uint8_t ATH20_Read_Cal_Enable() {
    uint8_t val = ATH20_Read_Status();
    if ((val & 0x68) == 0x08) {
        return 1;
    }
    return 0;
}

// AHT20读取温湿度数据
void ATH20_Read_CTdata(uint32_t *ct) {
    uint8_t tmp[2] = {0x33, 0x00};
    uint8_t data[10];
    uint16_t cnt = 0;
    
    // 发送测量命令
    if (i2c_write_register(ATH20_SLAVE_ADDRESS, ATH20_START_TEST_CMD, tmp, 2) != 0) {
        printf("[AHT20] 发送测量命令失败\n");
        ct[0] = 0;
        ct[1] = 0;
        return;
    }
    delay_ms(75);
    
    // 等待忙状态结束
    while ((ATH20_Read_Status() & 0x80) == 0x80) {
        delay_ms(1);
        if (cnt++ >= 100) {
            printf("[AHT20] 等待忙状态超时\n");
            ct[0] = 0;
            ct[1] = 0;
            return;
        }
    }
    
    // 读取数据
    if (i2c_read_register(ATH20_SLAVE_ADDRESS, 0x00, data, 7) == 0) {
        // 检查数据有效性
        if ((data[0] & 0x80) == 0) {
            printf("[AHT20] 数据无效，状态: 0x%02X\n", data[0]);
            ct[0] = 0;
            ct[1] = 0;
            return;
        }
        
        // 解析湿度数据 (20位) - 使用原始demo.cpp的方法
        uint32_t humidity = 0;
        humidity = (humidity | data[1]) << 8;
        humidity = (humidity | data[2]) << 8;
        humidity = (humidity | data[3]);
        humidity = humidity >> 4;  // 右移4位
        ct[0] = humidity;
        
        // 解析温度数据 (20位) - 使用原始demo.cpp的方法
        uint32_t temperature = 0;
        temperature = (temperature | data[3]) << 8;
        temperature = (temperature | data[4]) << 8;
        temperature = (temperature | data[5]);
        temperature = temperature & 0xfffff;  // 使用掩码
        ct[1] = temperature;
        
        printf("[AHT20] 原始数据: H=0x%05lX, T=0x%05lX\n", humidity, temperature);
    } else {
        printf("[AHT20] 读取数据失败\n");
        ct[0] = 0;
        ct[1] = 0;
    }
}

// AHT20初始化
uint8_t ATH20_Init() {
    uint8_t tmp[2] = {0x08, 0x00};
    uint8_t count = 0;
    
    delay_ms(40);
    
    // 发送初始化命令
    i2c_write_register(ATH20_SLAVE_ADDRESS, ATH20_INIT_CMD, tmp, 2);
    delay_ms(500);
    
    // 等待校准完成
    while (ATH20_Read_Cal_Enable() == 0) {
        i2c_write_register(ATH20_SLAVE_ADDRESS, ATH20_SOFT_RESET_CMD, NULL, 0);
        delay_ms(200);
        
        i2c_write_register(ATH20_SLAVE_ADDRESS, ATH20_INIT_CMD, tmp, 2);
        
        count++;
        if (count >= 10) return 0;
        delay_ms(500);
    }
    return 1;
}

// BMP280获取原始数据
void BMP280GetPressure() {
    uint8_t data[6];
    
    if (i2c_read_register(BMP280_SLAVE_ADDRESS, BMP280_PRESSURE_MSB_REG, data, 6) == 0) {
        bmp280RawPressure = (int32_t)((((uint32_t)(data[0])) << 12) | (((uint32_t)(data[1])) << 4) | ((uint32_t)data[2] >> 4));
        bmp280RawTemperature = (int32_t)((((uint32_t)(data[3])) << 12) | (((uint32_t)(data[4])) << 4) | ((uint32_t)data[5] >> 4));
    }
}

// BMP280温度补偿
int32_t BMP280CompensateT(int32_t adcT) {
    int32_t var1, var2, T;
    
    var1 = ((((adcT >> 3) - ((int32_t)bmp280Cal.dig_T1 << 1))) * ((int32_t)bmp280Cal.dig_T2)) >> 11;
    var2 = (((((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1)) * ((adcT >> 4) - ((int32_t)bmp280Cal.dig_T1))) >> 12) * ((int32_t)bmp280Cal.dig_T3)) >> 14;
    bmp280Cal.t_fine = var1 + var2;
    
    T = (bmp280Cal.t_fine * 5 + 128) >> 8;
    return T;
}

// BMP280压力补偿
uint32_t BMP280CompensateP(int32_t adcP) {
    int64_t var1, var2, p;
    var1 = ((int64_t)bmp280Cal.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280Cal.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280Cal.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280Cal.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280Cal.dig_P3) >> 8) + ((var1 * (int64_t)bmp280Cal.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280Cal.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adcP;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280Cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280Cal.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280Cal.dig_P7) << 4);
    return (uint32_t)p;
}

// 压力转海拔
float BMP280PressureToAltitude(float *pressure) {
    if (*pressure > 0) {
        return ((pow((1015.7f / *pressure), CONST_PF) - 1.0f) * (FIX_TEMP + 273.15f)) / 0.0065f;
    }
    return 0;
}

// 压力滤波
void pressureFilter(float *in, float *out) {
    static uint8_t i = 0;
    static float filter_buf[FILTER_NUM] = {0.0f};
    double filter_sum = 0.0;
    uint8_t cnt = 0;
    float deta;
    
    if (filter_buf[i] == 0.0f) {
        filter_buf[i] = *in;
        *out = *in;
        if (++i >= FILTER_NUM) i = 0;
    } else {
        if (i) deta = *in - filter_buf[i - 1];
        else deta = *in - filter_buf[FILTER_NUM - 1];
        
        if (fabs(deta) < FILTER_A) {
            filter_buf[i] = *in;
            if (++i >= FILTER_NUM) i = 0;
        }
        for (cnt = 0; cnt < FILTER_NUM; cnt++) {
            filter_sum += filter_buf[cnt];
        }
        *out = filter_sum / FILTER_NUM;
    }
}

// BMP280获取数据
void BMP280GetData(float *pressure, float *temperature, float *asl) {
    static float t;
    static float p;
    
    BMP280GetPressure();
    t = BMP280CompensateT(bmp280RawTemperature) / 100.0f;
    p = BMP280CompensateP(bmp280RawPressure) / 25600.0f;
    pressureFilter(&p, pressure);
    *temperature = t;
    *asl = BMP280PressureToAltitude(pressure);
}

// BMP280初始化
uint8_t BMP280_Init() {
    uint8_t bmp280_id;
    uint8_t tmp[1];
    
    // 读取芯片ID
    if (i2c_read_register(BMP280_SLAVE_ADDRESS, BMP280_CHIPID_REG, &bmp280_id, 1) != 0) {
        return 0;
    }
    
    // 读取校准参数
    if (i2c_read_register(BMP280_SLAVE_ADDRESS, BMP280_DIG_T1_LSB_REG, (uint8_t *)&bmp280Cal, 24) != 0) {
        return 0;
    }
    
    // 设置工作模式
    tmp[0] = BMP280_MODE;
    i2c_write_register(BMP280_SLAVE_ADDRESS, BMP280_CTRLMEAS_REG, tmp, 1);
    
    // 设置IIR滤波器
    tmp[0] = (5 << 2);
    i2c_write_register(BMP280_SLAVE_ADDRESS, BMP280_CONFIG_REG, tmp, 1);
    
    return bmp280_id;
}

// 初始化硬件
bool initialize_hardware() {
    printf("[HARDWARE] 开始初始化硬件...\n");
    
    // 初始化I2C
    printf("[HARDWARE] 初始化I2C...\n");
    i2c_init_hardware();
    
    // 检测I2C设备
    printf("[HARDWARE] 检测I2C设备...\n");
    if (i2c_detect_device(ATH20_SLAVE_ADDRESS)) {
        printf("[HARDWARE] AHT20传感器检测到 (地址: 0x%02X)\n", ATH20_SLAVE_ADDRESS);
    } else {
        printf("[HARDWARE] AHT20传感器未检测到 (地址: 0x%02X)\n", ATH20_SLAVE_ADDRESS);
    }
    
    if (i2c_detect_device(BMP280_SLAVE_ADDRESS)) {
        printf("[HARDWARE] BMP280传感器检测到 (地址: 0x%02X)\n", BMP280_SLAVE_ADDRESS);
    } else {
        printf("[HARDWARE] BMP280传感器未检测到 (地址: 0x%02X)\n", BMP280_SLAVE_ADDRESS);
    }
    
    // 初始化ILI9488显示屏
    printf("[HARDWARE] 初始化ILI9488显示屏...\n");
    g_lcd_driver = new ili9488::ILI9488Driver(ILI9488_GET_SPI_CONFIG());
    if (!g_lcd_driver->initialize()) {
        printf("[HARDWARE] LCD初始化失败\n");
        return false;
    }
    
    // 设置显示屏参数
    g_lcd_driver->setBacklightBrightness(204);  // 80%亮度 (255 * 0.8 = 204)
    g_lcd_driver->setDisplayMode(ili9488::DisplayMode::Night);
    printf("[HARDWARE] ILI9488显示屏初始化完成\n");
    
    // 初始化环境监测显示模块
    printf("[HARDWARE] 初始化环境监测显示模块...\n");
    g_env_monitor = new environmental_monitor::EnvironmentalMonitor(g_lcd_driver);
    g_env_monitor->initialize_display();
    printf("[HARDWARE] 环境监测显示模块初始化完成\n");
    
    // 初始化AHT20传感器
    printf("[HARDWARE] 初始化AHT20传感器...\n");
    uint8_t ret = ATH20_Init();
    if (ret == 0) {
        printf("[HARDWARE] AHT20传感器初始化失败\n");
        g_env_monitor->show_error("AHT20初始化失败");
        return false;
    }
    printf("[HARDWARE] AHT20传感器初始化完成\n");
    
    // 初始化BMP280传感器
    printf("[HARDWARE] 初始化BMP280传感器...\n");
    ret = BMP280_Init();
    if (ret != 0x58) {
        printf("[HARDWARE] BMP280传感器初始化失败\n");
        g_env_monitor->show_error("BMP280初始化失败");
        return false;
    }
    printf("[HARDWARE] BMP280传感器初始化完成\n");
    
    printf("[HARDWARE] 所有硬件初始化完成\n");
    return true;
}

int main() {
    // 初始化串口
    stdio_init_all();
    
    // 等待串口稳定
    delay_ms(2000);
    
    printf("=== 环境监测器 - ILI9488显示版本 ===\n");
    printf("版本: v1.0.0\n");
    printf("显示屏: ILI9488 3.5寸 (320x480)\n");
    printf("传感器: AHT20 + BMP280\n");
    printf("====================================\n");
    
    // 初始化硬件
    if (!initialize_hardware()) {
        printf("[FATAL ERROR] 硬件初始化失败\n");
        while (1) {
            sleep_ms(1000);
        }
    }
    
    printf("[MAIN] 开始主循环...\n");
    
    // 主循环
    while (1) {
        environmental_monitor::SensorData sensor_data;
        
        // 读取AHT20温湿度数据
        uint32_t CT_data[2];
        uint8_t cal_retry = 0;
        while (ATH20_Read_Cal_Enable() == 0 && cal_retry < 3) {
            printf("[AHT20] 校准状态检查失败，重试 %d/3\n", cal_retry + 1);
            ATH20_Init();
            delay_ms(100);
            cal_retry++;
        }
        
        if (cal_retry >= 3) {
            printf("[AHT20] 校准失败，使用默认值\n");
            CT_data[0] = 0;
            CT_data[1] = 0;
        } else {
            ATH20_Read_CTdata(CT_data);
        }
        
        // 读取BMP280数据（先读取，因为温度卡片需要BMP280温度）
        float P, T, ALT;
        BMP280GetData(&P, &T, &ALT);
        sensor_data.bmp280_pressure = P;
        sensor_data.bmp280_temperature = T;
        sensor_data.bmp280_altitude = ALT;
        
        // 转换AHT20数据 - 使用原始demo.cpp的方法
        if (CT_data[0] > 0 && CT_data[1] > 0) {
            // 使用原始demo.cpp的转换公式
            sensor_data.aht20_humidity = CT_data[0] * 1000.0f / 1024.0f / 1024.0f;  // 湿度值
            sensor_data.aht20_temperature = CT_data[1] * 200.0f * 10.0f / 1024.0f / 1024.0f - 50.0f;  // 温度值
            printf("[AHT20] 转换后: 湿度=%.1f%%, 温度=%.1f°C\n", 
                   sensor_data.aht20_humidity, sensor_data.aht20_temperature);
        } else {
            // 使用BMP280温度作为AHT20温度的替代值，湿度设为50%
            sensor_data.aht20_humidity = 50.0f;
            sensor_data.aht20_temperature = sensor_data.bmp280_temperature;
            printf("[AHT20] 数据无效，使用BMP280温度作为替代值\n");
        }
        
        printf("[BMP280] 压力=%.4fhPa, 温度=%.1f°C, 海拔=%.1fm\n", P, T, ALT);
        
        // 更新显示
        g_env_monitor->update_sensor_data(sensor_data);
        
        // 打印调试信息
        printf("[DATA] AHT20: %.1f°C, %.1f%% | BMP280: %.1f°C, %.4fhPa, %.1fm\n",
               sensor_data.aht20_temperature, sensor_data.aht20_humidity,
               sensor_data.bmp280_temperature, sensor_data.bmp280_pressure, sensor_data.bmp280_altitude);
        
        // 延时1秒
        delay_ms(1000);
    }
    
    return 0;
}
