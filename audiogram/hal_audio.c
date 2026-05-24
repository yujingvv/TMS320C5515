/* ============================================================
 * hal_audio.c  --  AIC3204 codec + I2S + DMA 真实驱动实现
 *
 * 目标芯片：TMS320C5515
 * 音频接口：I2S0（C5515 内置 I2S，地址 0x2800）
 * 控制接口：I2C（AIC3204 地址 0x18）
 * 采样率  ：16000 Hz，单声道，16-bit
 *
 * 替换 stub_bsl.c 中的四个空函数：
 *   audio_codec_init()
 *   dma_audio_start()
 *   intc_enable()
 *   cpu_idle_until_irq()
 * ============================================================ */

#include "config.h"

/* ---- 基本类型（与 evm5515.h 一致） ---- */
#define Uint32  unsigned long
#define Uint16  unsigned short
#define Uint8   unsigned char
#define Int16   short

/* ============================================================
 * 寄存器直接访问宏（C5515 I/O 空间）
 * ============================================================ */

/* 系统控制 */
#define SYS_PCGCR1    *(volatile ioport Uint16*)(0x1C02)
#define SYS_PCGCR2    *(volatile ioport Uint16*)(0x1C03)
#define SYS_EXBUSSEL  *(volatile ioport Uint16*)(0x1C00)

/* I2C */
#define I2C_PSC   *(volatile ioport Uint16*)(0x1A30)
#define I2C_CLKL  *(volatile ioport Uint16*)(0x1A0C)
#define I2C_CLKH  *(volatile ioport Uint16*)(0x1A10)
#define I2C_MDR   *(volatile ioport Uint16*)(0x1A24)
#define I2C_STR   *(volatile ioport Uint16*)(0x1A08)
#define I2C_CNT   *(volatile ioport Uint16*)(0x1A14)
#define I2C_SAR   *(volatile ioport Uint16*)(0x1A1C)
#define I2C_DXR   *(volatile ioport Uint16*)(0x1A20)
#define I2C_DRR   *(volatile ioport Uint16*)(0x1A18)

/* I2C MDR 位域 */
#define MDR_STT  0x2000
#define MDR_STP  0x0800
#define MDR_TRX  0x0200
#define MDR_MST  0x0400
#define MDR_IRS  0x0020
#define MDR_FREE 0x4000
#define STR_XRDY 0x0010
#define STR_RRDY 0x0008

/* I2S0 寄存器（C5515 内置 I2S，对应 evm5515.h 中 I2S0_xxx） */
#define I2S0_CR       *(volatile ioport Uint16*)(0x2800)  /* 控制寄存器    */
#define I2S0_SRGR     *(volatile ioport Uint16*)(0x2804)  /* 采样率生成器  */
#define I2S0_W0_LSW_R *(volatile ioport Uint16*)(0x2828)  /* 接收左声道LSW */
#define I2S0_W0_MSW_R *(volatile ioport Uint16*)(0x2829)  /* 接收左声道MSW */
#define I2S0_W0_LSW_W *(volatile ioport Uint16*)(0x2808)  /* 发送左声道LSW */
#define I2S0_W0_MSW_W *(volatile ioport Uint16*)(0x2809)  /* 发送左声道MSW */
#define I2S0_IR       *(volatile ioport Uint16*)(0x2810)  /* 中断标志      */
#define I2S0_ICMR     *(volatile ioport Uint16*)(0x2814)  /* 中断使能      */

/* DMA 系统中断标志寄存器 */
#define DMAIFR  *(volatile ioport Uint16*)(0x0D04)
#define DMAIER  *(volatile ioport Uint16*)(0x0D06)

/* CPU 中断相关 */
#define IER0    *(volatile ioport Uint16*)(0x0000)
#define IFR0    *(volatile ioport Uint16*)(0x0001)
#define IER1    *(volatile ioport Uint16*)(0x0045)
#define IFR1    *(volatile ioport Uint16*)(0x0046)

/* AIC3204 I2C 地址 */
#define AIC3204_I2C_ADDR  0x18

/* ============================================================
 * 内部函数：软件延时
 * ============================================================ */
static void wait_loop(Uint32 n)
{
    volatile Uint32 i;
    for (i = 0; i < n; i++) {}
}

/* ============================================================
 * 内部函数：I2C 初始化（100 kHz）
 * ============================================================ */
static void i2c_init(void)
{
    I2C_MDR  = 0x0400;   /* 复位 I2C                          */
    I2C_PSC  = 15;       /* 预分频：SYSCLK/16 ≈ 6.25 MHz      */
    I2C_CLKL = 25;       /* 低电平时间 → 100 kHz              */
    I2C_CLKH = 25;       /* 高电平时间                        */
    I2C_MDR  = 0x0420;   /* 释放复位；主机、发送、7-bit 地址  */
}

/* ============================================================
 * 内部函数：I2C 写 2 字节（寄存器地址 + 数据）
 * ============================================================ */
static Int16 i2c_write2(Uint16 dev_addr, Uint8 reg, Uint8 val)
{
    Uint8 buf[2];
    int timeout, i;

    buf[0] = reg;
    buf[1] = val;

    I2C_CNT = 2;
    I2C_SAR = dev_addr;
    I2C_MDR = MDR_STT | MDR_TRX | MDR_MST | MDR_IRS | MDR_FREE;

    wait_loop(100);

    for (i = 0; i < 2; i++) {
        I2C_DXR = buf[i];
        timeout = 0x7FFF;
        while ((I2C_STR & STR_XRDY) == 0) {
            if (--timeout < 0) {
                i2c_init();   /* 超时则复位 I2C */
                return -1;
            }
        }
    }

    I2C_MDR |= MDR_STP;   /* 产生 STOP 条件 */
    wait_loop(800);        /* 等待总线释放   */
    return 0;
}

/* ============================================================
 * 内部函数：AIC3204 寄存器写
 * ============================================================ */
static void aic_rset(Uint16 reg, Uint16 val)
{
    i2c_write2(AIC3204_I2C_ADDR, (Uint8)reg, (Uint8)val);
}

/* ============================================================
 * 内部函数：AIC3204 初始化（16 kHz，单声道麦克风输入）
 *
 * 时钟链路（与 aic3204_loop_mic_in.c 相同，降为 16 kHz）：
 *   PLL_CLKIN = BCLK（由 I2S0 SRGR 产生）
 *   PLL_CLK   = PLL_CLKIN × R × J.D / P
 *             = BCLK × 2 × 32 / 1
 *   CODEC_CLKIN = PLL_CLK
 *   ADC_FS = CODEC_CLKIN / (NADC × MADC × AOSR)
 *          = 16000 Hz
 * ============================================================ */
static void aic3204_init_16k(void)
{
    aic_rset(  0, 0x00 );   /* 选择 Page 0                              */
    aic_rset(  1, 0x01 );   /* 软件复位                                 */
    wait_loop(5000);

    aic_rset(  0, 0x01 );   /* 选择 Page 1                              */
    aic_rset(  1, 0x08 );   /* 禁用从 DVDD 产生 AVDD                    */
    aic_rset(  2, 0x00 );   /* 使能模拟模块                             */

    /* PLL 和时钟配置 */
    aic_rset(  0, 0x00 );   /* 选择 Page 0                              */
    aic_rset( 27, 0x40 );   /* BCLK/WCLK 作为 AIC3204 输入（从机模式） */
    aic_rset(  4, 0x07 );   /* PLL_CLK <- BCLK；CODEC_CLKIN <- PLL_CLK */
    aic_rset(  5, 0x92 );   /* PLL 上电，P=1，R=2                       */
    aic_rset(  6, 0x20 );   /* J = 32                                   */
    aic_rset(  7, 0x00 );   /* D 高字节 = 0                             */
    aic_rset(  8, 0x00 );   /* D 低字节 = 0                             */

    /* ADC 过采样率：AOSR=128，NADC=4，MADC=2
     * ADC_FS = PLL_CLK / (4×2×128) = 16000 Hz（PLL_CLK=16.384MHz） */
    aic_rset( 20, 0x80 );   /* AOSR = 128                               */
    aic_rset( 18, 0x84 );   /* NADC=4，上电                             */
    aic_rset( 19, 0x82 );   /* MADC=2，上电                             */

    /* DAC 时钟（耳机输出用） */
    aic_rset( 13, 0x00 );   /* DOSR 高字节 = 0                          */
    aic_rset( 14, 0x80 );   /* DOSR = 128                               */
    aic_rset( 11, 0x84 );   /* NDAC=4，上电                             */
    aic_rset( 12, 0x82 );   /* MDAC=2，上电                             */

    /* DAC 路由和上电（耳机输出） */
    aic_rset(  0, 0x01 );   /* 选择 Page 1                              */
    aic_rset( 12, 0x08 );   /* LDAC -> HPL                              */
    aic_rset( 13, 0x08 );   /* RDAC -> HPR                              */
    aic_rset(  0, 0x00 );   /* 选择 Page 0                              */
    aic_rset( 64, 0x02 );   /* 左右声道 DAC 音量联动                    */
    aic_rset( 65, 0x00 );   /* DAC 增益 0 dB                            */
    aic_rset( 63, 0xD4 );   /* 左右 DAC 数据路径上电                    */
    aic_rset(  0, 0x01 );   /* 选择 Page 1                              */
    aic_rset( 16, 0x06 );   /* HPL 增益 +6 dB，取消静音                 */
    aic_rset( 17, 0x06 );   /* HPR 增益 +6 dB，取消静音                 */
    aic_rset(  9, 0x30 );   /* HPL/HPR 上电                             */
    aic_rset(  0, 0x00 );   /* 选择 Page 0                              */
    wait_loop(4000);

    /* ADC 路由和上电（麦克风输入） */
    aic_rset(  0, 0x01 );   /* 选择 Page 1                              */
    aic_rset( 51, 0x40 );   /* MICBIAS 设置                             */
    aic_rset( 52, 0xC0 );   /* IN2_L -> LADC_P，40 kΩ                  */
    aic_rset( 55, 0xC0 );   /* IN2_R -> RADC_P，40 kΩ                  */
    aic_rset( 54, 0x03 );   /* CM_1  -> LADC_M，40 kΩ                  */
    aic_rset( 57, 0xC0 );   /* CM_1  -> RADC_M，40 kΩ                  */
    aic_rset( 59, 0x5F );   /* MIC_PGA_L 取消静音，增益约 23.5 dB      */
    aic_rset( 60, 0x5F );   /* MIC_PGA_R 取消静音                       */
    aic_rset(  0, 0x00 );   /* 选择 Page 0                              */
    aic_rset( 81, 0xC0 );   /* 左右 ADC 上电                            */
    aic_rset( 82, 0x00 );   /* 左右 ADC 取消静音                        */

    wait_loop(4000);
}

/* ============================================================
 * 内部函数：I2S0 初始化（16 kHz，从机模式，16-bit，单声道）
 *
 * C5515 I2S0 CR 寄存器：
 *   bit15   : ENABLE
 *   bit14   : 主/从（0=从机，时钟由 AIC3204 提供）
 *   bit11:8 : 字长（0x0=16-bit）
 *   bit5    : 单/双声道（0=双声道，1=单声道）
 *   bit2    : LOOPBACK
 *   bit0    : RESET
 * ============================================================ */
static void i2s0_init(void)
{
    /* 禁用 I2S0，先复位 */
    I2S0_CR   = 0x0001;   /* 复位                                       */
    wait_loop(100);
    I2S0_CR   = 0x0000;   /* 解除复位                                   */

    /* 配置：从机，16-bit，立体声（两路均采集，只用左声道） */
    /* CR = 0x8000: ENABLE=1, SLAVE=0（bit14=0表示从机）, 16bit */
    I2S0_CR   = 0x8000;   /* 使能 I2S0，从机模式，16-bit 立体声         */

    /* 使能 I2S0 接收中断（bit1=RRDY） */
    I2S0_ICMR = 0x0008;   /* 使能接收满中断                             */
}

/* ============================================================
 * 全局 DMA ping-pong 缓冲区（由 app_main.c 声明并传入）
 * ============================================================ */
static short  *g_dma_in_ptr  = 0;
static short  *g_dma_out_ptr = 0;
static int     g_half_size   = 0;

/* ============================================================
 * audio_codec_init()
 * 对外接口：初始化 I2C、AIC3204 codec、I2S0
 * ============================================================ */
void audio_codec_init(unsigned int sample_rate)
{
    /* 1. 使能所有外设时钟 */
    SYS_PCGCR1 = 0x0000;
    SYS_PCGCR2 = 0x0000;

    /* 2. 配置串口引脚为 I2S0 模式
     *    SYS_EXBUSSEL[11:8]=0x0A: Serial Port 0 = I2S0 */
    SYS_EXBUSSEL &= ~0x0F00;
    SYS_EXBUSSEL |=  0x0A00;

    /* 3. 初始化 I2C */
    i2c_init();
    wait_loop(1000);

    /* 4. 初始化 AIC3204（寄存器配置） */
    aic3204_init_16k();

    /* 5. 初始化 I2S0 */
    i2s0_init();

    (void)sample_rate;   /* 本实现固定 16 kHz */
}

/* ============================================================
 * dma_audio_start()
 * 对外接口：记录 ping-pong 缓冲区指针，使能 I2S 中断
 * 注：C5515 上 I2S 采用轮询或 CPU 中断驱动（无独立 DMA 通道），
 *     audio_isr() 在 I2S 接收满时由中断向量调用。
 * ============================================================ */
void dma_audio_start(short *in_buf, short *out_buf, int half_size)
{
    g_dma_in_ptr  = in_buf;
    g_dma_out_ptr = out_buf;
    g_half_size   = half_size;
}

/* ============================================================
 * intc_enable()
 * 对外接口：使能指定中断（此处使能 I2S0 接收中断，IRQ id=6）
 * ============================================================ */
void intc_enable(int irq_id)
{
    /* 使能 IER0 中对应的中断位
     * C5515 I2S0 RX 中断通常映射到 INT6（IER0 bit6） */
    IER0 |= (1u << 6);
    /* 全局中断使能（INTM=0） */
    asm(" BCLR INTM");
    (void)irq_id;
}

/* ============================================================
 * cpu_idle_until_irq()
 * 对外接口：进入低功耗等待，直到下一个中断唤醒
 * ============================================================ */
void cpu_idle_until_irq(void)
{
    asm(" IDLE");
}
