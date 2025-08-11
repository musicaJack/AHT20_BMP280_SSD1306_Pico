#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

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
    i2c_write_register(ATH20_SLAVE_ADDRESS, ATH20_START_TEST_CMD, tmp, 2);
    delay_ms(75);
    
    // 等待忙状态结束
    while ((ATH20_Read_Status() & 0x80) == 0x80) {
        delay_ms(1);
        if (cnt++ >= 100) break;
    }
    
    // 读取数据
    if (i2c_read_register(ATH20_SLAVE_ADDRESS, 0x00, data, 7) == 0) {
        // 解析湿度数据
        uint32_t humidity = 0;
        humidity = (humidity | data[1]) << 8;
        humidity = (humidity | data[2]) << 8;
        humidity = (humidity | data[3]);
        humidity = humidity >> 4;
        ct[0] = humidity;
        
        // 解析温度数据
        uint32_t temperature = 0;
        temperature = (temperature | data[3]) << 8;
        temperature = (temperature | data[4]) << 8;
        temperature = (temperature | data[5]);
        temperature = temperature & 0xfffff;
        ct[1] = temperature;
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

// 全局变量用于存储上一次的数据
static int last_aht20_temp = -999;
static int last_aht20_humi = -999;
static float last_bmp280_pressure = -999.0f;
static float last_bmp280_temp = -999.0f;
static float last_bmp280_alt = -999.0f;
static float last_avg_temp = -999.0f;
static bool first_run = true;

// 清屏函数
void clear_screen() {
    printf("\033[2J\033[H");  // ANSI转义序列：清屏并移动光标到左上角
}

// 移动光标到指定位置
void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

// 打印表格头部
void print_table_header() {
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                RP2040 AHT20 + BMP280 传感器数据              │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ 传感器  │ 参数   │ 数值     │ 单位 │ 状态                    │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
}

// 打印表格底部
void print_table_footer() {
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ 更新时间: %s │\n", "实时更新");
    printf("└─────────────────────────────────────────────────────────────┘\n");
}

// 打印一行数据
void print_data_row(const char* sensor, const char* param, const char* value, const char* unit, const char* status) {
    printf("│ %-7s │ %-6s │ %-9s │ %-4s │ %-22s │\n", sensor, param, value, unit, status);
}

// 格式化数值为字符串
void format_value(char* buffer, float value, int precision) {
    if (precision == 0) {
        sprintf(buffer, "%d", (int)value);
    } else if (precision == 1) {
        sprintf(buffer, "%d.%d", (int)value, (int)(value * 10) % 10);
    } else {
        sprintf(buffer, "%.*f", precision, value);
    }
}

// 检查数值是否发生变化
bool value_changed(float current, float last, float threshold) {
    return fabs(current - last) >= threshold;
}

int main() {
    // 初始化串口
    stdio_init_all();
    
    // 初始化I2C
    i2c_init_hardware();
    
    // 等待串口稳定
    delay_ms(2000);
    
    // 初始化AHT20
    uint8_t ret = ATH20_Init();
    if (ret == 0) {
        printf("AHT20传感器初始化失败！\n");
        while (1) {
            sleep_ms(1000);
        }
    }
    
    // 初始化BMP280
    ret = BMP280_Init();
    if (ret != 0x58) {
        printf("BMP280传感器初始化失败！\n");
        while (1) {
            sleep_ms(1000);
        }
    }
    
    // 清屏并打印初始表格
    clear_screen();
    
    // 添加一些空行，让表格下移
    printf("\n\n");
    
    print_table_header();
    
    // 打印初始数据行
    print_data_row("AHT20", "温度", "---", "℃", "初始化中...");
    print_data_row("AHT20", "湿度", "---", "%", "初始化中...");
    print_data_row("BMP280", "气压", "---", "hPa", "初始化中...");
    print_data_row("BMP280", "温度", "---", "℃", "初始化中...");
    print_data_row("平均", "温度", "---", "℃", "初始化中...");
    print_data_row("BMP280", "海拔", "---", "m", "初始化中...");
    
    print_table_footer();
    
    // 主循环
    while (1) {
        uint32_t CT_data[2];
        int c1, t1;
        float P, T, ALT;
        char value_buffer[16];
        bool data_updated = false;
        
        // 读取AHT20温湿度数据
        while (ATH20_Read_Cal_Enable() == 0) {
            ATH20_Init();
            delay_ms(30);
        }
        ATH20_Read_CTdata(CT_data);
        c1 = CT_data[0] * 1000 / 1024 / 1024;  // 湿度值放大10倍
        t1 = CT_data[1] * 200 * 10 / 1024 / 1024 - 500;  // 温度值放大10倍
        
        // 读取BMP280数据
        BMP280GetData(&P, &T, &ALT);
        
        // 检查AHT20温度是否变化
        if (first_run || value_changed(t1, last_aht20_temp, 1)) {
            format_value(value_buffer, t1 / 10.0f, 1);
            move_cursor(6, 1);  // 移动到AHT20温度行（加2行空行）
            print_data_row("AHT20", "温度", value_buffer, "℃", "正常");
            last_aht20_temp = t1;
            data_updated = true;
        }
        
        // 检查AHT20湿度是否变化
        if (first_run || value_changed(c1, last_aht20_humi, 1)) {
            format_value(value_buffer, c1 / 10.0f, 1);
            move_cursor(7, 1);  // 移动到AHT20湿度行（加2行空行）
            print_data_row("AHT20", "湿度", value_buffer, "%", "正常");
            last_aht20_humi = c1;
            data_updated = true;
        }
        
        // 检查BMP280气压是否变化
        if (first_run || value_changed(P, last_bmp280_pressure, 0.01f)) {
            format_value(value_buffer, P, 4);
            move_cursor(8, 1);  // 移动到BMP280气压行（加2行空行）
            print_data_row("BMP280", "气压", value_buffer, "hPa", "正常");
            last_bmp280_pressure = P;
            data_updated = true;
        }
        
        // 检查BMP280温度是否变化
        if (first_run || value_changed(T, last_bmp280_temp, 0.01f)) {
            format_value(value_buffer, T, 2);
            move_cursor(9, 1);  // 移动到BMP280温度行（加2行空行）
            print_data_row("BMP280", "温度", value_buffer, "℃", "正常");
            last_bmp280_temp = T;
            data_updated = true;
        }
        
        // 计算并检查平均温度是否变化
        float aht20_temp_float = t1 / 10.0f;
        float avg_temp = (aht20_temp_float + T) / 2.0f;
        if (first_run || value_changed(avg_temp, last_avg_temp, 0.01f)) {
            format_value(value_buffer, avg_temp, 2);
            move_cursor(10, 1);  // 移动到平均温度行（加2行空行）
            print_data_row("平均", "温度", value_buffer, "℃", "正常");
            last_avg_temp = avg_temp;
            data_updated = true;
        }
        
        // 检查BMP280海拔是否变化
        if (first_run || value_changed(ALT, last_bmp280_alt, 0.01f)) {
            format_value(value_buffer, ALT / 10.0f, 2);
            move_cursor(11, 1);  // 移动到BMP280海拔行（加2行空行）
            print_data_row("BMP280", "海拔", value_buffer, "m", "正常");
            last_bmp280_alt = ALT;
            data_updated = true;
        }
        
        // 更新表格底部时间
        if (data_updated) {
            move_cursor(13, 1);  // 移动到表格底部（加2行空行）
            printf("├─────────────────────────────────────────────────────────────┤\n");
            printf("│ 更新时间: %s │\n", "实时更新");
            printf("└─────────────────────────────────────────────────────────────┘\n");
        }
        
        first_run = false;
        
        // 延时1秒
        delay_ms(1000);
    }
    
    return 0;
}
