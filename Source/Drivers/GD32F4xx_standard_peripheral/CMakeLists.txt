# Drivers/GD32F4xx_standard_peripheral/CMakeLists.txt

add_library(
  GD32F4xx_standard_peripheral EXCLUDE_FROM_ALL
  Source/gd32f4xx_adc.c
  Source/gd32f4xx_can.c
  Source/gd32f4xx_crc.c
  Source/gd32f4xx_ctc.c
  Source/gd32f4xx_dac.c
  Source/gd32f4xx_dbg.c
  Source/gd32f4xx_dci.c
  Source/gd32f4xx_dma.c
  Source/gd32f4xx_enet.c
  Source/gd32f4xx_exmc.c
  Source/gd32f4xx_exti.c
  Source/gd32f4xx_fmc.c
  Source/gd32f4xx_fwdgt.c
  Source/gd32f4xx_gpio.c
  Source/gd32f4xx_i2c.c
  Source/gd32f4xx_ipa.c
  Source/gd32f4xx_iref.c
  Source/gd32f4xx_misc.c
  Source/gd32f4xx_pmu.c
  Source/gd32f4xx_rcu.c
  Source/gd32f4xx_rtc.c
  Source/gd32f4xx_sdio.c
  Source/gd32f4xx_spi.c
  Source/gd32f4xx_syscfg.c
  Source/gd32f4xx_timer.c
  Source/gd32f4xx_tli.c
  Source/gd32f4xx_trng.c
  Source/gd32f4xx_usart.c
  Source/gd32f4xx_wwdgt.c)

target_include_directories(GD32F4xx_standard_peripheral PUBLIC Include)

add_dependencies(GD32F4xx_standard_peripheral CMSIS)
target_link_libraries(GD32F4xx_standard_peripheral CMSIS)

target_compile_options(GD32F4xx_standard_peripheral PRIVATE -Os)
