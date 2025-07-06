#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>  // for timer_* functions
#include <libopencm3/cm3/nvic.h>     // for nvic_enable_irq
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/spi.h>

volatile uint32_t system_millis = 0;



void timer_setup(void){
    
    rcc_periph_clock_enable(RCC_TIM2);
    timer_disable_counter(TIM2);
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM2, 7199);    //10kHz tick
    timer_set_period(TIM2, 999);         //
    timer_clear_flag(TIM2, TIM_SR_UIF);
    timer_enable_irq(TIM2, TIM_DIER_UIE);
    nvic_enable_irq(NVIC_TIM2_IRQ);
    timer_enable_counter(TIM2);

}


void tim2_isr(void) __attribute__((interrupt("IRQ")));
void tim2_isr(void){
    if (timer_get_flag(TIM2, TIM_SR_UIF)) {
        timer_clear_flag(TIM2, TIM_SR_UIF);
        gpio_toggle(GPIOC, GPIO13);
    }
}

void sys_tick_handler(void) __attribute__((interrupt("IRQ")));
void sys_tick_handler(void) {
    system_millis++;
}

void msleep(uint32_t delay) {
    uint32_t wake = system_millis + delay;
    while (system_millis < wake);
}

void systick_setup(void) {
    systick_set_reload(72000 - 1); // 72 MHz / 1000 = 1ms tick
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint16_t read_adc(void){
    adc_set_regular_sequence(ADC1, 1, (uint8_t[]){0});
    adc_start_conversion_direct(ADC1);
    while( !adc_eoc(ADC1));
    return adc_read_regular(ADC1);

}

uint8_t spiRead(void){
    while(!(SPI_SR(SPI1) & SPI_SR_RXNE));
    return SPI_DR(SPI1);
}

void spiWrite(uint8_t data){
    while(!(SPI_SR(SPI1) & SPI_SR_TXE));
    SPI_DR(SPI1) = data;
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO14);

    gpio_set_mode(GPIOA, GPIO_SPI1_NSS, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4);         //CS
    gpio_set_mode(GPIOA, GPIO_SPI1_SCK, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5);         //SPC
    gpio_set_mode(GPIOA, GPIO_SPI1_MISO, GPIO_CNF_INPUT_FLOAT, GPIO6);                  //SDO
    gpio_set_mode(GPIOA, GPIO_SPI1_MOSI, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);        //SDI

    cm_enable_interrupts();
    timer_setup();
    systick_setup();
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_16, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, 
        SPI_CR1_CPHA_CLK_TRANSITION_1, 1, 0);
    spi_enable(SPI1);

    uint16_t data;
    uint8_t imuRegAddr = 1;
    while(1){
        imuRegAddr++;
        imuRegAddr %= 0x6B;

        gpio_clear(GPIOA, GPIO4);
        spiWrite(((1<<7) | imuRegAddr));
        uint16_t test = 1<<7;
        test = test | imuRegAddr;
        data = spiRead();
        gpio_set(GPIOA, GPIO4);
        if (data != 0){
            gpio_set(GPIOC, GPIO14);
        }
        else{
            gpio_clear(GPIOC, GPIO14);
        }
    
    }
}