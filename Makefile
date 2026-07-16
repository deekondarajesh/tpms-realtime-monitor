# Makefile for the TPMS Real-Time Sensor Monitoring System
#
# Targets:
#   make firmware   - cross-compiles the real STM32F103 firmware (.elf + .bin)
#   make test       - builds and runs the host-side unit tests natively
#   make clean      - removes build output

# ---------------- Firmware (STM32F103, Cortex-M3) ----------------

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

TARGET  = build/tpms_monitor

MCU_FLAGS = -mcpu=cortex-m3 -mthumb

FW_INCLUDES = -Isrc -IDrivers/CMSIS -IMiddlewares/FreeRTOS/include \
              -IMiddlewares/FreeRTOS/portable/GCC/ARM_CM3

FW_SOURCES = src/main.c \
             src/task_scheduler.c \
             src/pressure_calibration.c \
             src/can_comm.c \
             src/alert_detection.c \
             Middlewares/FreeRTOS/tasks.c \
             Middlewares/FreeRTOS/list.c \
             Middlewares/FreeRTOS/queue.c \
             Middlewares/FreeRTOS/timers.c \
             Middlewares/FreeRTOS/portable/GCC/ARM_CM3/port.c \
             Middlewares/FreeRTOS/portable/MemMang/heap_4.c

FW_ASM_SOURCES = Drivers/startup_stm32f103xb.s

LDSCRIPT = Drivers/STM32F103C8Tx_FLASH.ld

CFLAGS  = $(MCU_FLAGS) $(FW_INCLUDES) -Wall -Wextra -Os -ffunction-sections \
          -fdata-sections -std=c11 -g
LDFLAGS = $(MCU_FLAGS) -T$(LDSCRIPT) -Wl,--gc-sections -specs=nosys.specs \
          -specs=nano.specs -Wl,-Map=$(TARGET).map

.PHONY: firmware
firmware: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(FW_SOURCES) $(FW_ASM_SOURCES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(FW_ASM_SOURCES) $(FW_SOURCES) $(LDFLAGS) -o $@

# ---------------- Host-side unit tests ----------------

TEST_TARGET  = build/unit_tests
TEST_SOURCES = tests/unit_tests.c \
               src/pressure_calibration.c \
               src/alert_detection.c \
               src/can_comm.c

.PHONY: test
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	@mkdir -p build
	gcc -Isrc -IDrivers/CMSIS -Wall -Wextra -std=c11 -g $(TEST_SOURCES) -lm -o $@

.PHONY: clean
clean:
	rm -rf build
