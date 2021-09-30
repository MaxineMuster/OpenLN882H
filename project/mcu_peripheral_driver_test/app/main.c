/**
 * @file     main.c
 * @author   BSP Team 
 * @brief    
 * @version  0.0.0.1
 * @date     2021-08-05
 * 
 * @copyright Copyright (c) 2021 Shanghai Lightning Semiconductor Technology Co. Ltd
 * 
 */

#include "hal/hal_common.h"
#include "hal/hal_gpio.h"
#include "ln_test_common.h"
#include "ln_show_reg.h"
#include "log.h"


#include "hal_adc.h"
#include "hal_timer.h"
#include "hal_clock.h"
#include "hal_adv_timer.h"

static unsigned char tx_data[100];              //����BUFFER
static unsigned char rx_data[100];
static unsigned char sensor_data[6];
static unsigned int  err_cnt = 0;

static volatile double        tem_data = 0;     //�¶�����
static volatile double        hum_data = 0;     //ʪ������


static unsigned int address[10];
static unsigned int address_num = 0;

static unsigned int light_value = 0;

static unsigned char status[2];


int main (int argc, char* argv[])
{  
    /* 1. ϵͳ��ʼ�� */
    SetSysClock();
    
    log_init();
    
    /* 2. I2C���� */
    
    ln_i2c_init();              
    
    /* 2.1 ��ʼɨ��I2C��ַ */
    LOG(LOG_LVL_INFO,"ln8620 i2c scan start! \n");
    
    hal_i2c_address_scan(I2C_BASE);
    
    if(address_num == 0)
    {
        LOG(LOG_LVL_INFO,"There is no device on I2C bus! \n");
    }
    else
    {
        for(int i = address_num; i > 0; i--)
        {
            LOG(LOG_LVL_INFO,"Found device %d,address : 0x%x! \n",address_num - i,address[address_num - i]);
        }
        LOG(LOG_LVL_INFO,"Scan end. \n");
    }
    
    for(int i = 0; i < 100; i++)
    {
        tx_data[i] = i;
    }
    
    /* 2.2 ��ʼ��дAT24C04 */
    LOG(LOG_LVL_INFO,"ln8620 i2c + 24c04 test start! \n");
    
    hal_24c04_write(256,tx_data,100);
    hal_24c04_read(256,rx_data,100);
    
    for(int i = 0; i < 100; i++)
    {   
        if(tx_data[i] != rx_data[i])
        {
            err_cnt++;
        }
    }
    if(err_cnt != 0)
        LOG(LOG_LVL_INFO,"ln8620 i2c + 24c04 test fail! \n");
    else
        LOG(LOG_LVL_INFO,"ln8620 i2c + 24c04 test success! \n");
    
    LOG(LOG_LVL_INFO,"ln8620 i2c + oled test start! \n");
    
    /* 2.3 ��ʼ��OLED��ʾ�� */
    gpio_init_t gpio_init;
    memset(&gpio_init,0,sizeof(gpio_init));        //����ṹ��
    gpio_init.dir = GPIO_OUTPUT;                   //����GPIO��������������
    gpio_init.pin = GPIO_PIN_4;                    //����GPIO���ź�
    gpio_init.speed = GPIO_HIGH_SPEED;             //����GPIO�ٶ�
    hal_gpio_init(GPIOB_BASE,&gpio_init);          //��ʼ��GPIO
    hal_gpio_pin_reset(GPIOB_BASE,GPIO_PIN_4);     //RESET OLED��Ļ
    ln_delay_ms(100);
    hal_gpio_pin_set(GPIOB_BASE,GPIO_PIN_4);
    
    ln_oled_init(I2C_BASE);
    ln_oled_display_on();

    ln_oled_show_string(0,40,(unsigned char *)"Test success!",12);      //��ʾ���Գɹ���Ӣ��
    LOG(LOG_LVL_INFO,"ln8620 i2c + oled test success! \n");
    
    ln_oled_refresh();                                                  //ˢ����ʾ
    ln_delay_ms(1000);
    ln_oled_clear();
    
    /* 2.4 ��ʼ����ʪ�ȴ����� */
    LOG(LOG_LVL_INFO,"ln8620 i2c + sht30 test start! \n");
    ln_tem_hum_init();
    
    /* 3. ADC���� ���ɼ���������ֵ��*/
    
    adc_init_t_def adc_init;
    
    memset(&adc_init,0,sizeof(adc_init));
    
    adc_init.adc_ch = ADC_CH5;                              //����ͨ��
    adc_init.adc_conv_mode = ADC_CONV_MODE_CONTINUE;        //����ADCΪ����ת��ģʽ
    adc_init.adc_presc = 40;                             
    hal_adc_init(ADC_BASE,&adc_init);                       //��ʼ��ADC
    
    hal_gpio_pin_mode_set(GPIOB_BASE,GPIO_PIN_3,GPIO_MODE_ANALOG);      //����PB3Ϊģ������
    hal_adc_it_cfg(ADC_BASE,ADC_IT_FLAG_EOC_5,HAL_ENABLE);            
    
    hal_adc_en(ADC_BASE,HAL_ENABLE);
    hal_adc_start_conv(ADC_BASE);
    

    /* 4. SPI Flash���� */
    ln_spi_master_init();
    
    //��ʼ������buffer
    for(int i = 0; i < 100 ; i++)
    {
        tx_data[i] = i + 2;
    }

    LOG(LOG_LVL_INFO,"ln8620 SPI + 25WQ16 test start! \n");

    /********************************SPIֱ�Ӷ�дFlashоƬ****************************************************************/

    //��ȡID
    hal_25wq16_read_id(SPI0_BASE,rx_data);
    LOG(LOG_LVL_INFO,"ln8620 25WQ16 ID: %x %x %x \n",rx_data[0],rx_data[1],rx_data[2]);
    
    //��ȡ״̬
    hal_25wq16_read_status(SPI0_BASE,rx_data);

    //дʹ��
    hal_25wq16_write_enable(SPI0_BASE);

    //��ȡ״̬
    hal_25wq16_read_status(SPI0_BASE,rx_data);

    //����оƬ
    hal_25wq16_erase_flash(SPI0_BASE,0x200);

    //�ȴ��������
    while(1)
    {
        hal_25wq16_read_status(SPI0_BASE,status);
        if((status[0] & 0x01) == 0x00)
            break;
    }

    //��FlashоƬ��д������
    hal_25wq16_write_flash(SPI0_BASE,0x200,tx_data,100);

    //�ȴ�д�����
    while(1)
    {
        hal_25wq16_read_status(SPI0_BASE,status);
        if((status[0] & 0x01) == 0x00)
            break;
    }

    //��FlashоƬ�ж�������
    hal_25wq16_read_flash(SPI0_BASE,0x200,rx_data,100);
    
    //�ж�д��������Ƿ���ȷ
    err_cnt = 0;
    for(int i = 0 ; i < 100; i++)
    {
        if(rx_data[i] != tx_data[i])
        {
            err_cnt++;
        }
    }

    //��ӡLOG
    if(err_cnt != 0)
    {
        LOG(LOG_LVL_INFO,"ln8620 SPI + 25WQ16 test fail! \n");
    }
    else
    {
        LOG(LOG_LVL_INFO,"ln8620 SPI + 25WQ16 test success! \n");
    }
    
    /* 5. PWM���� */
    __NVIC_SetPriorityGrouping(4);
    __enable_irq();

    /* pwm ���ų�ʼ�� */
    hal_gpio_afio_select(GPIOA_BASE,GPIO_PIN_8,ADV_TIMER_PWM0);
    hal_gpio_afio_select(GPIOA_BASE,GPIO_PIN_9,ADV_TIMER_PWM1);
    hal_gpio_afio_en(GPIOA_BASE,GPIO_PIN_8,HAL_ENABLE);
    hal_gpio_afio_en(GPIOA_BASE,GPIO_PIN_9,HAL_ENABLE);  

    /* PWM������ʼ�� */
    adv_tim_init_t_def adv_tim_init;
    memset(&adv_tim_init,0,sizeof(adv_tim_init));
    adv_tim_init.adv_tim_clk_div = 0;                               //����ʱ�ӷ�Ƶ��0Ϊ����Ƶ
    adv_tim_init.adv_tim_load_value =  40000 - 1;                   //����PWMƵ�ʣ�40000 * 1 / PCLK(80M) / DIV(0) = 2k
    adv_tim_init.adv_tim_cmp_a_value = 40000 ;                      //����ͨ��a�Ƚ�ֵ��ռ�ձ�Ϊ 50%
    adv_tim_init.adv_tim_cmp_b_value = 40000 ;                      //����ͨ��b�Ƚ�ֵ��ռ�ձ�Ϊ 50%
    adv_tim_init.adv_tim_dead_gap_value = 1000;                     //��������ʱ��
    adv_tim_init.adv_tim_dead_en = ADV_TIMER_DEAD_DIS;              //����������
    adv_tim_init.adv_tim_cnt_mode = ADV_TIMER_CNT_MODE_INC;         //���ϼ���ģʽ
    adv_tim_init.adv_tim_cha_en = ADV_TIMER_CHA_EN;                 //ʹ��ͨ��a
    adv_tim_init.adv_tim_chb_en = ADV_TIMER_CHB_EN;                 //ʹ��ͨ��b
    adv_tim_init.adv_tim_cha_it_mode = ADV_TIMER_CHA_IT_MODE_INC;   //ʹ��ͨ��a���ϼ����ж�
    adv_tim_init.adv_tim_chb_it_mode = ADV_TIMER_CHB_IT_MODE_INC;   //ʹ��ͨ��b���ϼ����ж�
    hal_adv_tim_init(ADV_TIMER_0_BASE,&adv_tim_init);               //��ʼ��ADV_TIMER0

    uint32_t cmp_a_value = 0;       //ͨ��a�ıȽ�ֵ
    float    duty_value  = 0;       //ռ�ձ�
    uint8_t  inc_dir     = 0;       //ռ�ձȵ���/������
    uint32_t pulse_cnt   = 0;       //�������
    
    while(1)
    {   
        unsigned char display_buffer[16];
        
        if(tem_hum_read_sensor_data(sensor_data) == 0)                     //�����ʪ����Ϣ
        {
            
            memset(display_buffer,' ',16);
            tem_data = -45 + (175 * (sensor_data[0] * 256 + sensor_data[1]) * 1.0 / (65535 - 1)) ;      //ת��Ϊ���϶�
            hum_data = 100 * ((sensor_data[3] * 256 + sensor_data[4]) * 1.0 / (65535 - 1)) ;            //ת��Ϊ%

            sprintf((char *)display_buffer,"TEM : %0.2f",tem_data);        //oled��ʾ�ʹ��ڴ�ӡ����
            ln_oled_show_string_with_len(0,32,display_buffer,16,15);      
            memset(display_buffer,' ',16);
            sprintf((char *)display_buffer,"HUM : %0.2f",hum_data);
            ln_oled_show_string_with_len(0,48,display_buffer,16,15);      
        }

        //��ʾ�ʹ�ӡLOG��
        LOG(LOG_LVL_INFO,"Tem = %0.2f  ",tem_data);
        LOG(LOG_LVL_INFO,"Hum = %0.2f%\n",hum_data);
        
        
        
        
        if(hal_adc_get_it_flag(ADC_BASE,ADC_IT_FLAG_EOC_5))
        {
            light_value = hal_adc_get_data(ADC_BASE,ADC_CH5);     
            hal_adc_clr_it_flag(ADC_BASE,ADC_IT_FLAG_EOC_5);
            
            memset(display_buffer,' ',16);
            sprintf((char *)display_buffer,"LIGHT : %d",light_value);
            ln_oled_show_string_with_len(0,0,display_buffer,16,15);  
            LOG(LOG_LVL_INFO,"LIGHT : %d \r\n\r\n",light_value);
        }
        
        //�˴������ǹ��������LED�����������ն�Խ�ͣ��Ƶ�����Խ�ߡ�
        //200�ǹ�����ǿʱ��ɼ���ADֵ��2000����ȫ�ڵ���������ɼ���ֵ
        if(light_value <= 300)
            duty_value = 0.01;
        else if(light_value >= 2000)
            duty_value = 0.99;
        else
            duty_value = (((light_value - 300) * 1.0f) / (2000 - 300));
        hal_adv_tim_set_comp_a(ADV_TIMER_0_BASE, (duty_value) * 40000);
        hal_adv_tim_set_comp_b(ADV_TIMER_0_BASE, (duty_value) * 40000);
       
        
        memset(display_buffer,' ',16);
        sprintf((char *)display_buffer,"PWM : %0.2f",((duty_value)*100));
        ln_oled_show_string_with_len(0,16,display_buffer,16,15);  
        
        
        
        
        ln_oled_refresh();                                                  //ˢ��OLED��ʾ
        
        ln_delay_ms(30);
    }
    
}


